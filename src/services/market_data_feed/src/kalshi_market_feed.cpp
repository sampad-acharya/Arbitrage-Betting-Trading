#include "kalshi_market_feed.hpp"

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <cstring>

using json = nlohmann::json;

namespace arbitrage {

// Anonymous namespace for helper functions
namespace {

std::string base64_encode(const unsigned char* data, size_t len) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    result.reserve(((len + 2) / 3) * 4);
    
    for (size_t i = 0; i < len; i += 3) {
        unsigned int triple = (data[i] << 16);
        if (i + 1 < len) triple |= (data[i + 1] << 8);
        if (i + 2 < len) triple |= data[i + 2];
        
        result.push_back(base64_chars[(triple >> 18) & 0x3F]);
        result.push_back(base64_chars[(triple >> 12) & 0x3F]);
        result.push_back((i + 1 < len) ? base64_chars[(triple >> 6) & 0x3F] : '=');
        result.push_back((i + 2 < len) ? base64_chars[triple & 0x3F] : '=');
    }
    
    return result;
}

uint64_t get_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // anonymous namespace

KalshiMarketFeed::KalshiMarketFeed(const MarketFeedConfig& config)
    : MarketFeedBase("Kalshi", config.ws_url)
    , config_(config)
{
    // Initialize order books for configured tickers
    for (const auto& ticker : config_.market_tickers) {
        orderbooks_[ticker] = std::make_unique<OrderBook>(ticker);
    }
    
    // Load private key
    private_key_pem_ = load_private_key();
}

KalshiMarketFeed::~KalshiMarketFeed() {
    stop();
    disconnect();
}

std::string KalshiMarketFeed::load_private_key() {
    std::ifstream file(config_.private_key_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open private key file: " + config_.private_key_path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string KalshiMarketFeed::sign_pss_text(const std::string& text) {
    // Load the private key
    BIO* bio = BIO_new_mem_buf(private_key_pem_.c_str(), -1);
    if (!bio) {
        throw std::runtime_error("Failed to create BIO for private key");
    }
    
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!pkey) {
        throw std::runtime_error("Failed to read private key");
    }
    
    // Create signing context
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to create MD context");
    }
    
    EVP_PKEY_CTX* pkey_ctx = nullptr;
    
    if (EVP_DigestSignInit(md_ctx, &pkey_ctx, EVP_sha256(), nullptr, pkey) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to initialize digest sign");
    }
    
    // Set PSS padding
    if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to set PSS padding");
    }
    
    // Set salt length to digest length
    if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, RSA_PSS_SALTLEN_DIGEST) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to set salt length");
    }
    
    // Sign the message
    if (EVP_DigestSignUpdate(md_ctx, text.c_str(), text.length()) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to update digest");
    }
    
    // Get signature length
    size_t sig_len = 0;
    if (EVP_DigestSignFinal(md_ctx, nullptr, &sig_len) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to get signature length");
    }
    
    // Get signature
    std::vector<unsigned char> signature(sig_len);
    if (EVP_DigestSignFinal(md_ctx, signature.data(), &sig_len) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to sign message");
    }
    
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    
    // Base64 encode the signature
    return base64_encode(signature.data(), sig_len);
}

std::unordered_map<std::string, std::string> KalshiMarketFeed::create_auth_headers(
    const std::string& method, 
    const std::string& path
) {
    uint64_t timestamp = get_timestamp_ms();
    std::string timestamp_str = std::to_string(timestamp);
    
    // Extract path without query string
    std::string path_only = path;
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path_only = path.substr(0, query_pos);
    }
    
    // Create message string: timestamp + method + path
    std::string msg_string = timestamp_str + method + path_only;
    
    // Sign the message
    std::string signature = sign_pss_text(msg_string);
    
    return {
        {"Content-Type", "application/json"},
        {"KALSHI-ACCESS-KEY", config_.api_key},
        {"KALSHI-ACCESS-SIGNATURE", signature},
        {"KALSHI-ACCESS-TIMESTAMP", timestamp_str}
    };
}

