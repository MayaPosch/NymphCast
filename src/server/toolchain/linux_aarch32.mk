# Toolchain configuration for Linux-AArch32

TARGET_OS = linux
#TARGET_ARCH = armv7
ifdef OS
#TOOLCHAIN_NAME = arm-none-linux-gnueabihf
TOOLCHAIN_NAME = aarch64-none-linux-gnu
else
TOOLCHAIN_NAME = arm-linux-gnueabihf
endif

GCC = $(TOOLCHAIN_NAME)-gcc
GPP = $(TOOLCHAIN_NAME)-g++
AR = $(TOOLCHAIN_NAME)-ar
LD = $(TOOLCHAIN_NAME)-g++
STRIP = $(TOOLCHAIN_NAME)-strip
OBJCOPY = $(TOOLCHAIN_NAME)-objcopy
MAKEDIR = mkdir -p
RM = rm
