# tool macros
CXX := g++
CXXFLAGS := -Ofast -std=c++11 -lcryptopp 

TARGET := build/s3pir
INCLUDE := src/include

# src files & obj files
SRC := src/client.cpp src/server.cpp src/main.cpp src/utils.cpp
DEPS := src/include/client.h src/include/server.h src/include/utils.h 

all: $(TARGET) $(TARGET)_simlargeserver 

debug: $(TARGET)_debug

clean: 
	rm build/*

.PHONY: all clean debug

$(TARGET): $(SRC) $(DEPS)
	$(CXX) -o $(TARGET) -I $(INCLUDE) $(SRC) $(CXXFLAGS) 

$(TARGET)_debug: $(SRC) $(DEPS)
	$(CXX) -DDEBUG -o $(TARGET)_debug -I $(INCLUDE) $(SRC) $(CXXFLAGS)

$(TARGET)_simlargeserver: $(SRC) $(DEPS)
	$(CXX) -DSimLargeServer -o $(TARGET)_simlargeserver -I $(INCLUDE) $(SRC) $(CXXFLAGS)