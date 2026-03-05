CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g

# Project directories
PROJECT_ROOT = ../../../..
SRC_DIR = .
BUILD_DIR = ../build
BIN_DIR = ../bin

# Dependencies (install via Homebrew)
# brew install ixwebsocket nlohmann-json openssl@3

# Homebrew paths (Apple Silicon)
HOMEBREW_PREFIX = /opt/homebrew

# Include paths
INCLUDES = -I$(HOMEBREW_PREFIX)/include \
           -I$(HOMEBREW_PREFIX)/opt/openssl@3/include \
           -I$(SRC_DIR) \
           -I../include

# Library paths
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib \
          -L$(HOMEBREW_PREFIX)/opt/openssl@3/lib

# Libraries to link
LIBS = -lixwebsocket -lssl -lcrypto -lz -pthread

# Source files
SRCS = market_feed.cpp \
       orderbook.cpp \
       kalshi_market_feed.cpp \
       main_kalshi_feed.cpp

# Object files (in build directory)
OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Target executable
TARGET = $(BIN_DIR)/kalshi_feed

.PHONY: all clean dirs

all: dirs $(TARGET)

# Create build directories
dirs:
    @mkdir -p $(BUILD_DIR)
    @mkdir -p $(BIN_DIR)

# Link executable
$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
    $(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

clean:
    rm -rf $(BUILD_DIR) $(BIN_DIR)

# Header dependencies
$(BUILD_DIR)/market_feed.o: market_feed.cpp market_feed.hpp
$(BUILD_DIR)/orderbook.o: orderbook.cpp orderbook.hpp
$(BUILD_DIR)/kalshi_market_feed.o: kalshi_market_feed.cpp kalshi_market_feed.hpp market_feed.hpp orderbook.hpp
$(BUILD_DIR)/main_kalshi_feed.o: main_kalshi_feed.cpp kalshi_market_feed.hpp

# Debug target - print variables
debug:
    @echo "PROJECT_ROOT: $(PROJECT_ROOT)"
    @echo "SRC_DIR: $(SRC_DIR)"
    @echo "BUILD_DIR: $(BUILD_DIR)"
    @echo "SRCS: $(SRCS)"
    @echo "OBJS: $(OBJS)"
    @echo "TARGET: $(TARGET)"