void KalshiMarketFeed::setup_websocket() {
    websocket_ = std::make_unique<ix::WebSocket>();
    
    // Create auth headers
    auto headers = create_auth_headers("GET", "/trade-api/ws/v2");
    
    ix::WebSocketHttpHeaders ws_headers;
    for (const auto& [key, value] : headers) {
        ws_headers[key] = value;
    }
    
    websocket_->setUrl(ws_url_);
    websocket_->setExtraHeaders(ws_headers);
    websocket_->setPingInterval(config_.ping_interval_ms / 1000);
    websocket_->setPingTimeout(config_.ping_timeout_ms / 1000);
    
    // Set message callback
    websocket_->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
            case ix::WebSocketMessageType::Open:
                set_connected(true);
                invoke_on_connected();
                
                // Subscribe to configured tickers
                if (!config_.market_tickers.empty()) {
                    subscribe(config_.market_tickers);
                }
                break;
                
            case ix::WebSocketMessageType::Close:
                set_connected(false);
                invoke_on_disconnected(msg->closeInfo.reason);
                break;
                
            case ix::WebSocketMessageType::Error:
                invoke_on_error(msg->errorInfo.reason);
                break;
                
            case ix::WebSocketMessageType::Message:
                on_message(msg->str);
                break;
                
            case ix::WebSocketMessageType::Ping:
            case ix::WebSocketMessageType::Pong:
            case ix::WebSocketMessageType::Fragment:
                // Handled automatically
                break;
        }
    });
}

bool KalshiMarketFeed::connect() {
    if (is_connected()) {
        return true;
    }
    
    try {
        setup_websocket();
        websocket_->start();
        return true;
    } catch (const std::exception& e) {
        invoke_on_error(std::string("Connection failed: ") + e.what());
        return false;
    }
}

void KalshiMarketFeed::disconnect() {
    if (websocket_) {
        websocket_->stop();
        websocket_.reset();
    }
    set_connected(false);
}

bool KalshiMarketFeed::reconnect() {
    disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
    return connect();
}

