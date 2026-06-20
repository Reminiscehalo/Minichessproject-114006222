CXX = g++
CXXFLAGS = -std=c++2a -Wall -Wextra -Wpedantic -g -O3
LDFLAGS =

SOURCES_DIR = src
UNITTEST_DIR = unittest
BUILD_DIR = build

STATE_SOURCE = $(SOURCES_DIR)/games/minichess/state.cpp
POLICY_SRC = $(wildcard $(SOURCES_DIR)/policy/*.cpp)
UNITTESTS = $(wildcard $(UNITTEST_DIR)/*.cpp)
TARGET_UNITTEST = $(UNITTESTS:$(UNITTEST_DIR)/%_test.cpp=%)

MINICHESS_INC = -Isrc/games/minichess -Isrc/state -Isrc

ifeq ($(OS),Windows_NT)
	EXE = .exe
	LDFLAGS = -static -static-libgcc -static-libstdc++ -lwinpthread
	MKDIR_BUILD = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	MKDIR_TEST = if not exist $(UNITTEST_DIR)\build mkdir $(UNITTEST_DIR)\build
	RM = del /Q
	NULLDEV = 2>NUL
else
	EXE =
	MKDIR_BUILD = mkdir -p $(BUILD_DIR)
	MKDIR_TEST = mkdir -p $(UNITTEST_DIR)/build
	RM = rm -f
	NULLDEV =
endif

MINICHESS_BIN = $(BUILD_DIR)/minichess-ubgi$(EXE)
BENCHMARK_BIN = $(BUILD_DIR)/minichess-benchmark$(EXE)

.PHONY: all clean minichess benchmark test

all: minichess benchmark test

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(UNITTEST_DIR)/build:
	$(MKDIR_TEST)

minichess: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(MINICHESS_INC) -o $(MINICHESS_BIN) $(STATE_SOURCE) $(POLICY_SRC) src/ubgi/ubgi.cpp $(LDFLAGS)

benchmark: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(MINICHESS_INC) -o $(BENCHMARK_BIN) $(STATE_SOURCE) $(POLICY_SRC) src/benchmark.cpp $(LDFLAGS)

test: $(TARGET_UNITTEST)

$(TARGET_UNITTEST): %: $(UNITTEST_DIR)/%_test.cpp | $(UNITTEST_DIR)/build
	$(CXX) $(CXXFLAGS) $(MINICHESS_INC) -o $(UNITTEST_DIR)/build/$@_test$(EXE) $(STATE_SOURCE) $(POLICY_SRC) $< $(LDFLAGS)

clean:
ifeq ($(OS),Windows_NT)
	-$(RM) $(BUILD_DIR)\minichess-ubgi.exe $(BUILD_DIR)\minichess-benchmark.exe $(UNITTEST_DIR)\build\*_test.exe $(NULLDEV)
	-$(RM) $(BUILD_DIR)\minichess-ubgi $(BUILD_DIR)\minichess-benchmark $(UNITTEST_DIR)\build\*_test $(NULLDEV)
else
	$(RM) $(BUILD_DIR)/minichess-ubgi $(BUILD_DIR)/minichess-benchmark
	$(RM) $(UNITTEST_DIR)/build/*_test
endif
