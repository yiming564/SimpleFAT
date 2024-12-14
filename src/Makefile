# 获取从 /Makefile 传递的变量
ROOT_DIR ?= $(CURDIR)
RULES_DIR ?= $(ROOT_DIR)/rules
INCLUDE_DIR ?= $(ROOT_DIR)/include
BUILD_DIR ?= $(ROOT_DIR)/build

# 包含编译规则
include $(RULES_DIR)/flags.make

# 源文件和对象文件
SRC_DIR = $(CURDIR)
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# 编译源文件为对象文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(OBJ_FILES)