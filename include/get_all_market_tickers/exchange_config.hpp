#pragma once

#include <string>
#include <chrono>
#include <optional>

namespace ticker_service {

/**
 * @brief Configuration for exchange connections
 * Following Single Responsibility - only holds configuration data
 */
struct ExchangeConfig {
    std::string apiKey;
    std::string apiSecret;
    std::string baseUrl;
    std::chrono::milliseconds timeout{5000};
    int maxRetries{3};
    bool useSandbox{false};
    
    ExchangeConfig() = default;
    
    ExchangeConfig(const std::string& apiKey_,
                   const std::string& apiSecret_,
                   const std::string& baseUrl_)
        : apiKey(apiKey_)
        , apiSecret(apiSecret_)
        , baseUrl(baseUrl_) {}
};

/**
 * @brief Kalshi-specific configuration
 */
struct KalshiConfig : public ExchangeConfig {
    static constexpr const char* PRODUCTION_URL = "https://trading-api.kalshi.com/trade-api/v2";
    static constexpr const char* SANDBOX_URL = "https://demo-api.kalshi.co/trade-api/v2";
    
    KalshiConfig() {
        baseUrl = PRODUCTION_URL;
    }
    
    KalshiConfig(const std::string& apiKey_, const std::string& apiSecret_, bool sandbox = false) 
        : ExchangeConfig(apiKey_, apiSecret_, sandbox ? SANDBOX_URL : PRODUCTION_URL) {
        useSandbox = sandbox;
    }
};

/**
 * @brief Polymarket-specific configuration
 */
struct PolymarketConfig : public ExchangeConfig {
    static constexpr const char* PRODUCTION_URL = "https://clob.polymarket.com";
    static constexpr const char* GAMMA_API_URL = "https://gamma-api.polymarket.com";
    
    std::string gammaApiUrl{GAMMA_API_URL};
    
    PolymarketConfig() {
        baseUrl = PRODUCTION_URL;
    }
    
    PolymarketConfig(const std::string& apiKey_, const std::string& apiSecret_)
        : ExchangeConfig(apiKey_, apiSecret_, PRODUCTION_URL) {}
};

/**
 * @brief CME-specific configuration
 */
struct CMEConfig : public ExchangeConfig {
    static constexpr const char* PRODUCTION_URL = "https://api.cmegroup.com";
    static constexpr const char* SANDBOX_URL = "https://api-sandbox.cmegroup.com";
    
    std::string firmId;
    std::string traderId;
    
    CMEConfig() {
        baseUrl = PRODUCTION_URL;
    }
    
    CMEConfig(const std::string& apiKey_, 
              const std::string& apiSecret_,
              const std::string& firmId_,
              const std::string& traderId_,
              bool sandbox = false)
        : ExchangeConfig(apiKey_, apiSecret_, sandbox ? SANDBOX_URL : PRODUCTION_URL)
        , firmId(firmId_)
        , traderId(traderId_) {
        useSandbox = sandbox;
    }
};

} // namespace ticker_service
