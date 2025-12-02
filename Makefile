CXX = g++
CXXFLAGS = -Include -Wall -Wextra -std=c++11 -I./simlib/src
LDFLAGS = -L./simlib/src -l:libsimlib.a
SRC = src/main.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = simulation_executable

all: $(TARGET)

# Build rules
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)