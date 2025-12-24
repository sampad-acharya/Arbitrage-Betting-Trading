#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iomanip>
#include "../include/matching_engine.hpp"


int main() {
    std::cout << std::fixed << std::setprecision(2);
    MatchingEngine engine;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line == "END") break;
        std::istringstream ss(line);
        engine.processOrder(line);
    }
    return 0;
} 