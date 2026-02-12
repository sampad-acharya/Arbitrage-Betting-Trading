#pragma once

/**
 * @file ticker_service.hpp
 * @brief Main header file for the Ticker Collection Service
 * 
 * This service provides a unified interface for collecting market tickers
 * from multiple exchanges including:
 * - Kalshi (prediction markets)
 * - Polymarket (prediction markets)  
 * - CME Group (futures and options)
 * 
 * Architecture follows SOLID principles:
 * - Single Responsibility: Each class has one clear purpose
 * - Open/Closed: Easy to add new exchanges without modifying existing code
 * - Liskov Substitution: All exchanges can be used interchangeably via IFullExchange
 * - Interface Segregation: Separate interfaces for connection and fetching
 * - Dependency Inversion: High-level modules depend on abstractions (IExchange, ITickerFetcher)
 * 
 * Usage Example:
 * @code
 * #include "ticker_service.hpp"
 * 
 * using namespace ticker_service;
 * 
 * int main() {
 *     // Create the collector service
 *     TickerCollectorService service;
 *     
 *     // Add exchanges
 *     auto& factory = ExchangeFactory::getInstance();
 *     
 *     KalshiConfig kalshiConfig("api_key", "api_secret", true);
 *     service.addExchange(factory.createKalshiExchange(kalshiConfig));
 *     
 *     PolymarketConfig polyConfig("api_key", "api_secret");
 *     service.addExchange(factory.createPolymarketExchange(polyConfig));
 *     
 *     // Connect to all exchanges
 *     service.connectAll();
 *     
 *     // Fetch all tickers
 *     auto results = service.fetchAllTickers();
 *     
 *     for (const auto& [exchange, result] : results) {
 *         if (result.success) {
 *             std::cout << exchange << ": " << result.tickers.size() << " tickers\n";
 *         }
 *     }
 *     
 *     return 0;
 * }
 * @endcode
 */

// Core data structures
#include "ticker.hpp"
#include "exchange_config.hpp"

// Interfaces
#include "i_exchange.hpp"

// Base implementations
#include "base_exchange.hpp"

// Exchange implementations
#include "kalshi_exchange.hpp"
#include "polymarket_exchange.hpp"
#include "cme_exchange.hpp"

// Factory and service
#include "exchange_factory.hpp"
#include "ticker_collector_service.hpp"