void KalshiMarketFeed::run() {
    running_.store(true);
    
    while (running_.load()) {
        if (!is_connected()) {
            if (!connect()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
                continue;
            }
        }
        
        // Sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void KalshiMarketFeed::stop() {
    running_.store(false);
}

void KalshiMarketFeed::send_message(const std::string& message) {
    if (websocket_ && is_connected()) {
        websocket_->send(message);
    }
}

std::string KalshiMarketFeed::create_subscribe_message(const std::vector<std::string>& tickers) {
    json msg = {
        {"id", message_id_.fetch_add(1)},
        {"cmd", "subscribe"},
        {"params", {
            {"channels", {"orderbook_delta", "trade"}},
            {"market_tickers", tickers}
        }}
    };
    return msg.dump();
}

std::string KalshiMarketFeed::create_unsubscribe_message(const std::vector<std::string>& tickers) {
    json msg = {
        {"id", message_id_.fetch_add(1)},
        {"cmd", "unsubscribe"},
        {"params", {
            {"channels", {"orderbook_delta", "trade"}},
            {"market_tickers", tickers}
        }}
    };
    return msg.dump();
}

bool KalshiMarketFeed::subscribe(const std::vector<std::string>& market_tickers) {
    if (!is_connected()) {
        return false;
    }
    
    // Initialize order books for new tickers
    {
        std::lock_guard<std::mutex> lock(orderbook_mutex_);
        for (const auto& ticker : market_tickers) {
            if (orderbooks_.find(ticker) == orderbooks_.end()) {
                orderbooks_[ticker] = std::make_unique<OrderBook>(ticker);
            }
        }
    }
    
    // Add to subscribed markets
    for (const auto& ticker : market_tickers) {
        add_subscribed_market(ticker);
    }
    
    std::string msg = create_subscribe_message(market_tickers);
    send_message(msg);
    
    return true;
}

bool KalshiMarketFeed::unsubscribe(const std::vector<std::string>& market_tickers) {
    if (!is_connected()) {
        return false;
    }
    
    for (const auto& ticker : market_tickers) {
        remove_subscribed_market(ticker);
    }
    
    std::string msg = create_unsubscribe_message(market_tickers);
    send_message(msg);
    
    return true;
}

void KalshiMarketFeed::on_message(const std::string& message) {
    try {
        json data = json::parse(message);
        
        std::string msg_type = data.value("type", "");
        
        if (msg_type == "subscribed") {
            // Subscription confirmed
            return;
        }
        
        if (!data.contains("msg")) {
            return;
        }
        
        std::string msg_content = data["msg"].dump();
        
        if (msg_type == "orderbook_snapshot") {
            handle_snapshot(msg_content);
        } else if (msg_type == "orderbook_delta") {
            handle_orderbook_delta(msg_content);
        } else if (msg_type == "trade") {
            handle_trade(msg_content);
        } else if (msg_type == "error") {
            invoke_on_error("Kalshi error: " + msg_content);
        }
        
    } catch (const json::exception& e) {
        invoke_on_error(std::string("JSON parse error: ") + e.what());
    }
}

void KalshiMarketFeed::handle_snapshot(const std::string& json_msg) {
    try {
        json msg = json::parse(json_msg);
        
        std::string market_ticker = msg.value("market_ticker", "");
        
        if (market_ticker.empty()) {
            return;
        }
        
        // Parse bids and asks
        std::map<double, double> bids;
        std::map<double, double> asks;
        
        if (msg.contains("yes")) {
            for (const auto& level : msg["yes"]) {
                double price = std::stod(level[0].get<std::string>());
                double size = level[1].get<double>();
                bids[price] = size;
            }
        }
        
        if (msg.contains("no")) {
            for (const auto& level : msg["no"]) {
                double price = 1.0 - std::stod(level[0].get<std::string>());
                double size = level[1].get<double>();
                asks[price] = size;
            }
        }
        
        // Update order book
        {
            std::lock_guard<std::mutex> lock(orderbook_mutex_);
            auto it = orderbooks_.find(market_ticker);
            if (it != orderbooks_.end()) {
                it->second->load_snapshot(bids, asks);
            }
        }
        
        // Create and invoke callback
        OrderBookSnapshot snapshot;
        snapshot.market_ticker = market_ticker;
        snapshot.timestamp = get_timestamp_ms();
        
        for (const auto& [price, size] : bids) {
            snapshot.bids.push_back({price, size});
        }
        for (const auto& [price, size] : asks) {
            snapshot.asks.push_back({price, size});
        }
        
        invoke_on_snapshot(snapshot);
        
    } catch (const std::exception& e) {
        invoke_on_error(std::string("Snapshot parse error: ") + e.what());
    }
}

void KalshiMarketFeed::handle_orderbook_delta(const std::string& json_msg) {
    try {
        json msg = json::parse(json_msg);
        
        std::string market_ticker = msg.value("market_ticker", "");
        std::string side_str = msg.value("side", "");
        double delta = msg.value("delta", 0.0);
        
        if (market_ticker.empty() || side_str.empty()) {
            return;
        }
        
        // Parse price
        double price = 0.0;
        if (msg.contains("price_dollars")) {
            price = std::stod(msg["price_dollars"].get<std::string>());
        } else if (msg.contains("price")) {
            price = msg["price"].get<double>();
        }
        
        // Convert "no" side price
        int side = (side_str == "yes") ? 0 : 1;
        if (side == 1) {
            price = 1.0 - price;
        }
        
        // Update order book
        {
            std::lock_guard<std::mutex> lock(orderbook_mutex_);
            auto it = orderbooks_.find(market_ticker);
            if (it != orderbooks_.end()) {
                it->second->update_delta(side, price, delta);
            }
        }
        
        // Create and invoke callback
        OrderBookUpdate update;
        update.market_ticker = market_ticker;
        update.side = side;
        update.price = price;
        update.size_delta = delta;
        update.timestamp = get_timestamp_ms();
        
        invoke_on_update(update);
        
    } catch (const std::exception& e) {
        invoke_on_error(std::string("Delta parse error: ") + e.what());
    }
}

void KalshiMarketFeed::handle_trade(const std::string& json_msg) {
    try {
        json msg = json::parse(json_msg);
        
        std::string market_ticker = msg.value("market_ticker", "");
        std::string taker_side_str = msg.value("taker_side", "");
        double count = msg.value("count", 0.0);
        
        if (market_ticker.empty()) {
            return;
        }
        
        // Parse price
        double price = 0.0;
        if (msg.contains("yes_price_dollars")) {
            price = std::stod(msg["yes_price_dollars"].get<std::string>());
        }
        
        int taker_side = (taker_side_str == "yes") ? 0 : 1;
        
        // Create and invoke callback
        TradeEvent trade;
        trade.market_ticker = market_ticker;
        trade.price = price;
        trade.size = count;
        trade.taker_side = taker_side;
        trade.timestamp = get_timestamp_ms();
        
        invoke_on_trade(trade);
        
    } catch (const std::exception& e) {
        invoke_on_error(std::string("Trade parse error: ") + e.what());
    }
}

TopOfBook KalshiMarketFeed::get_top_of_book(const std::string& market_ticker) const {
    TopOfBook tob;
    
    std::lock_guard<std::mutex> lock(orderbook_mutex_);
    auto it = orderbooks_.find(market_ticker);
    if (it == orderbooks_.end()) {
        return tob;
    }
    
    auto [bid_price, bid_size] = it->second->get_best_bid();
    auto [ask_price, ask_size] = it->second->get_best_ask();
    
    tob.best_bid_price = bid_price;
    tob.best_bid_size = bid_size;
    tob.best_ask_price = ask_price;
    tob.best_ask_size = ask_size;
    
    return tob;
}

std::optional<std::pair<double, double>> KalshiMarketFeed::get_best_bid(const std::string& market_ticker) const {
    std::lock_guard<std::mutex> lock(orderbook_mutex_);
    auto it = orderbooks_.find(market_ticker);
    if (it == orderbooks_.end()) {
        return std::nullopt;
    }
    
    auto [price, size] = it->second->get_best_bid();
    if (price && size) {
        return std::make_pair(*price, *size);
    }
    return std::nullopt;
}

std::optional<std::pair<double, double>> KalshiMarketFeed::get_best_ask(const std::string& market_ticker) const {
    std::lock_guard<std::mutex> lock(orderbook_mutex_);
    auto it = orderbooks_.find(market_ticker);
    if (it == orderbooks_.end()) {
        return std::nullopt;
    }
    
    auto [price, size] = it->second->get_best_ask();
    if (price && size) {
        return std::make_pair(*price, *size);
    }
    return std::nullopt;
}

std::optional<double> KalshiMarketFeed::get_size_at_price(
    const std::string& market_ticker, 
    int side, 
    double price
) const {
    std::lock_guard<std::mutex> lock(orderbook_mutex_);
    auto it = orderbooks_.find(market_ticker);
    if (it == orderbooks_.end()) {
        return std::nullopt;
    }
    
    return it->second->get_size_at_price(side, price);
}

// Factory registration
KalshiMarketFeedRegistrar::KalshiMarketFeedRegistrar() {
    MarketFeedFactory::register_market("Kalshi", [](const MarketFeedConfig& config) {
        return std::make_unique<KalshiMarketFeed>(config);
    });
}

// Static instance to trigger registration
static KalshiMarketFeedRegistrar kalshi_registrar;

} // namespace arbitrage