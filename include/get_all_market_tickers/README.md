Code base structure:
--------------------

Ticker - Data structure for ticker information
IExchange - Abstract interface for exchanges (Interface Segregation + Dependency Inversion)
Concrete Exchange classes - KalshiExchange, PolymarketExchange, CMEExchange (Open/Closed)
ITickerFetcher - Interface for fetching tickers (Single Responsibility)
TickerCollectorService - Service that orchestrates collection (Single Responsibility)
ExchangeFactory - Factory to create exchanges (Open/Closed)


How to run it?
--------------
mkdir build && cd build
cmake .. -DUSE_CURL=ON  # Enable libcurl for HTTP
make