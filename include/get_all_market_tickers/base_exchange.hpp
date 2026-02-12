#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "i_exchange.hpp"
#include "exchange_config.hpp"

namespace ticker_service {

/**
 * @brief Abstract base class for exchange implementations
 * Provides common functionality for all exchanges
 * Following Open/Closed Principle - open for extension, closed for modification
 */
class BaseExchange : public IFullExchange {
protected:
    ExchangeConfig config_;
    std::atomic<bool> connected_{false};
    mutable std::mutex mutex_;
    
    /**
     * @brief Parse JSON response to tickers (to be implemented by derived classes)
     * @param jsonResponse The raw JSON response from the API
     * @return Vector of parsed tickers
     */
    virtual std::vector<Ticker> parseTickersFromResponse(const std::string& jsonResponse) = 0;
    
    /**
     * @brief Make HTTP GET request to the exchange API
     * @param endpoint The API endpoint to call
     * @return Response body as string
     */
    virtual std::string makeGetRequest(const std::string& endpoint);
    
    /**
     * @brief Add authentication headers to request
     * @param headers Map of headers to modify
     */
    virtual void addAuthHeaders(std::map<std::string, std::string>& headers);
    
public:
    explicit BaseExchange(const ExchangeConfig& config) : config_(config) {}
    virtual ~BaseExchange() = default;
    
    // Non-copyable, movable
    BaseExchange(const BaseExchange&) = delete;
    BaseExchange& operator=(const BaseExchange&) = delete;
    BaseExchange(BaseExchange&&) = default;
    BaseExchange& operator=(BaseExchange&&) = default;
    
    bool isConnected() const override {
        return connected_.load();
    }
    
    bool connect() override {
        std::lock_guard<std::mutex> lock(mutex_);
        // Base implementation - derived classes can override for specific connection logic
        connected_ = true;
        return true;
    }
    
    void disconnect() override {
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = false;
    }
    
    const ExchangeConfig& getConfig() const { return config_; }
};

} // namespace ticker_service
