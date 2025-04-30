
# conan definitions
CONAN_INCLUDE_DIR_FLAG ?= -I
CONAN_LIB_DIR_FLAG ?= -L
CONAN_LIB_FLAG ?= -l
# Include Conan-generated make variables
CONANDEPS_FILE = build/conandeps.mk
-include $(CONANDEPS_FILE)

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra $(CONAN_INCLUDE_DIRS)
LDFLAGS = $(CONAN_LIB_DIRS) $(CONAN_LIBS)

SRC_DIR = src
BUILD_DIR = build
TARGET = waddup

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean conan

all: conan $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

conan:
	conan install . --output-folder=build --build=missing

clean:
	rm -rf $(BUILD_DIR)