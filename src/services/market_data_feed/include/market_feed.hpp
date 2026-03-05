#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>
#include <mutex>

namespace arbitrage {

// Forward declarations
struct OrderBookLevel {
    double price;
    double size;
};

struct TopOfBook {
    std::optional<double> best_bid_price;
    std::optional<double> best_bid_size;
    std::optional<double> best_ask_price;
    std::optional<double> best_ask_size;
};

struct TradeEvent {
    std::string market_ticker;
    double price;
    double size;
    int taker_side;  // 0 = buy, 1 = sell
    uint64_t timestamp;
};

struct OrderBookUpdate {
    std::string market_ticker;
    int side;  // 0 = bid, 1 = ask
    double price;
    double size;
    uint64_t timestamp;
};

struct OrderBookSnapshot {
    std::string market_ticker;
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
    uint64_t timestamp;
};

// Callback types
using OnSnapshotCallback = std::function<void(const OrderBookSnapshot&)>;
using OnUpdateCallback = std::function<void(const OrderBookUpdate&)>;
using OnTradeCallback = std::function<void(const TradeEvent&)>;
using OnConnectedCallback = std::function<void()>;
using OnDisconnectedCallback = std::function<void(const std::string& reason)>;
using OnErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Abstract interface for market data feeds
 * 
 * This interface can be inherited by any market-specific implementation
 * (Kalshi, Polymarket, PredictIt, etc.)
 */
class IMarketFeed {
public:
    virtual ~IMarketFeed() = default;

    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    virtual bool reconnect() = 0;

    // Subscription management
    virtual bool subscribe(const std::vector<std::string>& market_tickers) = 0;
    virtual bool unsubscribe(const std::vector<std::string>& market_tickers) = 0;
    virtual std::vector<std::string> get_subscribed_markets() const = 0;

    // Order book queries
    virtual TopOfBook get_top_of_book(const std::string& market_ticker) const = 0;
    virtual std::optional<std::pair<double, double>> get_best_bid(const std::string& market_ticker) const = 0;
    virtual std::optional<std::pair<double, double>> get_best_ask(const std::string& market_ticker) const = 0;
    virtual std::optional<double> get_size_at_price(const std::string& market_ticker, int side, double price) const = 0;

    // Callback registration
    virtual void set_on_snapshot(OnSnapshotCallback callback) = 0;
    virtual void set_on_update(OnUpdateCallback callback) = 0;
    virtual void set_on_trade(OnTradeCallback callback) = 0;
    virtual void set_on_connected(OnConnectedCallback callback) = 0;
    virtual void set_on_disconnected(OnDisconnectedCallback callback) = 0;
    virtual void set_on_error(OnErrorCallback callback) = 0;

    // Market info
    virtual std::string get_market_name() const = 0;
    virtual std::string get_websocket_url() const = 0;
};

/**
 * @brief Base implementation of IMarketFeed with common functionality
 */
class MarketFeedBase : public IMarketFeed {
public:
    MarketFeedBase(const std::string& market_name, const std::string& ws_url);
    ~MarketFeedBase() override = default;

    // IMarketFeed interface - common implementations
    bool is_connected() const override;
    std::vector<std::string> get_subscribed_markets() const override;
    std::string get_market_name() const override;
    std::string get_websocket_url() const override;

    // Callback registration
    void set_on_snapshot(OnSnapshotCallback callback) override;
    void set_on_update(OnUpdateCallback callback) override;
    void set_on_trade(OnTradeCallback callback) override;
    void set_on_connected(OnConnectedCallback callback) override;
    void set_on_disconnected(OnDisconnectedCallback callback) override;
    void set_on_error(OnErrorCallback callback) override;

protected:
    // Helper methods for derived classes
    void invoke_on_snapshot(const OrderBookSnapshot& snapshot);
    void invoke_on_update(const OrderBookUpdate& update);
    void invoke_on_trade(const TradeEvent& trade);
    void invoke_on_connected();
    void invoke_on_disconnected(const std::string& reason);
    void invoke_on_error(const std::string& error);

    void set_connected(bool connected);
    void add_subscribed_market(const std::string& ticker);
    void remove_subscribed_market(const std::string& ticker);

    // Pure virtual methods that must be implemented by derived classes
    virtual void handle_snapshot_message(const std::string& raw_message) = 0;
    virtual void handle_update_message(const std::string& raw_message) = 0;
    virtual void handle_trade_message(const std::string& raw_message) = 0;
    virtual std::string create_subscribe_message(const std::vector<std::string>& tickers) = 0;
    virtual std::string create_unsubscribe_message(const std::vector<std::string>& tickers) = 0;

protected:
    std::string market_name_;
    std::string ws_url_;
    bool connected_;
    std::vector<std::string> subscribed_markets_;
    mutable std::mutex mutex_;

    // Callbacks
    OnSnapshotCallback on_snapshot_;
    OnUpdateCallback on_update_;
    OnTradeCallback on_trade_;
    OnConnectedCallback on_connected_;
    OnDisconnectedCallback on_disconnected_;
    OnErrorCallback on_error_;
};

/**
 * @brief Configuration for market feed authentication
 */
struct MarketFeedConfig {
    std::string api_key;
    std::string private_key_path;
    std::string ws_url;
    std::vector<std::string> market_tickers;
    
    // Optional settings
    int reconnect_delay_ms = 5000;
    int ping_interval_ms = 20000;
    int ping_timeout_ms = 10000;
};

/**
 * @brief Factory for creating market feeds
 */
class MarketFeedFactory {
public:
    using CreateFunc = std::function<std::unique_ptr<IMarketFeed>(const MarketFeedConfig&)>;
    
    static void register_market(const std::string& market_name, CreateFunc creator);
    static std::unique_ptr<IMarketFeed> create(const std::string& market_name, const MarketFeedConfig& config);
    
private:
    static std::unordered_map<std::string, CreateFunc>& get_registry();
};

} // namespace arbitrage