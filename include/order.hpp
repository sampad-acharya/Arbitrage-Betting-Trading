#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include <cstdint>



enum class OrderType { BUY, SELL };

struct Order{
    std::string orderId;
    OrderType type;
    double price;
    int quantity;
    //We do not really need the timestamp here but keeping it here for future, 
    //in case, we want to store order timestamp
    int timestamp; 


    Order() = default; 
    Order(std::string &id_, OrderType type_, double price_, int quantity_, int timestamp_);
};
