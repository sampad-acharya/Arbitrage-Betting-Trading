#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include <iomanip>
#include <algorithm> 
#include "../include/order_book.hpp"


void OrderBook::addOrder(const Order &order){
    auto &type = order.type;
    if (type == OrderType::BUY){
        auto &book = buyOrder;
        auto &queue = buyOrder[order.price];
        queue.push_back(order);
    }else if (type == OrderType::SELL){
        auto &book = sellOrder;
        auto &queue = sellOrder[order.price];
        queue.push_back(order);
    };
};

void OrderBook::removeOrder(const std::string &id, OrderType type, double price){
    if (type == OrderType::BUY){
        auto &book = buyOrder;
        auto &queue = buyOrder[price];
        queue.erase(std::remove_if(queue.begin(), queue.end(), [&](const Order& o) { return o.orderId == id; }), queue.end());
        if (queue.empty()) book.erase(price);
    }else if (type == OrderType::SELL){
        auto &book = sellOrder;
        auto &queue = sellOrder[price];
        queue.erase(std::remove_if(queue.begin(), queue.end(), [&](const Order& o) { return o.orderId == id; }), queue.end());
        if (queue.empty()) book.erase(price);
        };
};

void OrderBook::printOutput(Order &aggressiveOrder, Order &restingOrder, int traderQuantity){

    std::cout << "2," << traderQuantity << "," << restingOrder.price << "\n";

    if (aggressiveOrder.quantity == 0)
        std::cout << "3," << aggressiveOrder.orderId << "\n";
    else
        std::cout << "4," << aggressiveOrder.orderId << "," << aggressiveOrder.quantity << "\n";

    if (restingOrder.quantity == 0) {
        std::cout << "3," << restingOrder.orderId << "\n";
    } else {
        std::cout << "4," << restingOrder.orderId << "," << restingOrder.quantity << "\n";
    }

}

void OrderBook::matchOrder(Order &aggressiveOrder, std::unordered_map<std::string, Order> &orderMap){

    if (aggressiveOrder.type == OrderType::BUY){
        auto &book = sellOrder;
        auto it = book.begin();
        while (it != book.end() && aggressiveOrder.quantity > 0){
            if (aggressiveOrder.price >= it->first && aggressiveOrder.quantity > 0){
                auto &currentPriceQueue = it->second;
                while (!currentPriceQueue.empty() && aggressiveOrder.quantity > 0){
                    Order &currentOrderInQueue = currentPriceQueue.front();
                    int targetMatch = std::min(aggressiveOrder.quantity, currentOrderInQueue.quantity);

                    aggressiveOrder.quantity -= targetMatch;
                    currentOrderInQueue.quantity -= targetMatch;

                    printOutput(aggressiveOrder, currentOrderInQueue, targetMatch);
                    if (!currentOrderInQueue.quantity){
                        orderMap.erase(currentOrderInQueue.orderId);
                        currentPriceQueue.pop_front();
                    }else{
                        Order &order = orderMap[currentOrderInQueue.orderId];
                        order.quantity = currentOrderInQueue.quantity;
                    }     
                }
                if (currentPriceQueue.empty()) it = book.erase(it);
                else ++it;
            }else break;
        }
    }else if (aggressiveOrder.type == OrderType::SELL){
        auto &book = buyOrder;
        auto it = book.begin();
        while (it != book.end() && aggressiveOrder.quantity > 0){
            if (aggressiveOrder.price <= it->first && aggressiveOrder.quantity > 0){
                auto &currentPriceQueue = it->second;
                while (!currentPriceQueue.empty() && aggressiveOrder.quantity > 0){
                    Order &currentOrderInQueue = currentPriceQueue.front();
                    int targetMatch = std::min(aggressiveOrder.quantity, currentOrderInQueue.quantity);

                    aggressiveOrder.quantity -= targetMatch;
                    currentOrderInQueue.quantity -= targetMatch;

                    printOutput(aggressiveOrder, currentOrderInQueue, targetMatch);

                    if (!currentOrderInQueue.quantity){
                        orderMap.erase(currentOrderInQueue.orderId);
                        currentPriceQueue.pop_front();
                    }else{
                        Order &order = orderMap[currentOrderInQueue.orderId];
                        order.quantity = currentOrderInQueue.quantity;
                    }     
                }
                if (currentPriceQueue.empty()) it = book.erase(it);
                else ++it;
            }else break;
        }
    }
};

