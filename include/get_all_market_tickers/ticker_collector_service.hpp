#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <future>
#include <functional>
#include "i_exchange.hpp"
#include "ticker.hpp"

namespace ticker_service {

/**
 * @brief Callback type for ticker update notifications
 */
using TickerUpdateCallback = std::function<void(const std::string& exchange, const std::vector<Ticker>& tickers)>;
using ErrorCallback = std::function<void(const std::string& exchange, const std::string& error)>;

/**
 * @brief Service for collecting tickers from multiple exchanges
 * Following Single Responsibility - orchestrates ticker collection only
 * Following Dependency Inversion - depends on IFullExchange abstraction
 */
class TickerCollectorService {
private:
    std::unordered_map<std::string, std::unique_ptr<IFullExchange>> exchanges_;
    std::unordered_map<std::string, std::vector<Ticker>> tickerCache_;
    mutable std::mutex mutex_;
    
    TickerUpdateCallback onTickerUpdate_;
    ErrorCallback onError_;
    
public:
    TickerCollectorService() = default;
    ~TickerCollectorService() = default;
    
    // Non-copyable, movable
    TickerCollectorService(const TickerCollectorService&) = delete;
    TickerCollectorService& operator=(const TickerCollectorService&) = delete;
    TickerCollectorService(TickerCollectorService&&) = default;
    TickerCollectorService& operator=(TickerCollectorService&&) = default;
    
    /**
     * @brief Add an exchange to the collector
     * @param exchange Unique pointer to the exchange
     */
    void addExchange(std::unique_ptr<IFullExchange> exchange);
    
    /**
     * @brief Remove an exchange from the collector
     * @param exchangeName Name of the exchange to remove
     * @return true if exchange was removed
     */
    bool removeExchange(const std::string& exchangeName);
    
    /**
     * @brief Get an exchange by name
     * @param exchangeName Name of the exchange
     * @return Pointer to the exchange (nullptr if not found)
     */
    IFullExchange* getExchange(const std::string& exchangeName);
    
    /**
     * @brief Get all registered exchange names
     * @return Vector of exchange names
     */
    std::vector<std::string> getExchangeNames() const;
    
    /**
     * @brief Connect to all registered exchanges
     * @return Map of exchange name to connection success
     */
    std::unordered_map<std::string, bool> connectAll();
    
    /**
     * @brief Disconnect from all exchanges
     */
    void disconnectAll();
    
    /**
     * @brief Fetch tickers from all exchanges synchronously
     * @return Map of exchange name to ticker fetch result
     */
    std::unordered_map<std::string, TickerFetchResult> fetchAllTickers();
    
    /**
     * @brief Fetch tickers from a specific exchange
     * @param exchangeName Name of the exchange
     * @return Ticker fetch result
     */
    TickerFetchResult fetchTickersFromExchange(const std::string& exchangeName);
    
    /**
     * @brief Fetch tickers from all exchanges asynchronously
     * @return Future containing map of results
     */
    std::future<std::unordered_map<std::string, TickerFetchResult>> fetchAllTickersAsync();
    
    /**
     * @brief Fetch tickers by category from all exchanges
     * @param category The category to filter by
     * @return Map of exchange name to filtered results
     */
    std::unordered_map<std::string, TickerFetchResult> fetchTickersByCategory(const std::string& category);
    
    /**
     * @brief Search for a ticker across all exchanges
     * @param symbol The symbol to search for
     * @return Vector of matching tickers from all exchanges
     */
    std::vector<Ticker> searchTicker(const std::string& symbol);
    
    /**
     * @brief Get cached tickers for an exchange
     * @param exchangeName Name of the exchange
     * @return Vector of cached tickers
     */
    std::vector<Ticker> getCachedTickers(const std::string& exchangeName) const;
    
    /**
     * @brief Get all cached tickers from all exchanges
     * @return Vector of all cached tickers
     */
    std::vector<Ticker> getAllCachedTickers() const;
    
    /**
     * @brief Set callback for ticker updates
     * @param callback Function to call when tickers are updated
     */
    void setOnTickerUpdate(TickerUpdateCallback callback);
    
    /**
     * @brief Set callback for errors
     * @param callback Function to call when errors occur
     */
    void setOnError(ErrorCallback callback);
    
    /**
     * @brief Clear all cached tickers
     */
    void clearCache();
    
    /**
     * @brief Get statistics about cached tickers
     * @return Map of exchange name to ticker count
     */
    std::unordered_map<std::string, size_t> getCacheStats() const;
};

} // namespace ticker_service
