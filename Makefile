CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -Isrc
LDFLAGS =

SRC = src/main.cpp src/vector.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = lights-please

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
