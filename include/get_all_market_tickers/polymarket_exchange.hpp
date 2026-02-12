#pragma once

#include <string>
#include <vector>
#include <memory>
#include "base_exchange.hpp"
#include "exchange_config.hpp"

namespace ticker_service {

/**
 * @brief Polymarket exchange implementation
 * Following Liskov Substitution Principle - can be used wherever IFullExchange is expected
 * Following Single Responsibility - handles only Polymarket-specific logic
 */
class PolymarketExchange : public BaseExchange {
private:
    static constexpr const char* EXCHANGE_NAME = "POLYMARKET";
    static constexpr const char* MARKETS_ENDPOINT = "/markets";
    static constexpr const char* TOKENS_ENDPOINT = "/tokens";
    
    PolymarketConfig polyConfig_;
    
    /**
     * @brief Fetch data from Gamma API (used for market metadata)
     * @param endpoint The gamma API endpoint
     * @return Response body
     */
    std::string fetchFromGammaApi(const std::string& endpoint);
    
protected:
    std::vector<Ticker> parseTickersFromResponse(const std::string& jsonResponse) override;
    
public:
    explicit PolymarketExchange(const PolymarketConfig& config);
    ~PolymarketExchange() override = default;
    
    std::string getName() const override { return EXCHANGE_NAME; }
    
    bool connect() override;
    
    TickerFetchResult fetchAllTickers() override;
    TickerFetchResult fetchTickersByCategory(const std::string& category) override;
    TickerFetchResult fetchTickerBySymbol(const std::string& symbol) override;
    
    /**
     * @brief Fetch active markets only
     * @return Result containing only active/open markets
     */
    TickerFetchResult fetchActiveMarkets();
    
    /**
     * @brief Fetch markets with minimum liquidity
     * @param minLiquidity Minimum liquidity threshold
     * @return Result containing filtered markets
     */
    TickerFetchResult fetchMarketsWithMinLiquidity(double minLiquidity);
};

} // namespace ticker_service
