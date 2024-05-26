ifeq "$(CONFIG)" ""
	CONFIG=Release
endif

# Build directory
BUILD_DIR = $(CONFIG)

# C++ Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wextra -std=c++11

ifeq "$(CONFIG)" "Debug"
	CXXFLAGS += -g -Wall -O0
else
	CXXFLAGS += -O2
endif

# Execution file target
TARGET = main
TARGET_BUILD_PATH = $(BUILD_DIR)/$(TARGET)

# Sources
SOURCE = \
	pac/pac.cpp \

all: dirs $(TARGET_BUILD_PATH)

$(TARGET_BUILD_PATH) : $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET_BUILD_PATH) $(SOURCE)

clean:
	rm -rf $(BUILD_DIR)

dirs:
	-mkdir -p $(BUILD_DIR)