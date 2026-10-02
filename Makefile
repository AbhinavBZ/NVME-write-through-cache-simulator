CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude -pthread
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:.cpp=.o)
BIN      := nvme_wt_sim

.PHONY: all clean run test

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(BIN)
	mkdir -p results
	./$(BIN)

clean:
	rm -f src/*.o $(BIN)
TEST_BINS := tests/workload_test tests/metrics_test tests/cache_integration_test tests/coalescing_test

tests/workload_test: tests/workload_test.cpp src/workload.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/workload_test.cpp src/workload.cpp

tests/metrics_test: tests/metrics_test.cpp src/metrics.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/metrics_test.cpp src/metrics.cpp

tests/cache_integration_test: tests/cache_integration_test.cpp src/nvme_device.cpp src/write_through_cache.cpp src/metrics.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/cache_integration_test.cpp src/nvme_device.cpp src/write_through_cache.cpp src/metrics.cpp

tests/coalescing_test: tests/coalescing_test.cpp src/nvme_device.cpp src/write_through_cache.cpp src/metrics.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/coalescing_test.cpp src/nvme_device.cpp src/write_through_cache.cpp src/metrics.cpp

test: $(TEST_BINS)
	./tests/workload_test
	./tests/metrics_test
	./tests/cache_integration_test
	./tests/coalescing_test
