#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include "../include/matching_engine.hpp"


OrderType MatchingEngine::getOrderType(std::string &type){
    return (type == "0") ? OrderType::BUY : OrderType::SELL; 
};

void MatchingEngine::processOrder(std::string &line){
    int timestamp{};
    std::istringstream ss(line);

    std::string token;
    char delimiter{','};
    std::vector<std::string> tokens{};

    while (std::getline(ss, token, delimiter)){
        tokens.push_back(token);
    }

    std::string msgtype, side, orderId;
    double price;
    int quantity;
    if (tokens.empty()){
        std::cout << "An erroneous input value: " << line << std::endl;
        return;
    }
    msgtype = tokens[0];

    if (msgtype == "0"){
        if (tokens.size() == 5){
            orderId = tokens[1];
            side = tokens[2];
            quantity = stoi(tokens[3]);
            price = stod(tokens[4]);

            if (price <= 0 || quantity <= 0 || orderId.empty() || (side != "0" && side != "1")){
                std::cout << "An erroneous input value: " << line << std::endl;
                return;
            };

            OrderType type = getOrderType(side);
            Order order{orderId, type, price, quantity, timestamp++ };
            book.matchOrder(order, orderMap);

            if (order.quantity > 0){
                book.addOrder(order);
                orderMap[orderId] = order;
            }
        }else{
            std::cout << "An erroneous input: " << line << std::endl;
            return; 
        }
    }else if (msgtype == "1"){
        if (tokens.size() == 2){
            //call removeOrder
            orderId = tokens[1];

            if (orderMap.count(orderId) > 0){
                Order &order = orderMap[orderId];
                book.removeOrder(order.orderId, order.type, order.price);
                orderMap.erase(orderId);
            }
        }else{
            std::cout << "An erroneous input: " << line << std::endl;
            return; 
        }
    }else{
        std::cout << "An erroneous input: " << line << std::endl;
        return;
    }
};




