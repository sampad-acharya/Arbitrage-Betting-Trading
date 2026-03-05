#include "market_feed.hpp"
#include <algorithm>
#include <stdexcept>

namespace arbitrage {

// MarketFeedBase implementation

MarketFeedBase::MarketFeedBase(const std::string& market_name, const std::string& ws_url)
    : market_name_(market_name)
    , ws_url_(ws_url)
    , connected_(false)
    , running_(false)
{
}

bool MarketFeedBase::is_connected() const {
    return connected_.load();
}

std::vector<std::string> MarketFeedBase::get_subscribed_markets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscribed_markets_;
}

std::string MarketFeedBase::get_market_name() const {
    return market_name_;
}

std::string MarketFeedBase::get_websocket_url() const {
    return ws_url_;
}

void MarketFeedBase::set_on_snapshot(OnSnapshotCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_snapshot_ = std::move(callback);
}

void MarketFeedBase::set_on_update(OnUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_update_ = std::move(callback);
}

void MarketFeedBase::set_on_trade(OnTradeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_trade_ = std::move(callback);
}

void MarketFeedBase::set_on_connected(OnConnectedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_connected_ = std::move(callback);
}

void MarketFeedBase::set_on_disconnected(OnDisconnectedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_disconnected_ = std::move(callback);
}

void MarketFeedBase::set_on_error(OnErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_error_ = std::move(callback);
}

void MarketFeedBase::invoke_on_snapshot(const OrderBookSnapshot& snapshot) {
    OnSnapshotCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_snapshot_;
    }
    if (callback) {
        callback(snapshot);
    }
}

void MarketFeedBase::invoke_on_update(const OrderBookUpdate& update) {
    OnUpdateCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_update_;
    }
    if (callback) {
        callback(update);
    }
}

void MarketFeedBase::invoke_on_trade(const TradeEvent& trade) {
    OnTradeCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_trade_;
    }
    if (callback) {
        callback(trade);
    }
}

void MarketFeedBase::invoke_on_connected() {
    OnConnectedCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_connected_;
    }
    if (callback) {
        callback();
    }
}

void MarketFeedBase::invoke_on_disconnected(const std::string& reason) {
    OnDisconnectedCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_disconnected_;
    }
    if (callback) {
        callback(reason);
    }
}

void MarketFeedBase::invoke_on_error(const std::string& error) {
    OnErrorCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_error_;
    }
    if (callback) {
        callback(error);
    }
}

void MarketFeedBase::set_connected(bool connected) {
    connected_.store(connected);
}

void MarketFeedBase::add_subscribed_market(const std::string& ticker) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(subscribed_markets_.begin(), subscribed_markets_.end(), ticker) == subscribed_markets_.end()) {
        subscribed_markets_.push_back(ticker);
    }
}

void MarketFeedBase::remove_subscribed_market(const std::string& ticker) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(subscribed_markets_.begin(), subscribed_markets_.end(), ticker);
    if (it != subscribed_markets_.end()) {
        subscribed_markets_.erase(it);
    }
}

void MarketFeedBase::clear_subscribed_markets() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribed_markets_.clear();
}

// MarketFeedFactory implementation

std::unordered_map<std::string, MarketFeedFactory::CreateFunc>& MarketFeedFactory::get_registry() {
    static std::unordered_map<std::string, CreateFunc> registry;
    return registry;
}

void MarketFeedFactory::register_market(const std::string& market_name, CreateFunc creator) {
    get_registry()[market_name] = std::move(creator);
}

std::unique_ptr<IMarketFeed> MarketFeedFactory::create(const std::string& market_name, const MarketFeedConfig& config) {
    auto& registry = get_registry();
    auto it = registry.find(market_name);
    if (it == registry.end()) {
        throw std::runtime_error("Unknown market: " + market_name);
    }
    return it->second(config);
}

} // namespace arbitrage