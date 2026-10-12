CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -ffast-math -march=native -Wall -Wextra -pthread
SRC      := src/main.cpp
HDRS     := $(wildcard src/*.h)
BIN      := build/raytracer

all: $(BIN)

$(BIN): $(SRC) $(HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

run: $(BIN)
	$(BIN) --scene spheres --width 800 --samples 100 --out render.png

clean:
	rm -rf build render.png

.PHONY: all run clean
