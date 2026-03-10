CXX      = g++
CXXFLAGS = -std=c++11 -Wall -O2
TARGET = assign2ppl861
SRC    = assign2ppl861.cpp
OBJ    = assign2ppl861.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
