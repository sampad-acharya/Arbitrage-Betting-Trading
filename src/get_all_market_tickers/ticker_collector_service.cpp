#include "../include/get_all_market_tickers/ticker_collector_service.hpp"
#include <algorithm>
#include <future>

namespace ticker_service {

void TickerCollectorService::addExchange(std::unique_ptr<IFullExchange> exchange) {
    if (!exchange) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string name = exchange->getName();
    exchanges_[name] = std::move(exchange);
}

bool TickerCollectorService::removeExchange(const std::string& exchangeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = exchanges_.find(exchangeName);
    if (it == exchanges_.end()) {
        return false;
    }
    
    // Disconnect before removing
    if (it->second->isConnected()) {
        it->second->disconnect();
    }
    
    exchanges_.erase(it);
    tickerCache_.erase(exchangeName);
    return true;
}

IFullExchange* TickerCollectorService::getExchange(const std::string& exchangeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = exchanges_.find(exchangeName);
    if (it == exchanges_.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::vector<std::string> TickerCollectorService::getExchangeNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> names;
    names.reserve(exchanges_.size());
    for (const auto& [name, _] : exchanges_) {
        names.push_back(name);
    }
    return names;
}

std::unordered_map<std::string, bool> TickerCollectorService::connectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, bool> results;
    for (auto& [name, exchange] : exchanges_) {
        results[name] = exchange->connect();
    }
    return results;
}

void TickerCollectorService::disconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [name, exchange] : exchanges_) {
        if (exchange->isConnected()) {
            exchange->disconnect();
        }
    }
}

std::unordered_map<std::string, TickerFetchResult> TickerCollectorService::fetchAllTickers() {
    std::unordered_map<std::string, TickerFetchResult> results;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [name, exchange] : exchanges_) {
        if (!exchange->isConnected()) {
            results[name] = TickerFetchResult::Failure("Exchange not connected");
            continue;
        }
        
        auto result = exchange->fetchAllTickers();
        
        // Update cache
        if (result.success) {
            tickerCache_[name] = result.tickers;
            
            if (onTickerUpdate_) {
                onTickerUpdate_(name, result.tickers);
            }
        } else {
            if (onError_) {
                onError_(name, result.errorMessage);
            }
        }
        
        results[name] = std::move(result);
    }
    
    return results;
}

TickerFetchResult TickerCollectorService::fetchTickersFromExchange(const std::string& exchangeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = exchanges_.find(exchangeName);
    if (it == exchanges_.end()) {
        return TickerFetchResult::Failure("Exchange not found: " + exchangeName);
    }
    
    if (!it->second->isConnected()) {
        return TickerFetchResult::Failure("Exchange not connected: " + exchangeName);
    }
    
    auto result = it->second->fetchAllTickers();
    
    if (result.success) {
        tickerCache_[exchangeName] = result.tickers;
        
        if (onTickerUpdate_) {
            onTickerUpdate_(exchangeName, result.tickers);
        }
    } else {
        if (onError_) {
            onError_(exchangeName, result.errorMessage);
        }
    }
    
    return result;
}

std::future<std::unordered_map<std::string, TickerFetchResult>> 
TickerCollectorService::fetchAllTickersAsync() {
    return std::async(std::launch::async, [this]() {
        std::unordered_map<std::string, TickerFetchResult> results;
        std::vector<std::future<std::pair<std::string, TickerFetchResult>>> futures;
        
        // Get list of exchanges (with lock)
        std::vector<std::pair<std::string, IFullExchange*>> exchangeList;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [name, exchange] : exchanges_) {
                exchangeList.emplace_back(name, exchange.get());
            }
        }
        
        // Launch async fetch for each exchange
        for (auto& [name, exchange] : exchangeList) {
            if (!exchange->isConnected()) {
                results[name] = TickerFetchResult::Failure("Exchange not connected");
                continue;
            }
            
            futures.push_back(std::async(std::launch::async, 
                [name, exchange]() -> std::pair<std::string, TickerFetchResult> {
                    return {name, exchange->fetchAllTickers()};
                }));
        }
        
        // Collect results
        for (auto& future : futures) {
            auto [name, result] = future.get();
            
            // Update cache with lock
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (result.success) {
                    tickerCache_[name] = result.tickers;
                    if (onTickerUpdate_) {
                        onTickerUpdate_(name, result.tickers);
                    }
                } else {
                    if (onError_) {
                        onError_(name, result.errorMessage);
                    }
                }
            }
            
            results[name] = std::move(result);
        }
        
        return results;
    });
}

std::unordered_map<std::string, TickerFetchResult> 
TickerCollectorService::fetchTickersByCategory(const std::string& category) {
    std::unordered_map<std::string, TickerFetchResult> results;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [name, exchange] : exchanges_) {
        if (!exchange->isConnected()) {
            results[name] = TickerFetchResult::Failure("Exchange not connected");
            continue;
        }
        
        results[name] = exchange->fetchTickersByCategory(category);
    }
    
    return results;
}

std::vector<Ticker> TickerCollectorService::searchTicker(const std::string& symbol) {
    std::vector<Ticker> matches;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [name, exchange] : exchanges_) {
        if (!exchange->isConnected()) {
            continue;
        }
        
        auto result = exchange->fetchTickerBySymbol(symbol);
        if (result.success) {
            matches.insert(matches.end(), result.tickers.begin(), result.tickers.end());
        }
    }
    
    return matches;
}

std::vector<Ticker> TickerCollectorService::getCachedTickers(const std::string& exchangeName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tickerCache_.find(exchangeName);
    if (it == tickerCache_.end()) {
        return {};
    }
    return it->second;
}

std::vector<Ticker> TickerCollectorService::getAllCachedTickers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Ticker> allTickers;
    for (const auto& [name, tickers] : tickerCache_) {
        allTickers.insert(allTickers.end(), tickers.begin(), tickers.end());
    }
    return allTickers;
}

void TickerCollectorService::setOnTickerUpdate(TickerUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    onTickerUpdate_ = std::move(callback);
}

void TickerCollectorService::setOnError(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    onError_ = std::move(callback);
}

void TickerCollectorService::clearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    tickerCache_.clear();
}

std::unordered_map<std::string, size_t> TickerCollectorService::getCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, size_t> stats;
    for (const auto& [name, tickers] : tickerCache_) {
        stats[name] = tickers.size();
    }
    return stats;
}

} // namespace ticker_service
