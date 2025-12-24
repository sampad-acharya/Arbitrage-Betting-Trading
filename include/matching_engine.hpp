#pragma once

#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include <cstdint>
#include "../include/order_book.hpp"
#include  "../include/order.hpp"

class MatchingEngine{
    OrderBook book;
    std::unordered_map<std::string, Order> orderMap;
    int timestamp; 

public:
    MatchingEngine() = default;
    void processOrder(std::string &line);
    OrderType getOrderType(std::string &type);

};