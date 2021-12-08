
ifndef SYSROOT
$(error SYSROOT must be defined for this platform.)
endif

TARGET_ARCH = armv7
TARGET_SOC = bcm2836

TOOLCHAIN 	:= linux_aarch32

#SYSROOT 	:= --sysroot=$(SYSROOT)

PLATFORM_FLAGS 		:= -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a7 --sysroot=$(SYSROOT)
PLATFORM_LDFLAGS	:= --sysroot=$(SYSROOT) -L $(SYSROOT)/lib/arm-linux-gnueabihf \
						-L $(SYSROOT)/usr/lib/arm-linux-gnueabihf \
						 -L $(SYSROOT)/opt/vc/lib \
						 -lmmal -lmmal_core -lmmal_util -lmmal_vc_client \
						 -lvcsm -lvchiq_arm -lvcos -lrt -ldl

#STD_FLAGS = $(PLATFORM_FLAGS) -Og -g3 -Wall -c -fmessage-length=0 -ffunction-sections -fdata-sections
#STD_CFLAGS = $(STD_FLAGS)
#STD_CXXFLAGS = -std=c++11 $(STD_FLAGS)
#STD_LDFLAGS = -L $(TOP)/build/$(TARGET)/poco/lib \
#				-Wl,--gc-sections 
#STD_LIBDIRS = $(STD_LDFLAGS)

#STD_INCLUDE= -I. -I $(SYSROOT)/usr/include \
#				-I $(TOP)/build/$(TARGET)/poco/include
#STD_LIBS=-lpthread -ldl -ldlt -lrt 