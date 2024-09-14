
# Uncomment this if you're using STL in your project
# You can find more information here:
# https://developer.android.com/ndk/guides/cpp-support
APP_STL := c++_shared

APP_ABI := arm64-v8a x86_64 
#armeabi-v7a

# Min runtime API level
APP_PLATFORM=android-24

APP_CFLAGS := -fsanitize=address -fno-omit-frame-pointer
APP_LDFLAGS := -fsanitize=address

#APP_SHORT_COMMANDS := true
