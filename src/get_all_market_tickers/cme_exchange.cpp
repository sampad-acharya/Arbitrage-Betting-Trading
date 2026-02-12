#include "../include/get_all_market_tickers/cme_exchange.hpp"
#include <sstream>
#include <algorithm>

namespace ticker_service {

CMEExchange::CMEExchange(const CMEConfig& config) 
    : BaseExchange(config)
    , cmeConfig_(config) {}

bool CMEExchange::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // CME uses OAuth2 or API key authentication
    // Implement connection verification here
    connected_ = true;
    return true;
}

std::string CMEExchange::venueToString(CMEVenue venue) {
    switch (venue) {
        case CMEVenue::CME:   return "CME";
        case CMEVenue::CBOT:  return "CBOT";
        case CMEVenue::NYMEX: return "NYMEX";
        case CMEVenue::COMEX: return "COMEX";
        case CMEVenue::ALL:   return "";
        default:              return "";
    }
}

std::string CMEExchange::productTypeToString(ProductType type) {
    switch (type) {
        case ProductType::FUTURES: return "FUT";
        case ProductType::OPTIONS: return "OPT";
        case ProductType::SPREADS: return "SPR";
        case ProductType::ALL:     return "";
        default:                   return "";
    }
}

void CMEExchange::addAuthHeaders(std::map<std::string, std::string>& headers) {
    // CME uses specific authentication headers
    headers["X-CME-API-Key"] = config_.apiKey;
    headers["X-CME-Firm-ID"] = cmeConfig_.firmId;
    headers["X-CME-Trader-ID"] = cmeConfig_.traderId;
    headers["Content-Type"] = "application/json";
}

std::vector<Ticker> CMEExchange::parseTickersFromResponse(const std::string& jsonResponse) {
    std::vector<Ticker> tickers;
    
    /*
     * CME products response format varies by endpoint
     * Example structure:
     * {
     *   "products": [
     *     {
     *       "productId": "ES",
     *       "productName": "E-mini S&P 500 Futures",
     *       "productType": "FUT",
     *       "exchange": "CME",
     *       "sector": "Equity Index",
     *       "contractSpecs": {
     *         "tickSize": 0.25,
     *         "tickValue": 12.50,
     *         "contractSize": 50
     *       },
     *       "quotes": {
     *         "lastPrice": 4500.25,
     *         "bidPrice": 4500.00,
     *         "askPrice": 4500.50,
     *         "volume": 1250000,
     *         "openInterest": 2500000
     *       },
     *       "expirations": [
     *         {"contract": "ESH4", "expiry": "2024-03-15"},
     *         {"contract": "ESM4", "expiry": "2024-06-21"}
     *       ]
     *     }
     *   ]
     * }
     * 
     * Example parsing:
     * 
     * auto json = nlohmann::json::parse(jsonResponse);
     * for (const auto& product : json["products"]) {
     *     for (const auto& expiry : product["expirations"]) {
     *         Ticker ticker;
     *         ticker.symbol = expiry["contract"].get<std::string>();
     *         ticker.name = product["productName"].get<std::string>();
     *         ticker.exchange = getName();
     *         ticker.category = product["sector"].get<std::string>();
     *         
     *         if (product.contains("quotes")) {
     *             ticker.lastPrice = product["quotes"]["lastPrice"].get<double>();
     *             ticker.bidPrice = product["quotes"]["bidPrice"].get<double>();
     *             ticker.askPrice = product["quotes"]["askPrice"].get<double>();
     *             ticker.volume24h = product["quotes"]["volume"].get<double>();
     *         }
     *         
     *         ticker.isActive = true;
     *         tickers.push_back(ticker);
     *     }
     * }
     */
    
    return tickers;
}

TickerFetchResult CMEExchange::fetchAllTickers() {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to CME");
    }
    
    try {
        std::string response = makeGetRequest(PRODUCTS_ENDPOINT);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch CME tickers: ") + e.what());
    }
}

TickerFetchResult CMEExchange::fetchTickersByCategory(const std::string& category) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to CME");
    }
    
    try {
        std::string endpoint = std::string(PRODUCTS_ENDPOINT) + "?sector=" + category;
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch CME tickers by category: ") + e.what());
    }
}

TickerFetchResult CMEExchange::fetchTickerBySymbol(const std::string& symbol) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to CME");
    }
    
    try {
        std::string endpoint = std::string(PRODUCTS_ENDPOINT) + "/" + symbol;
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch CME ticker: ") + e.what());
    }
}

TickerFetchResult CMEExchange::fetchTickersByVenue(CMEVenue venue) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to CME");
    }
    
    try {
        std::string venueStr = venueToString(venue);
        std::string endpoint = std::string(PRODUCTS_ENDPOINT);
        if (!venueStr.empty()) {
            endpoint += "?exchange=" + venueStr;
        }
        
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch CME tickers by venue: ") + e.what());
    }
}

TickerFetchResult CMEExchange::fetchTickersByProductType(ProductType type) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to CME");
    }
    
    try {
        std::string typeStr = productTypeToString(type);
        std::string endpoint = std::string(PRODUCTS_ENDPOINT);
        if (!typeStr.empty()) {
            endpoint += "?productType=" + typeStr;
        }
        
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch CME tickers by product type: ") + e.what());
    }
}

TickerFetchResult CMEExchange::fetchTickersByAssetClass(const std::string& assetClass) {
    if (!isConnected()) {
        return TickerFetchResult::Failure("Not connected to CME");
    }
    
    try {
        std::string endpoint = std::string(PRODUCTS_ENDPOINT) + "?assetClass=" + assetClass;
        std::string response = makeGetRequest(endpoint);
        auto tickers = parseTickersFromResponse(response);
        
        return TickerFetchResult::Success(std::move(tickers));
        
    } catch (const std::exception& e) {
        return TickerFetchResult::Failure(std::string("Failed to fetch CME tickers by asset class: ") + e.what());
    }
}

} // namespace ticker_service
