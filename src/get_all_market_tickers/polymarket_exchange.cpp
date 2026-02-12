#include "../include/get_all_market_tickers/polymarket_exchange.hpp"
#include <sstream>
#include <algorithm>

namespace ticker_service {

PolymarketExchange::PolymarketExchange(const PolymarketConfig& config) 
    : BaseExchange(config)
    , polyConfig_(config) {}

bool PolymarketExchange::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Polymarket uses API keys passed in headers
    // No separate authentication step required
    connected_ = true;
    return true;
}

std::string PolymarketExchange::fetchFromGammaApi(const std::string& endpoint) {
    /*
     * The Gamma API provides market metadata and conditions
     * Use this for fetching market details, conditions, and resolutions
     * 
     * Example with libcurl:
     * std::string url = polyConfig_.gammaApiUrl + endpoint;
     * return makeGetRequest(url);
     */
    
    return "{}";
}

std::vector<Ticker> PolymarketExchange::parseTickersFromResponse(const std::string& jsonResponse) {
    std::vector<Ticker> tickers;
    
    /*
     * Polymarket CLOB API markets response format:
     * {
     *   "next_cursor": "abc123",
     *   "data": [
     *     {
     *       "condition_id": "0x...",
     *       "question": "Will X happen by Y date?",
     *       "market_slug": "will-x-happen",
     *       "tokens": [
     *         {
     *           "token_id": "12345",
     *           "outcome": "Yes",
     *           "price": 0.65,
     *           "winner": false
     *         },
     *         {
     *           "token_id": "12346", 
     *           "outcome": "No",
     *           "price": 0.35,
     *           "winner": false
     *         }
     *       ],
     *       "active": true,
     *       "closed": false,
     *       "volume": 150000.50,
     *       "liquidity": 25000.00,
     *       "end_date": "2024-12-31T23:59:59Z"
     *     }
     *   ]
     * }
     * 
     * Example parsing:
     * 
     * auto json = nlohmann::json::parse(jsonResponse);
     * for (const auto& market : json["data"]) {
     *     // Create ticker for Yes token
     *     Ticker yesTicker;
     *     yesTicker.symbol = market["tokens"][0]["token_id"].get<std::string>();
     *     yesTicker.name = market["question"].get<std::string>() + " - Yes";
     *     yesTicker.exchange = getName();
     *     yesTicker.category = "prediction";
     *     yesTicker.lastPrice = market["tokens"][0]["price"].get<double>();
     *     yesTicker.volume24h = market["volume"].get<double>();
     *     yesTicker.isActive = market["active"].get<bool>() && !market["closed"].get<bool>();
     *     tickers.push_back(yesTicker);
     *     
     *     // Create ticker for No token
     *     Ticker noTicker;
     *     noTicker.symbol = market["tokens"][1]["token_id"].get<std::string>();
     *     noTicker.name = market["question"].get<std::string>() + " - No";
     *     noTicker.exchange = getName();
     *     noTicker.lastPrice = market["tokens"][1]["price"].get<double>();
     *     noTicker.isActive = yesTicker.isActive;
     *     tickers.push_back(noTicker);
     * }
     */
    
    return tickers;
}

TickerFetchResult PolymarketExchange::fetchAllTickers() {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to Polymarket");
    }
    
    try {
        std::vector<Ticker> allTickers;
        std::string cursor;
        
        // Paginate through all markets
        do {
            std::string endpoint = std::string(MARKETS_ENDPOINT);
            if (!cursor.empty()) {
                endpoint += "?next_cursor=" + cursor;
            }
            
            std::string response = makeGetRequest(endpoint);
            auto tickers = parseTickersFromResponse(response);
            allTickers.insert(allTickers.end(), tickers.begin(), tickers.end());
            
            // Extract next cursor
            cursor.clear(); // Placeholder
            
        } while (!cursor.empty());
        
        return TickerFetchResult::Success(std::move(allTickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch Polymarket tickers: ") + e.what());
    }
}

TickerFetchResult PolymarketExchange::fetchTickersByCategory(const std::string& category) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to Polymarket");
    }
    
    try {
        // Polymarket uses tags for categorization
        std::string endpoint = std::string(MARKETS_ENDPOINT) + "?tag=" + category;
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch Polymarket tickers by category: ") + e.what());
    }
}

TickerFetchResult PolymarketExchange::fetchTickerBySymbol(const std::string& symbol) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to Polymarket");
    }
    
    try {
        // Fetch by condition_id or token_id
        std::string endpoint = std::string(TOKENS_ENDPOINT) + "/" + symbol;
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch Polymarket ticker: ") + e.what());
    }
}

TickerFetchResult PolymarketExchange::fetchActiveMarkets() {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to Polymarket");
    }
    
    try {
        std::string endpoint = std::string(MARKETS_ENDPOINT) + "?active=true&closed=false";
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch active Polymarket markets: ") + e.what());
    }
}

TickerFetchResult PolymarketExchange::fetchMarketsWithMinLiquidity(double minLiquidity) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to Polymarket");
    }
    
    try {
        // Fetch all and filter by liquidity
        auto result = fetchAllTickers();
        if (!result.success) {
            return result;
        }
        
        std::vector<Ticker> filtered;
        for (const auto& ticker : result.tickers) {
            // Assuming we stored liquidity info or can calculate from bid/ask
            // In production, you'd need to store this in the Ticker struct
            // For now, we'll use volume as a proxy
            if (ticker.volume24h >= minLiquidity) {
                filtered.push_back(ticker);
            }
        }
        
        return TickerFetchResult::Success(std::move(filtered));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch Polymarket markets with liquidity filter: ") + e.what());
    }
}

} // namespace ticker_service
