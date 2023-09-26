LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := nymphcastserver

SDL_PATH := ../SDL
SRC_PATH := ../../../../
# Server Root: SR.
SR := $(LOCAL_PATH)/$(SRC_PATH)

# Work around the limited command length on Windows.
LOCAL_SHORT_COMMANDS := true

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
					-I $(SR). -I $(SR)ffplay \
					-I $(SR)angelscript/angelscript/include -I $(SR)angelscript/add_on \
					-I $(SR)angelscript/json -I $(SR)angelscript/regexp \
					-I $(SR)gui/core -I $(SR)gui/core/animations  -I $(SR)gui/core/components \
					-I $(SR)gui/core/guis -I $(SR)gui/core/math -I $(SR)gui/core/renderers \
					-I $(SR)gui/core/resources  -I $(SR)gui/core/utils -I $(SR)gui/app \
					-I $(SR)gui/app/animations -I $(SR)gui/app/components -I $(SR)gui/app/guis \
					-I $(SR)gui/app/scrapers -I $(SR)gui/app/views -I $(SR)gui/app/gamelist \
					-I $(SR)gui/core/nanosvg \
					-I$(NDK_HOME)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/freetype2
					#-I freetype2

VERSION = v0.2-alpha0.20220505
VERSIONINFO = -D__VERSION="\"$(VERSION)\""
LOCAL_CPPFLAGS := -Dmain=SDL_main -ffunction-sections -fdata-sections -g3 -O1 -std=c++17 \
					$(VERSIONINFO) -DUSE_OPENGL_14

# Add your application source files here.
LOCAL_SRC_FILES := 	$(wildcard $(SR)*.cpp) \
					$(wildcard $(SR)ffplay/*.cpp) \
					$(wildcard $(SR)lcdapi/api/*.cpp) \
					$(wildcard $(SR)lcdapi/sensors/*.cpp) \
					$(wildcard $(SR)gui/core/*.cpp) \
					$(wildcard $(SR)gui/core/animations/*.cpp) \
					$(wildcard $(SR)gui/core/components/*.cpp) \
					$(wildcard $(SR)gui/core/guis/*.cpp) \
					$(wildcard $(SR)gui/core/math/*.cpp) \
					$(wildcard $(SR)gui/core/renderers/*.cpp) \
					$(wildcard $(SR)gui/core/resources/*.cpp) \
					$(wildcard $(SR)gui/core/utils/*.cpp) \
					$(wildcard $(SR)gui/app/*.cpp) \
					$(wildcard $(SR)gui/app/components/*.cpp) \
					$(wildcard $(SR)gui/app/guis/*.cpp) \
					$(wildcard $(SR)gui/app/scrapers/*.cpp) \
					$(wildcard $(SR)gui/app/views/*.cpp) \
					$(wildcard $(SR)gui/app/views/gamelist/*.cpp) \
					$(wildcard $(SR)gui/app/pugixml/src/*.cpp)

LOCAL_SHARED_LIBRARIES :=  SDL2 libangelscript

LOCAL_LDLIBS := -lnymphrpc -lPocoUtil -lPocoNetSSL -lPocoJSON -lPocoDataSQLite -lPocoData \
		-lcurl -lfreeimage -lfreetype \
		-lSDL2_image \
		-lPocoFoundation \
		-lnymphcast -lPocoCrypto -lPocoUtil -lPocoNet -lPocoFoundation \
		-lswscale -lavcodec -lavdevice -lavformat -lavutil -lswresample -lavfilter \
		-lssl -lcrypto -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid
		#-L$(SR)angelscript/angelscript/lib-$(TARGET) -langelscript \

include $(BUILD_SHARED_LIBRARY)
