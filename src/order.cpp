#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include "../include/order.hpp"

Order::Order(std::string &id_, OrderType type_, double price_, int quantity_, int timestamp_)
    : orderId(id_), type(type_), price(price_), quantity(quantity_), timestamp(timestamp_) {};

