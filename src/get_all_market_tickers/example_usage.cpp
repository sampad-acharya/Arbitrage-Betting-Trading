/**
 * @file example_usage.cpp
 * @brief Example usage of the Ticker Collection Service
 * 
 * This file demonstrates how to use the ticker service to collect
 * market tickers from multiple exchanges.
 */

#include <iostream>
#include <iomanip>
#include "../include/get_all_market_tickers/ticker_service.hpp"

using namespace ticker_service;

void printTicker(const Ticker& ticker) {
    std::cout << std::setw(20) << ticker.symbol 
              << std::setw(15) << ticker.exchange
              << std::setw(10) << std::fixed << std::setprecision(4) << ticker.lastPrice
              << std::setw(10) << ticker.bidPrice
              << std::setw(10) << ticker.askPrice
              << std::setw(15) << ticker.volume24h
              << std::endl;
}

void printHeader() {
    std::cout << std::setw(20) << "Symbol"
              << std::setw(15) << "Exchange"
              << std::setw(10) << "Last"
              << std::setw(10) << "Bid"
              << std::setw(10) << "Ask"
              << std::setw(15) << "Volume"
              << std::endl;
    std::cout << std::string(80, '-') << std::endl;
}

int main() {
    std::cout << "=== Ticker Collection Service Demo ===" << std::endl << std::endl;
    
    // Create the collector service
    TickerCollectorService service;
    
    // Get the exchange factory
    auto& factory = ExchangeFactory::getInstance();
    
    // Configure and add Kalshi exchange
    {
        KalshiConfig config;
        config.apiKey = "your_kalshi_api_key";
        config.apiSecret = "your_kalshi_api_secret";
        config.useSandbox = true;  // Use sandbox for testing
        
        auto exchange = factory.createKalshiExchange(config);
        service.addExchange(std::move(exchange));
        std::cout << "Added Kalshi exchange" << std::endl;
    }
    
    // Configure and add Polymarket exchange
    {
        PolymarketConfig config;
        config.apiKey = "your_polymarket_api_key";
        config.apiSecret = "your_polymarket_api_secret";
        
        auto exchange = factory.createPolymarketExchange(config);
        service.addExchange(std::move(exchange));
        std::cout << "Added Polymarket exchange" << std::endl;
    }
    
    // Configure and add CME exchange
    {
        CMEConfig config;
        config.apiKey = "your_cme_api_key";
        config.apiSecret = "your_cme_api_secret";
        config.firmId = "your_firm_id";
        config.traderId = "your_trader_id";
        config.useSandbox = true;
        
        auto exchange = factory.createCMEExchange(config);
        service.addExchange(std::move(exchange));
        std::cout << "Added CME exchange" << std::endl;
    }
    
    // Set up callbacks
    service.setOnTickerUpdate([](const std::string& exchange, const std::vector<Ticker>& tickers) {
        std::cout << "\n[UPDATE] " << exchange << ": Received " << tickers.size() << " tickers" << std::endl;
    });
    
    service.setOnError([](const std::string& exchange, const std::string& error) {
        std::cerr << "\n[ERROR] " << exchange << ": " << error << std::endl;
    });
    
    // Connect to all exchanges
    std::cout << "\nConnecting to exchanges..." << std::endl;
    auto connectionResults = service.connectAll();
    
    for (const auto& [exchange, success] : connectionResults) {
        std::cout << "  " << exchange << ": " << (success ? "Connected" : "Failed") << std::endl;
    }
    
    // Fetch all tickers synchronously
    std::cout << "\nFetching tickers from all exchanges..." << std::endl;
    auto results = service.fetchAllTickers();
    
    // Print results
    for (const auto& [exchange, result] : results) {
        std::cout << "\n" << exchange << ":" << std::endl;
        if (result.success) {
            std::cout << "  Fetched " << result.tickers.size() << " tickers" << std::endl;
            
            // Print first 5 tickers
            printHeader();
            int count = 0;
            for (const auto& ticker : result.tickers) {
                if (count++ >= 5) break;
                printTicker(ticker);
            }
        } else {
            std::cout << "  Error: " << result.errorMessage << std::endl;
        }
    }
    
    // Demonstrate async fetching
    std::cout << "\n\nFetching tickers asynchronously..." << std::endl;
    auto futureResults = service.fetchAllTickersAsync();
    
    // Do other work while waiting...
    std::cout << "Doing other work while fetching..." << std::endl;
    
    // Get async results
    auto asyncResults = futureResults.get();
    std::cout << "Async fetch complete!" << std::endl;
    
    // Print cache statistics
    std::cout << "\nCache Statistics:" << std::endl;
    auto stats = service.getCacheStats();
    for (const auto& [exchange, count] : stats) {
        std::cout << "  " << exchange << ": " << count << " tickers cached" << std::endl;
    }
    
    // Demonstrate searching for a ticker
    std::cout << "\nSearching for ticker 'BTC'..." << std::endl;
    auto matches = service.searchTicker("BTC");
    std::cout << "Found " << matches.size() << " matches" << std::endl;
    
    // Disconnect from all exchanges
    std::cout << "\nDisconnecting from all exchanges..." << std::endl;
    service.disconnectAll();
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    
    return 0;
}
