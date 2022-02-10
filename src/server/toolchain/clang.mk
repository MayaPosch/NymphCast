GPP = clang++
GCC = clang
STRIP = strip
MAKEDIR = mkdir -p
RM = rm

TARGET := $(shell clang -dumpmachine)
