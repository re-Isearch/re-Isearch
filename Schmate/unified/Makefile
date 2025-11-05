# Makefile for Unified HNSW with Binary/Ternary Quantization

CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall
INCLUDES = -I. -I./hnswlib
LDFLAGS =

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
endif

# Source files
SOURCES = unified_hnsw.cpp example_usage.cpp
OBJECTS = unified_hnsw.o
TARGET = example_usage

# Header files
HEADERS = unified_hnsw.hpp

# Default target
all: $(TARGET)

# Compile implementation
unified_hnsw.o: unified_hnsw.cpp unified_hnsw.hpp
	@echo "Compiling unified_hnsw.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c unified_hnsw.cpp -o unified_hnsw.o

# Link example
$(TARGET): unified_hnsw.o example_usage.cpp
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) unified_hnsw.o example_usage.cpp -o $(TARGET) $(LDFLAGS)
	@echo "Build complete!"
	@echo ""
	@echo "Run with: ./$(TARGET)"

# Clean build artifacts
clean:
	rm -f $(TARGET) *.o
	rm -f *.bin
	@echo "Cleaned build artifacts"

# Run the example
run: $(TARGET)
	./$(TARGET)

# Check which SIMD instructions are being used
check-simd:
	@echo "Checking SIMD support..."
	@echo ""
	@$(CXX) -march=native -dM -E - < /dev/null | grep -i "avx\|neon\|sse" || echo "No SIMD detected"

# Install (requires sudo on Linux)
install: $(TARGET)
	@echo "Installing to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/

# Debug build
debug: CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra
debug: clean all

# Help target
help:
	@echo "Unified HNSW Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the example program (default)"
	@echo "  clean       - Remove build artifacts"
	@echo "  run         - Build and run the example"
	@echo "  check-simd  - Check which SIMD instructions are available"
	@echo "  install     - Install to /usr/local/bin (requires sudo)"
	@echo "  debug       - Build with debug symbols"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CXX         - C++ compiler (default: g++ or clang++ on macOS)"
	@echo "  CXXFLAGS    - Compiler flags"
	@echo "  INCLUDES    - Include directories"
	@echo ""
	@echo "Example usage:"
	@echo "  make"
	@echo "  make run"
	@echo "  make CXX=clang++ CXXFLAGS='-std=c++17 -O2'"
	@echo ""
	@echo "Required files:"
	@echo "  - unified_hnsw.hpp (header)"
	@echo "  - unified_hnsw.cpp (implementation)"
	@echo "  - example_usage.cpp (example program)"
	@echo "  - hnswlib/ directory with HNSWlib headers"

.PHONY: all clean run check-simd install debug help
