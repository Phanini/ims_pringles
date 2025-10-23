CXX = g++
CXXFLAGS = -Include -Wall -Wextra -std=c++11
SRC = src/main.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = simulation_executable

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean