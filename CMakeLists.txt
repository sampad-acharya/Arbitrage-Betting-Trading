# CMakeLists.txt for Ticker Collection Service
cmake_minimum_required(VERSION 3.14)
project(ticker_service VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Option to build with curl support
option(USE_CURL "Build with libcurl for HTTP requests" OFF)

# Find dependencies
if(USE_CURL)
    find_package(CURL REQUIRED)
endif()

# Define source files
set(TICKER_SERVICE_SOURCES
    src/get_all_market_tickers/base_exchange.cpp
    src/get_all_market_tickers/kalshi_exchange.cpp
    src/get_all_market_tickers/polymarket_exchange.cpp
    src/get_all_market_tickers/cme_exchange.cpp
    src/get_all_market_tickers/ticker_collector_service.cpp
)

# Define header files
set(TICKER_SERVICE_HEADERS
    include/get_all_market_tickers/ticker.hpp
    include/get_all_market_tickers/exchange_config.hpp
    include/get_all_market_tickers/i_exchange.hpp
    include/get_all_market_tickers/base_exchange.hpp
    include/get_all_market_tickers/kalshi_exchange.hpp
    include/get_all_market_tickers/polymarket_exchange.hpp
    include/get_all_market_tickers/cme_exchange.hpp
    include/get_all_market_tickers/exchange_factory.hpp
    include/get_all_market_tickers/ticker_collector_service.hpp
    include/get_all_market_tickers/ticker_service.hpp
)

# Create the library
add_library(ticker_service STATIC
    ${TICKER_SERVICE_SOURCES}
    ${TICKER_SERVICE_HEADERS}
)

target_include_directories(ticker_service
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(USE_CURL)
    target_link_libraries(ticker_service PUBLIC CURL::libcurl)
    target_compile_definitions(ticker_service PUBLIC USE_CURL)
endif()

# Threading support
find_package(Threads REQUIRED)
target_link_libraries(ticker_service PUBLIC Threads::Threads)

# Example executable
add_executable(ticker_service_example
    src/get_all_market_tickers/example_usage.cpp
)

target_link_libraries(ticker_service_example PRIVATE ticker_service)

# Existing matching engine library (if needed)
set(MATCHING_ENGINE_SOURCES
    src/main.cpp
    src/matching_engine.cpp
    src/order_book.cpp
    src/order.cpp
)

# Install rules
install(TARGETS ticker_service
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/get_all_market_tickers
    DESTINATION include
)

# Optional: Enable testing
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    # Add test executable here
endif()
