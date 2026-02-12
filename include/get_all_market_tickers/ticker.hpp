#pragma once

#include <string>
#include <chrono>
#include <optional>

namespace ticker_service {

/**
 * @brief Represents a single ticker/market from an exchange
 * Following Single Responsibility Principle - only holds ticker data
 */
struct Ticker {
    std::string symbol;              // Unique identifier for the ticker
    std::string name;                // Human-readable name
    std::string exchange;            // Exchange source (e.g., "KALSHI", "POLYMARKET", "CME")
    std::string category;            // Market category (e.g., "politics", "sports", "commodities")
    
    double lastPrice{0.0};           // Last traded price
    double bidPrice{0.0};            // Current best bid
    double askPrice{0.0};            // Current best ask
    double volume24h{0.0};           // 24-hour volume
    
    bool isActive{true};             // Whether the market is currently active
    std::optional<std::chrono::system_clock::time_point> expirationDate;  // When the market expires (if applicable)
    std::chrono::system_clock::time_point lastUpdated;  // Last update timestamp
    
    Ticker() = default;
    
    Ticker(const std::string& symbol_, 
           const std::string& name_,
           const std::string& exchange_,
           const std::string& category_)
        : symbol(symbol_)
        , name(name_)
        , exchange(exchange_)
        , category(category_)
        , lastUpdated(std::chrono::system_clock::now()) {}
    
    // Comparison operators for container operations
    bool operator==(const Ticker& other) const {
        return symbol == other.symbol && exchange == other.exchange;
    }
    
    bool operator<(const Ticker& other) const {
        if (exchange != other.exchange) return exchange < other.exchange;
        return symbol < other.symbol;
    }
};

/**
 * @brief Represents the result of a ticker fetch operation
 */
struct TickerFetchResult {
    bool success{false};
    std::string errorMessage;
    std::vector<Ticker> tickers;
    std::chrono::system_clock::time_point fetchTime;
    
    TickerFetchResult() : fetchTime(std::chrono::system_clock::now()) {}
    
    static TickerFetchResult Success(std::vector<Ticker> tickers) {
        TickerFetchResult result;
        result.success = true;
        result.tickers = std::move(tickers);
        return result;
    }
    
    static TickerFetchResult Failure(const std::string& error) {
        TickerFetchResult result;
        result.success = false;
        result.errorMessage = error;
        return result;
    }
};

} // namespace ticker_service
