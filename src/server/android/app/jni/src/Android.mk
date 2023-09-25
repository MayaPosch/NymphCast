LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
SRC_PATH := ../../../../
# Server Root: SR.
SR := $(SRC_PATH)

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
					-I freetype2

VERSION = v0.2-alpha0.20220505
VERSIONINFO = -D__VERSION="\"$(VERSION)\""
LOCAL_CPPFLAGS := -Dmain=SDL_main -ffunction-sections -fdata-sections -g3 -O1 -std=c++17 \
					$(VERSIONINFO) -DUSE_OPENGL_14

# Add your application source files here.
LOCAL_SRC_FILES := 	$(wildcard $(SR)*.cpp) \
					$(wildcard $(SR)ffplay/*.cpp) \
					$(wildcard $(SR)angelscript/add_on/scriptstdstring/*.cpp) \
					$(wildcard $(SR)angelscript/add_on/scriptbuilder/*.cpp) \
					$(wildcard $(SR)angelscript/add_on/scriptarray/*.cpp) \
					$(wildcard $(SR)angelscript/add_on/scriptdictionary/*.cpp) \
					$(wildcard $(SR)angelscript/json/*.cpp) \
					$(wildcard $(SR)angelscript/regexp/*.cpp) \
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

LOCAL_SHARED_LIBRARIES := nymphrpc PocoUtil PocoNetSSL PocoJSON PocoDataDSQLite PocoData curl \
							freeimage freetype SDL2 SDL2_image PocoFoundation nymphcast PocoCrypto \
							PocoUtil PocoNet PocoFoundation lwscale avcodec avdevice avformat \
							avutil swresample avfilter ssl crypto

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
