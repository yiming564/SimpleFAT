# 设置编译器和编译选项
CXX = g++
CXXFLAGS = \
	-std=c++23 \
	-Wall \
	-Wno-packed-bitfield-compat\
	-g \
	-I$(INCLUDE_DIR)
