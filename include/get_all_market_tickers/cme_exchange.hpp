#pragma once

#include <string>
#include <vector>
#include <memory>
#include "base_exchange.hpp"
#include "exchange_config.hpp"

namespace ticker_service {

/**
 * @brief CME Group exchange implementation
 * Supports futures and options from CME, CBOT, NYMEX, COMEX
 * Following Liskov Substitution Principle - can be used wherever IFullExchange is expected
 * Following Single Responsibility - handles only CME-specific logic
 */
class CMEExchange : public BaseExchange {
public:
    /**
     * @brief CME product types
     */
    enum class ProductType {
        FUTURES,
        OPTIONS,
        SPREADS,
        ALL
    };
    
    /**
     * @brief CME sub-exchanges
     */
    enum class CMEVenue {
        CME,        // Chicago Mercantile Exchange
        CBOT,       // Chicago Board of Trade
        NYMEX,      // New York Mercantile Exchange
        COMEX,      // Commodity Exchange
        ALL
    };

private:
    static constexpr const char* EXCHANGE_NAME = "CME";
    static constexpr const char* PRODUCTS_ENDPOINT = "/products";
    static constexpr const char* QUOTES_ENDPOINT = "/quotes";
    
    CMEConfig cmeConfig_;
    
    /**
     * @brief Convert venue enum to string
     */
    static std::string venueToString(CMEVenue venue);
    
    /**
     * @brief Convert product type enum to string
     */
    static std::string productTypeToString(ProductType type);
    
protected:
    std::vector<Ticker> parseTickersFromResponse(const std::string& jsonResponse) override;
    void addAuthHeaders(std::map<std::string, std::string>& headers) override;
    
public:
    explicit CMEExchange(const CMEConfig& config);
    ~CMEExchange() override = default;
    
    std::string getName() const override { return EXCHANGE_NAME; }
    
    bool connect() override;
    
    TickerFetchResult fetchAllTickers() override;
    TickerFetchResult fetchTickersByCategory(const std::string& category) override;
    TickerFetchResult fetchTickerBySymbol(const std::string& symbol) override;
    
    /**
     * @brief Fetch tickers from a specific CME venue
     * @param venue The CME venue to fetch from
     * @return Result containing tickers from the venue
     */
    TickerFetchResult fetchTickersByVenue(CMEVenue venue);
    
    /**
     * @brief Fetch tickers of a specific product type
     * @param type The product type to filter by
     * @return Result containing filtered tickers
     */
    TickerFetchResult fetchTickersByProductType(ProductType type);
    
    /**
     * @brief Fetch tickers by asset class (e.g., "agriculture", "energy", "metals")
     * @param assetClass The asset class to filter by
     * @return Result containing filtered tickers
     */
    TickerFetchResult fetchTickersByAssetClass(const std::string& assetClass);
};

} // namespace ticker_service
