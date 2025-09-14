CXX=i686-w64-mingw32-g++
CXXFLAGS=-g -O2 -Wall -std=c++20
LDFLAGS=
OBJ=$(shell find -name "*.cc" | sed -e 's/\.cc$$/.o/g')
TARGET=ninix-proxy.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJ)
