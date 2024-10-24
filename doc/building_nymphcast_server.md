# NymphCast Server #

NymphCast Server (NCS) forms the core of the [NymphCast](https://github.com/MayaPosch/NymphCast) project. It provides the software that is installed on the receiver system that is connected to the speakers, television or similar. See the [NymphCast README](https://github.com/MayaPosch/NymphCast) for detailed instructions on installing and running NCS.

This document covers building NCS from source.


**Note:** the `setup.sh` script in the project root automates this process.


## Dependencies ##

A C++17-capable GCC, Clang or MSVC toolchain is required in addition to the below dependencies:

- [NymphRPC](https://github.com/MayaPosch/NymphRPC)
- [LibNymphCast](https://github.com/MayaPosch/libnymphcast)
- Ffmpeg [(LibAV)](https://trac.ffmpeg.org/wiki/Using%20libav*) (v5.1+) 
- LibSDL2
- LibSDL2_Image
- LibPOCO (1.5+)
- libFreeType
- libFreeImage
- RapidJson
- Pkg-config
- libCurl

On **Debian** & derivatives:

```
sudo apt -y install git g++ libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev pkg-config libfreetype6-dev libfreeimage-dev rapidjson-dev libcurl4-gnutls-dev
```

On **Arch** & derivatives:

```
sudo pacman -S --noconfirm --needed git sdl2 sdl2_image poco ffmpeg freetype2 freeimage rapidjson pkgconf curl
```

On **Alpine** & derivatives:

```
sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev freetype-dev freeimage-dev rapidjson-dev alsa-lib-dev glew-dev nymphrpc-dev curl-dev pkg-config
```

On **FreeBSD**:
```
pkg install -y gmake gcc git poco sdl2 sdl2_image ffmpeg openssl freetype2 freeimage rapidjson curl
```

On **MSYS2**:

```
pacman -S --noconfirm --needed git mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-poco mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-freetype mingw-w64-x86_64-freeimage mingw-w64-x86_64-rapidjson pkgconf curl
```

## Building The Server ##

If using a compatible OS (e.g. **Debian**, Alpine Linux or Arch Linux), one can use the setup script: 

1. Run the `setup.sh` script in the project root to perform the below tasks automatically.
2. Run the `install_linux.sh` script in the project root to compile & install the binaries and set up a systemd/OpenRC service on Linux systems.

Else, under Linux or MSYS2 (MinGW, Clang) use the manual procedure:

1. Check-out [NymphRPC](https://github.com/MayaPosch/NymphRPC) elsewhere and build the library with `make lib`.
2. Install NymphRPC with `sudo make install`.
3. Check-out [LibNymphCast](https://github.com/MayaPosch/libnymphcast) elsewhere and build the library with `make`.
4. Install LibNymphCast with `sudo make install`.
5. Change to `NymphCast/src/server` and execute `make` command.
6. Use `sudo make install` to install the server and associated files.
7. Use `sudo make install-systemd` (SystemD) or `sudo make install-openrc` (OpenRC) to install the relevant service file.

**Note**:For **FreeBSD** make sure to use `gmake` instead of `make`. Pay note to compile with Clang as well (see below), for NCS and dependencies like NymphRPC and libnymphcast.

## Clang ##

In order to use Clang instead of GCC/MinGW, call `make` as follows:

`make TOOLCHAIN=clang`

## MSVC ##

To compile the server with MSVC (2017, 2019 or 2022), ensure [vcpkg](https://vcpkg.io/) is installed with the `VCPKG_ROOT` environment variable defined, and call the provided .bat file from a native x64 MSVC shell:

`Setup-NMake-vcpkg.bat`

To remove intermediate build files:

`Setup-NMake-vcpkg.bat clean`

And to create an [InnoSetup](https://jrsoftware.org/isinfo.php)-based installer (with IS binaries on the system path):

`Setup-NMake-vcpkg.bat package`

**Note:** Visual Studio 2022 installs its own version of vcpkg which is incompatible with 'classic mode' vcpkg. Make sure that `VCPKG_ROOT` is set to the normal vcpkg (from GitHub).

## Uninstall ##

An (experimental) uninstall script is provided in the project root called `uninstall_linux.sh`, which should work on all supported Linux distributions.
