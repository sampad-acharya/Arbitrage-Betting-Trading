#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include "i_exchange.hpp"
#include "exchange_config.hpp"
#include "kalshi_exchange.hpp"
#include "polymarket_exchange.hpp"
#include "cme_exchange.hpp"

namespace ticker_service {

/**
 * @brief Enum representing supported exchanges
 */
enum class ExchangeType {
    KALSHI,
    POLYMARKET,
    CME
};

/**
 * @brief Factory for creating exchange instances
 * Following Open/Closed Principle - new exchanges can be registered without modifying factory
 * Following Dependency Inversion - factory creates abstractions, not concretions
 */
class ExchangeFactory {
public:
    using ExchangeCreator = std::function<std::unique_ptr<IFullExchange>(const ExchangeConfig&)>;
    
private:
    std::unordered_map<std::string, ExchangeCreator> creators_;
    
    // Private constructor for singleton
    ExchangeFactory() {
        registerDefaultExchanges();
    }
    
    void registerDefaultExchanges() {
        // Register Kalshi
        registerExchange("KALSHI", [](const ExchangeConfig& config) {
            KalshiConfig kalshiConfig;
            kalshiConfig.apiKey = config.apiKey;
            kalshiConfig.apiSecret = config.apiSecret;
            kalshiConfig.useSandbox = config.useSandbox;
            kalshiConfig.baseUrl = config.useSandbox ? KalshiConfig::SANDBOX_URL : KalshiConfig::PRODUCTION_URL;
            return std::make_unique<KalshiExchange>(kalshiConfig);
        });
        
        // Register Polymarket
        registerExchange("POLYMARKET", [](const ExchangeConfig& config) {
            PolymarketConfig polyConfig;
            polyConfig.apiKey = config.apiKey;
            polyConfig.apiSecret = config.apiSecret;
            return std::make_unique<PolymarketExchange>(polyConfig);
        });
        
        // Register CME
        registerExchange("CME", [](const ExchangeConfig& config) {
            CMEConfig cmeConfig;
            cmeConfig.apiKey = config.apiKey;
            cmeConfig.apiSecret = config.apiSecret;
            return std::make_unique<CMEExchange>(cmeConfig);
        });
    }
    
public:
    // Singleton access
    static ExchangeFactory& getInstance() {
        static ExchangeFactory instance;
        return instance;
    }
    
    // Delete copy/move constructors
    ExchangeFactory(const ExchangeFactory&) = delete;
    ExchangeFactory& operator=(const ExchangeFactory&) = delete;
    ExchangeFactory(ExchangeFactory&&) = delete;
    ExchangeFactory& operator=(ExchangeFactory&&) = delete;
    
    /**
     * @brief Register a new exchange type
     * @param exchangeName Name of the exchange
     * @param creator Factory function to create the exchange
     */
    void registerExchange(const std::string& exchangeName, ExchangeCreator creator) {
        creators_[exchangeName] = std::move(creator);
    }
    
    /**
     * @brief Create an exchange instance
     * @param exchangeName Name of the exchange to create
     * @param config Configuration for the exchange
     * @return Unique pointer to the exchange instance
     * @throws std::runtime_error if exchange type is not registered
     */
    std::unique_ptr<IFullExchange> createExchange(const std::string& exchangeName,
                                                   const ExchangeConfig& config) {
        auto it = creators_.find(exchangeName);
        if (it == creators_.end()) {
            throw std::runtime_error("Unknown exchange type: " + exchangeName);
        }
        return it->second(config);
    }
    
    /**
     * @brief Create an exchange with type-specific config
     */
    std::unique_ptr<KalshiExchange> createKalshiExchange(const KalshiConfig& config) {
        return std::make_unique<KalshiExchange>(config);
    }
    
    std::unique_ptr<PolymarketExchange> createPolymarketExchange(const PolymarketConfig& config) {
        return std::make_unique<PolymarketExchange>(config);
    }
    
    std::unique_ptr<CMEExchange> createCMEExchange(const CMEConfig& config) {
        return std::make_unique<CMEExchange>(config);
    }
    
    /**
     * @brief Check if an exchange type is registered
     * @param exchangeName Name of the exchange
     * @return true if registered
     */
    bool isExchangeRegistered(const std::string& exchangeName) const {
        return creators_.find(exchangeName) != creators_.end();
    }
    
    /**
     * @brief Get all registered exchange names
     * @return Vector of registered exchange names
     */
    std::vector<std::string> getRegisteredExchanges() const {
        std::vector<std::string> names;
        names.reserve(creators_.size());
        for (const auto& [name, _] : creators_) {
            names.push_back(name);
        }
        return names;
    }
};

} // namespace ticker_service
