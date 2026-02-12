#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ticker.hpp"

namespace ticker_service {

/**
 * @brief Abstract interface for exchange connections
 * Following Interface Segregation Principle - minimal interface for exchange operations
 * Following Dependency Inversion Principle - high-level modules depend on this abstraction
 */
class IExchange {
public:
    virtual ~IExchange() = default;
    
    /**
     * @brief Get the name of the exchange
     * @return Exchange name as string
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Check if connection to exchange is healthy
     * @return true if connected and operational
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Establish connection to the exchange
     * @return true if connection successful
     */
    virtual bool connect() = 0;
    
    /**
     * @brief Disconnect from the exchange
     */
    virtual void disconnect() = 0;
};

/**
 * @brief Interface for fetching tickers from an exchange
 * Following Interface Segregation - separate interface for ticker fetching
 */
class ITickerFetcher {
public:
    virtual ~ITickerFetcher() = default;
    
    /**
     * @brief Fetch all available tickers from the exchange
     * @return Result containing tickers or error information
     */
    virtual TickerFetchResult fetchAllTickers() = 0;
    
    /**
     * @brief Fetch tickers filtered by category
     * @param category The category to filter by
     * @return Result containing filtered tickers or error information
     */
    virtual TickerFetchResult fetchTickersByCategory(const std::string& category) = 0;
    
    /**
     * @brief Fetch a specific ticker by symbol
     * @param symbol The ticker symbol to fetch
     * @return Result containing the ticker or error information
     */
    virtual TickerFetchResult fetchTickerBySymbol(const std::string& symbol) = 0;
};

/**
 * @brief Combined interface for a full-featured exchange
 * Combines connection management and ticker fetching capabilities
 */
class IFullExchange : public IExchange, public ITickerFetcher {
public:
    virtual ~IFullExchange() = default;
};

} // namespace ticker_service
