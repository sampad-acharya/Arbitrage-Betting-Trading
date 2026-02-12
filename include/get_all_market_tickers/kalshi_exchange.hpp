#pragma once

#include <string>
#include <vector>
#include <memory>
#include "base_exchange.hpp"
#include "exchange_config.hpp"

namespace ticker_service {

/**
 * @brief Kalshi exchange implementation
 * Following Liskov Substitution Principle - can be used wherever IFullExchange is expected
 * Following Single Responsibility - handles only Kalshi-specific logic
 */
class KalshiExchange : public BaseExchange {
private:
    static constexpr const char* EXCHANGE_NAME = "KALSHI";
    static constexpr const char* MARKETS_ENDPOINT = "/markets";
    static constexpr const char* EVENTS_ENDPOINT = "/events";
    
    std::string authToken_;
    
    /**
     * @brief Authenticate with Kalshi API and get token
     * @return true if authentication successful
     */
    bool authenticate();
    
    /**
     * @brief Refresh authentication token if needed
     */
    void refreshTokenIfNeeded();
    
protected:
    std::vector<Ticker> parseTickersFromResponse(const std::string& jsonResponse) override;
    void addAuthHeaders(std::map<std::string, std::string>& headers) override;
    
public:
    explicit KalshiExchange(const KalshiConfig& config);
    ~KalshiExchange() override = default;
    
    std::string getName() const override { return EXCHANGE_NAME; }
    
    bool connect() override;
    void disconnect() override;
    
    TickerFetchResult fetchAllTickers() override;
    TickerFetchResult fetchTickersByCategory(const std::string& category) override;
    TickerFetchResult fetchTickerBySymbol(const std::string& symbol) override;
    
    /**
     * @brief Fetch all events (groupings of related markets)
     * @return Vector of event identifiers
     */
    std::vector<std::string> fetchAllEvents();
    
    /**
     * @brief Fetch markets for a specific event
     * @param eventTicker The event ticker to fetch markets for
     * @return Result containing markets for the event
     */
    TickerFetchResult fetchMarketsForEvent(const std::string& eventTicker);
};

} // namespace ticker_service
