
# ifndef SYSROOT
# $(error SYSROOT must be defined for this platform.)
# endif

TARGET_ARCH	= armv6
TARGET_SOC 	= bcm2835

TOOLCHAIN 	:= linux_aarch32

#SYSROOT 	:= --sysroot=$(SYSROOT)

PLATFORM_FLAGS 		:= -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mtune=arm1176jzf-s 
#--sysroot=$(SYSROOT)
#PLATFORM_LDFLAGS	:= --sysroot=$(SYSROOT)
