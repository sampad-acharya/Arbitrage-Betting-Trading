This is a matching engine for buy and sell orders. It contains 3 files in src/. We will need all of these 3 files to be compiled together to make Matching Engine work. 

There are 3 folders here and they are below:

1. src: It contains all the source files 
2. include: It contains all the header files
3. tests: it contains all the different test inputs

I have executed and tested on my MacBook, which has ARM processor and MacOS. I have compiled the code using C++17 and g++. 

To compile, enter the src folder and execute the command. 

Compilation command: 
g++ -std=c++17  main.cpp matching_engine.cpp order.cpp order_book.cpp -o MatchingEngine

Run examples: 

./MatchingEngine < ../tests/test_2.txt
./MatchingEngine < ../tests/test_5.txt

I have different tests to tackle corner cases, like double, int prices, bad message, negative or 0 price, empty file, only one side of orders, etc. The code can handle test cases where the orders are end with "END" keyword, but it is not shown in any of the example files, it is just an extra touch. 

Notes: 
This code is written with most optimized algorithm even though the overall latency and performance can be improved with more dedicated effort. 

For example, I am using std::string for orderId, which takes more memory than uint64_t, but it provides more flexibility to add alpha numberic values.

Instead of throwing exception or error, I am simply printing the errors on the terminal.

