# NymphCast Server #

NymphCast Server (NCS) forms the core of the [NymphCast](https://github.com/MayaPosch/NymphCast) project. It provides the software that is installed on the receiver system that is connected to the speakers, television or similar. See the [NymphCast README](https://github.com/MayaPosch/NymphCast) for detailed instructions on installing and running NCS.

This document covers building NCS from source.


**Note:** the `setup.sh` script in the project root automates this process.


## Dependencies ##

A C++17-capable GCC toolchain is required in addition to the below dependencies:

- [NymphRPC](https://github.com/MayaPosch/NymphRPC)
- [LibNymphCast](https://github.com/MayaPosch/libnymphcast)
- Ffmpeg [(LibAV)](https://trac.ffmpeg.org/wiki/Using%20libav*) (v4+) 
- LibSDL2
- LibSDL2_Image
- LibPOCO (1.5+)
- libFreeType
- libFreeImage
- RapidJson
- Pkg-config
- libVLC
- libCurl

On **Debian** & derivatives:

```
sudo apt -y install git g++ libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev pkg-config libfreetype6-dev libfreeimage-dev rapidjson-dev libcurl4-gnutls-dev libvlc-dev
```

On **Arch** & derivatives:

```
sudo pacman -S --noconfirm --needed git sdl2 sdl2_image poco ffmpeg freetype2 freeimage rapidjson pkgconf curl vlc
```

On **Alpine** & derivatives:

```
sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev freetype-dev freeimage-dev rapidjson-dev alsa-lib-dev glew-dev nymphrpc-dev curl-dev vlc-dev pkg-config
```

On **MSYS2**:

```
pacman -S --noconfirm --needed git mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-poco mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-freetype mingw-w64-x86_64-freeimage mingw-w64-x86_64-rapidjson pkgconf curl mingw-w64-x86_64-vlc
```

## Building The Server ##

If using a compatible OS (e.g. **Debian**, Alpine Linux or Arch Linux), one can use the setup script: 

1. Run the `install_linux.sh` script in the project root to compile & install the binaries and set up a systemd/OpenRC service on Linux systems.

Or:

1. Run the `setup.sh` script in the project root to perform the below tasks automatically.

Else, use the manual procedure:

1. Check-out [NymphRPC](https://github.com/MayaPosch/NymphRPC) elsewhere and build the library with `make lib`.
2. Install NymphRPC with `sudo make install`.
3. Check-out [LibNymphCast](https://github.com/MayaPosch/libnymphcast) elsewhere and build the library with `make`.
4. Install LibNymphCast with `sudo make install`.
5. Change to `NymphCast/src/server` and execute `make` command.
6. Use `sudo make install` to install the server and associated files.
7. Use `sudo make install-systemd` (SystemD) or `sudo make install-openrc` (OpenRC) to install the relevant service file.
