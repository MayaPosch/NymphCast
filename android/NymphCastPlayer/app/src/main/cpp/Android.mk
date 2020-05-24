LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := nymphcast_jni
LOCAL_SRC_FILES := nymphcast_android.cpp

include $(BUILD_SHARED_LIBRARY)
