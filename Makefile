# 获取所需目录的绝对路径路径
ROOT_DIR := $(CURDIR)
RULES_DIR = $(ROOT_DIR)/rules
SRC_DIR = $(ROOT_DIR)/src
BUILD_DIR = $(ROOT_DIR)/build
INCLUDE_DIR = $(ROOT_DIR)/include

# 目标可执行文件名
TARGET = $(BUILD_DIR)/FAT32

# 包含编译规则
include $(RULES_DIR)/flags.make

# 获取所有的对象文件
OBJ_FILES = $(wildcard $(BUILD_DIR)/*.o)

# 默认目标
all: $(TARGET)

# 链接对象文件生成最终可执行文件
$(TARGET): submodule
	$(CXX) $(OBJ_FILES) -o $@

# 调用子目录下的 Makefile 进行编译
submodule:
	@mkdir -p $(BUILD_DIR)
	@$(MAKE) -C $(SRC_DIR) ROOT_DIR=$(ROOT_DIR) INCLUDE_DIR=$(INCLUDE_DIR) BUILD_DIR=$(BUILD_DIR)

# 清理构建产生的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: all
	@echo "===== Program Launched ====="
	$(TARGET)

# 提供伪目标
.PHONY: all clean submodule