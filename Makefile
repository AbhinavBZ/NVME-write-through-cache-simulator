CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude -pthread
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:.cpp=.o)
BIN      := nvme_wt_sim

.PHONY: all clean run

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
