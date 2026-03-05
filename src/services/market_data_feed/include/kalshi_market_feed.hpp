#pragma once

#include "../include/market_feed.hpp"
#include "../include/order_book.hpp"
#include <unordered_map>
#include <thread>
#include <memory>

// Forward declarations for external libraries
namespace ix {
    class WebSocket;
}

namespace arbitrage {

/**
 * @brief Kalshi-specific market feed implementation
 */
class KalshiMarketFeed : public MarketFeedBase {
public:
    explicit KalshiMarketFeed(const MarketFeedConfig& config);
    ~KalshiMarketFeed() override;

    // IMarketFeed interface implementation
    bool connect() override;
    void disconnect() override;
    bool reconnect() override;
    bool subscribe(const std::vector<std::string>& market_tickers) override;
    bool unsubscribe(const std::vector<std::string>& market_tickers) override;
    
    // Order book queries
    TopOfBook get_top_of_book(const std::string& market_ticker) const override;
    std::optional<std::pair<double, double>> get_best_bid(const std::string& market_ticker) const override;
    std::optional<std::pair<double, double>> get_best_ask(const std::string& market_ticker) const override;
    std::optional<double> get_size_at_price(const std::string& market_ticker, int side, double price) const override;

    // Event loop
    void run() override;
    void stop() override;

private:
    // Authentication
    std::string load_private_key();
    std::string sign_pss_text(const std::string& text);
    std::unordered_map<std::string, std::string> create_auth_headers(const std::string& method, const std::string& path);

    // Message handling
    void on_message(const std::string& message);
    void handle_snapshot(const std::string& json_msg);
    void handle_orderbook_delta(const std::string& json_msg);
    void handle_trade(const std::string& json_msg);
    
    // Message creation
    std::string create_subscribe_message(const std::vector<std::string>& tickers);
    std::string create_unsubscribe_message(const std::vector<std::string>& tickers);

    // WebSocket management
    void setup_websocket();
    void send_message(const std::string& message);

private:
    MarketFeedConfig config_;
    std::string private_key_pem_;
    
    // Order books per market ticker
    mutable std::mutex orderbook_mutex_;
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> orderbooks_;
    
    // WebSocket
    std::unique_ptr<ix::WebSocket> websocket_;
    
    // Message ID counter
    std::atomic<int> message_id_{1};
};

/**
 * @brief Helper to register Kalshi feed with the factory
 */
struct KalshiMarketFeedRegistrar {
    KalshiMarketFeedRegistrar();
};

} // namespace arbitrage