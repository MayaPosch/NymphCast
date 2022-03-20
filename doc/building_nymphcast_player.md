# NymphCast Player #

This document covers how to build this software from source.

NymphCast Player (NCP) is a GUI (Qt5-based) client application that can auto-detect NymphCast Server & MediaServer instances on the local network, as well as initiate and control playback of content. 

NCP allows for the playback of local media files, files shared via an NC MediaServer, or via a URL.


## Dependencies ##

A C++17-capable GCC, or MSVC toolchain is required in addition to the below dependencies:

- [NymphRPC](https://github.com/MayaPosch/NymphRPC)
- [LibNymphCast](https://github.com/MayaPosch/libnymphcast)
- Qt5

On **Debian** & derivatives:

```
sudo apt -y install qtbase5-dev
```

On **Arch** & derivatives:

```
sudo pacman -S qt5-base
```

## Building ##

Option 1: The project's `.pro` file can be loaded in the Qt Creator IDE and the project built from there.

Option 2: Manual building using `qmake` & `make` commands.


Either way, obtain and install the NymphRPC & LibNymphCast dependencies:

1. Check-out [NymphRPC](https://github.com/MayaPosch/NymphRPC) elsewhere and build the library per the provided instructions.
2. Check-out [LibNymphCast](https://github.com/MayaPosch/libnymphcast) elsewhere and build the library per the provided instructions.

When building on the command line, follow one of the following sections.

### **Linux and similar** ###

1. Download or clone the project repository 
2. Navigate to `player/NymphCastPlayer` folder in the project files.
3. Create build folder: `mkdir build` and enter it: `cd build`
4. Trigger QMake: `qmake ..`
5. Build with Make: `make`
6. The player binary is created either in the same build folder or in a `debug/` sub-folder.

### **MSVC** ###

With MSVC 2017, 2019 or 2022:

1. Download or clone the project repository.
2. Ensure [vcpkg](https://vcpkg.io/) is installed with the VCPKG_ROOT environment variable defined.
3. Compile from native x64 MSVC shell using `Setup-NMake-vcpkg.bat`.
4. Create [InnoSetup](https://jrsoftware.org/isinfo.php)-based installer using `Setup-NMake-vcpkg.bat package` with the IS root folder in the system path.

### **Android** ###

**Note:** Poco for Android must be built using the patches provided with the [alternate Poco build system](https://github.com/MayaPosch/Poco-build) for certain network functionality to function.

1. Download or clone the project repository.
2. Compile the dependencies (LibNymphCast, NymphRPC & Poco) for the target Android platforms.
3. Ensure dependency libraries along with their headers are installed in the Android NDK, under `<ANDROID_SDK>/ndk/<VERSION>/toolchains/llvm/prebuilt/<HOST_OS>/sysroot/usr/lib/<TARGET>` where `TARGET` is the target Android platform (ARMv7, AArch64, x86, x86_64). Header files are placed in the accompanying `usr/include` folder.
4. Open the Qt project in a Qt Creator instance which has been configured for building for Android targets, and build the project.
5. An APK is produced, which can be loaded on any supported Android device.
