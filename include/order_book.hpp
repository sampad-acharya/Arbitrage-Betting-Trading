#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include <cstdint>
#include "order.hpp"


class OrderBook{
    std::map<double , std::deque<Order>, std::greater<>> buyOrder;
    std::map<double , std::deque<Order>> sellOrder;

public:
    void addOrder(const Order &order);

    void removeOrder(const std::string &id, OrderType type, double price);

    void matchOrder(Order &aggressiveOrder, std::unordered_map<std::string, Order> &orderMap);

    void printOutput(Order &aggressiveOrder, Order &restingOrder, int traderQuantity);

};