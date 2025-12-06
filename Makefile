# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -g -pedantic -I./simlib/src
LDFLAGS = -L./simlib/src -l:simlib.a

# Target name
TARGET = simulation_executable

# Source files
SRC = src/main.cpp
OBJ = $(SRC:.cpp=.o)
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) OBJ = $(OBJ)
