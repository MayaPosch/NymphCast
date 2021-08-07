[Repository Structure](#id-rs) &middot; [Getting Started](#id-gs) &middot; [Building From Source](#id-bfs) &middot; [Developer's Guide](#id-dg) &middot; [SDK](#id-sdk) &middot; [License](#id-lic) &middot; [Donate](#id-donate)

# What is NymphCast? #

NymphCast is a software solution which turns your choice of Linux-capable hardware into an audio and video source for a television or powered speakers. It enables the streaming of audio and video over the network from a wide range of client devices, as well as the streaming of internet media to a NymphCast server, controlled by a client device, or directly on the receiver.

In addition, the server supports powerful NymphCast apps written in AngelScript to extend the overall NymphCast functionality with e.g. 3rd party audio / video streaming protocol support on the server side, and cross-platform control panels served to the client application that integrate with the overall client experience.  

NymphCast requires in all but the GUI mode that at least the server application runs on a target device, while the full functionality is provided in conjunction with a  remote control device: 
![NymphCast diagram](doc/nymphcast.png)

Client-side core functionality is provided through the NymphCast library.

## Usage Scenarios ##

NymphCast can be used in a number of scenarios:

- **NymphCast Audio** :arrow_right: Audio-only receiver mode.
	+ Profile: `nymphcast_audio_config.ini`
- **NymphCast Video** :arrow_right: Audio & Video receiver mode.
	+ Profile: `nymphcast_video_config.ini`
- **NymphCast ScreenSaver** :arrow_right: Like NymphCast Video, but with image-based ScreenSaver when not playing content.
	+ Profile: `nymphcast_screensaver_config.ini`
- **NymphCast GUI** :arrow_right: Like NymphCast Video, but with stand-alone GUI mode (smart TV) enabled.
	+ Profile: `nymphcast_gui_config.ini`
	
## Network ports ##

**Note:** NymphCast Server uses UDP port 4004 for discovery, and TCP port 4004 for playback.

All of these ports have to be reachable on a system running NymphCast Server.

Systems running the [NymphCast MediaServer](https://github.com/MayaPosch/NymphCast-MediaServer) need to have UDP & TCP ports 4005 reachable.

Please ensure that these ports are whitelisted in e.g. firewall rules if applicable.

## GUI configuration ##

When setting up NymphCast server for GUI mode, the necessary resource files (in the `.emulationstation` folder) are by default located under `src/server`. This can be overridden in two ways:

1. Copy the `src/server/.emulationstation` folder to the home folder (`~/`) of the user running NymphCast Server.
2. Pass the path to the folder containing the `.emulationstation` folder as a command line option.

## Features & Status ##

The current development version is v0.1-alpha5. Version 0.1 will be the first release. The following list contains the major features that are planned for the v0.1 release, along with their implementation status.

Category | Status | Description | Notes
---|----|---|---
Streaming |90% | Streaming media files from client to server | Meta-info & playback status reporting in progress.
Playback | 75% | Subtitle & stream selection support | Implemented but untested.
Streaming | 100% | Streaming from URLs | 
Playback | 100% | Audio & Video codec support | Supports all codecs supported by ffmpeg.
Multi-cast | 50% | Synchronised multi-room playback | Basic implementation exists, synchronisation and other bugs remain.
Streaming | 100% | Streaming from NymphCast MediaServer instances | 
SmartTV | 75% | Stand-alone GUI mode | EmulationStation-based GUI has been integrated. Customisation and testing are in progress.


The following features are considered **experimental** and will exist as a potential v0.2 preview in v0.1-release:

Category | Status | Description | Notes
---|----|---|---
Apps | 25% | NymphCast Apps | AngelScript-based apps. Currently implemented as CLI-based interface with limited runtime APIs. GUI implementation is highly experimental.


## NymphCast Ecosystem ##


The NymphCast project consists out of multiple components:

Component | Purpose | Status
---|---|---
NymphCast Server | Receiver end-point for clients. Connected to the audiovisual device. | v0.1-alpha5
[LibNymphCast](https://github.com/MayaPosch/libnymphcast) | Library for developing NymphCast clients with. | v0.1-alpha5
NymphCast Client | CLI-based NymphCast client. | v0.1-alpha4
NymphCast Player | Graphical, Qt-based NymphCast client. SDK reference implementation. | v0.1-alpha4
[NymphCast MediaServer](https://github.com/MayaPosch/NymphCast-MediaServer) | Server application for making media content available to NymphCast clients. | v0.1-alpha0

### **NymphCast Player Client** ###

The NymphCast Player provides NymphCast client functionality in a graphical (Qt-based) format. It is also a demonstration platform for the NymphCast SDK (see details on the SDK later in this document). It is designed to run on any OS that is supported by the Qt framework.


### **Server Platforms** ###

The server should work on any platform that is supported by a C++17 toolchain and the LibPoco & ffmpeg (libAV) dependencies. This includes Windows, MacOS, Linux and BSD.

FFmpeg and SDL2 libraries are used for audio and video playback. Both of which are supported on a wide variety of platforms, with Linux, MacOS and Windows being the primary platforms. **System requirements** also depend on whether only audio or also video playback is required. The latter can be disabled, which drops any graphical output requirement.

**Memory requirements** depend on the NymphCast Server configuration: by default the ffmpeg library uses an internal 32 kB buffer, and the server itself a 20 MB buffer. The latter can be configured using the (required) configuration INI file, allowing it to be tweaked to fit the use case.

### **Client Platforms** ###

For the Qt-based NymphCast Player, a target platform needs to support LibPoco and have a C++ compiler which supports C++17 (&lt;filesystem&gt; header supported) or better, along with Qt5 support. Essentially, this means any mainstream desktop OS including Linux, Windows, BSD and MacOS should qualify, along with mobile platforms. Currently Android is also supported via Qt's mobile support, with iOS support planned.

For the CLI-based NymphCast Client, only LibPoco and and C++17 support are required. All clients require the use of libnymphcast (see SDK section) as dependency.

Note that all mobile platforms are a work in progress due to the limitations and peculiarities of these platforms.

<a id="id-rs"></a>
## Repository Structure ##

The repository currently contains the NymphCast server, client SDK and NymphCast Player client sources.

	/
	|- player 	(the NymphCast demonstration client)
	|- src/
	|	|- client 		(basic CLI NymphCast client)
	|	|- server		(the NymphCast server and NymphCast app files)
	|- tools	(shell scripts for creating releases, in progress)


<a id="id-gs"></a>
## Getting Started ##

To start using NymphCast, you need a device on which the server will be running (most likely an SBC or other Linux system). NymphCast is offered as binaries for selected distros, and as source code for use and development on a variety of platforms.

### **Releases** ###

NymphCast is currently in Alpha stage. Experimental releases are available on Github (see the ['Releases'](https://github.com/MayaPosch/NymphCast/releases) folder).

Some packages also exist for selected distros.

For **pacman**-based distros (ArchLinux, Manjaro):

* the server: [nymphcast-server-git](https://aur.archlinux.org/packages/nymphcast-server-git/)
* the sdk: [nymphcast-sdk-git](https://aur.archlinux.org/packages/nymphcast-sdk-git/)
* the player client: [nymphcast-player-git](https://aur.archlinux.org/packages/nymphcast-player-git/)

For **Alpine Linux** and postmarketOS:

* the server: [nymphcast-server](https://pkgs.alpinelinux.org/package/edge/testing/x86_64/nymphcast-server)
* the sdk: [nymphcast-dev](https://pkgs.alpinelinux.org/package/edge/testing/x86_64/nymphcast-dev)
* the player client: [nymphcast-client](https://pkgs.alpinelinux.org/package/edge/testing/x86_64/nymphcast-client)

Player client releases for **Android** and **Windows**:

* APK for installation on Android, see ['Releases'](https://github.com/MayaPosch/NymphCast/releases) 
* desktop client for Windows(x64), see ['Releases'](https://github.com/MayaPosch/NymphCast/releases)   

If pre-compiled releases for your target device or operating system are currently not listed above, you may need to build the server and client applications from source.

  
### **Running NymphCast** ###

The **server binary** can be started with only the configuration flag specified.
To start the server, execute the binary (from the `bin/<platform>/` folder) to have it start listening on port 4004, with the appropriate configuration file (see Usage Scenarios for a list). 

E.g. for an audio-only configuration:

`./nymphcast_server -c nymphcast_audio_config.ini`.
 
The server will listen on all network interfaces for incoming connections. It supports the following options:
```
-h	--help				Get this help message.
-c	--configuration		Path to the configuration file.
-a	--apps				Path to the NymphCast apps folder.
-w	--wallpaper			Path to the wallpapers folder.
-r	--resources			Path to the GUI resources folder.
-v	--version			Output NymphCast server version and exit.
```

The **client binary** supports the following flags:

```
Usage:
        nymphcast_client <options>

Options:
-h      --help          Get this help message.
-v      --version       Output the NymphCast client version and exit.
-r      --remotes       Display online NymphCast receivers and quit.
-f      --file          Name of file to stream to remote receiver.
-i      --ip            IP address of the target NymphCast receiver.
```

The **NymphCast Player** is a GUI-based application and accepts no command line options.


<a id="id-bfs"></a>
## Building From Source ##

**Note:** This section is for building the project from source. Pre-built binaries are provided in the ['Releases'](https://github.com/MayaPosch/NymphCast/releases) folder.

The steps below assume building the server part on a system running a current version of Debian (Buster) or similarly current version of Arch (Manjaro) Linux or Alpine Linux. The player client demo application can be built on Linux/BSD/MacOS with a current GCC toolchain, or MSYS2 on Windows with MinGW toolchain. 

Once the project files have been downloaded, run the `setup.sh` script in the project root, or install the dependencies and run the Makefile in the `client` and `server` folders as described. Either method will output a binary into the newly created `bin/` folder.

To build the corresponding client-related parts of NymphCast, in addition to a C++ toolchain with C++17 support, one needs the dependencies as listed below.

### **Server Dependencies** ###

- [NymphRPC](https://github.com/MayaPosch/NymphRPC)
- LibNymphCast (see client library & SDK sections)
- [LibAV](https://trac.ffmpeg.org/wiki/Using%20libav*) (v4+) 
- LibSDL2
- LibSDL2_Image
- LibPOCO (1.5+)
- libFreeType
- libFreeImage
- RapidJson
- Pkg-config

On **Debian** & derivatives:

```
sudo apt -y install git g++ libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev pkg-config libfreetype6-dev libfreeimage-dev rapidjson-dev libcurl4-gnutls-dev libvlc-dev
```

On **Arch** & derivatives:

```
sudo pacman -S --noconfirm --needed sdl2 sdl2_image poco ffmpeg freetype freeimage rapidjson pkgconf
```

On **Alpine** & derivatives:

```
sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev freetype-dev freeimage-dev rapidjson-dev vlc-dev curl-dev alsa-lib-dev nymphrpc-dev pkg-config
```


### **Client Library Dependencies** ###

* [NymphRPC](https://github.com/MayaPosch/NymphRPC)
* LibPOCO (1.5+)


### **Building The Server** ###

If using a compatible OS (e.g. **Debian** Buster, Alpine Linux or Arch Linux), one can use the setup script: 

1. Run the `setup.sh` script in the project root to perform the below tasks automatically.
2. Run the `install_linux.sh` script in the project root to install the binaries and set up a systemd/OpenRC service on Linux systems.

Else, use the manual procedure:

1. Check-out [NymphRPC](https://github.com/MayaPosch/NymphRPC) elsewhere and build the library with `make lib`.
2. Install NymphRPC with `sudo make install`.
3. Change to `NymphCast/src/client_library` and execute `make` and `sudo make install`.
4. Change to `NymphCast/src/server` and execute `make` command.
5. Use `sudo make install` to install the server and associated files.
6. Use `sudo make install-systemd` (SystemD) or `sudo make install-openrc` (OpenRC) to install the relevant service file.


### **Building The NymphCast Player Client** ###

This demonstration client uses Qt5 to provide user interface functionality. The binary release comes with the necessary dependencies, but when building it from source, make sure Qt5.x is installed or get it from [www.qt.io](https://www.qt.io/download).

For **Windows** (x64):

1. Download and extract the binary release.

Or (building and running on Windows & other **desktop** platforms):

1. Download or clone the project repository 
2. Build the libnymphcast library in the `src/client_lib` folder using the Makefile in that folder: `make lib`.
3. Execute `sudo make install` to install the library.
5. Ensure the Qt5 SDK is installed.
6. Create `player/NymphCastPlayer/build` folder and change into it.
7. Execute `qmake ..` followed by `make`.
8. The player binary is created either in the same build folder or in a `debug/` sub-folder.

On **Android**:

1. Download or clone the project repository.
2. Compile the dependencies (NymphCast client SDK, NymphRPC & Poco) for the target Android platforms.
3. Ensure dependency libraries along with their headers are installed in the Android NDK, under `<ANDROID_SDK>/ndk/<VERSION>/toolchains/llvm/prebuilt/<HOST_OS>/sysroot/usr/lib/<TARGET>` where `TARGET` is the target Android platform (ARMv7, AArch64, x86, x86_64). Header files are placed in the accompanying `usr/include` folder.
4. Open the Qt project in a Qt Creator instance which has been configured for building for Android targets, and build the project.
5. An APK is produced, which can be loaded on any supported Android device.


Now you should be able to execute the player binary, connect to the server instance using its IP address and start casting media from a file or URL.




<a id="id-dg"></a>
## Developer's Guide ##

The focus of the project is currently on the development of the NymphCast server and the protocol parts. Third parties are encouraged to contribute server-side app support of their services and developers in general to contribute to server- and client-side development.

The current server and client documentation is hosted at the [Nyanko website](http://nyanko.ws/nymphcast.php).

<a id="id-sdk"></a>
## SDK ##

An SDK has been made available in the [LibNymphCast repository](https://github.com/MayaPosch/libnymphcast). The player project under `player/` uses the SDK as part of a Qt5 project to implement a NymphCast client which exposes all of the NymphCast features to the user.

To use the SDK, the Makefile in the SDK folder can be executed with a simple `make` command, after which a library file can be found in the `src/client_lib/lib` folder. This can be installed (on Linux) with `make install`.

**Note:** to compile the SDK, both [NymphRPC](https://github.com/MayaPosch/NymphRPC) and LibPOCO (1.5+) must be installed.

**Note:** For Android, one can compile for ARMv7 Android using `make lib ANDROID=1`and for AArch64 Android using `ANDROID64=1`. This requires that the Android SDK and NDK are installed and on the system path.

After this the only files needed by a client project are this library file and the `nymphcast_client.h` header file. 


<a id="id-lic"></a>
## License ##

NymphCast is a fully open source project. The full, 3-clause BSD-licensed source code can be found at its project page on Github, along with binary releases.

<a id="id-donate"></a>
## Donate ##

NymphCast is fully free, but its development relies on your support. If you appreciate the project, your contribution, [Ko-Fi](https://ko-fi.com/mayaposch) or [donation](http://nyanko.ws/nymphcast.php#donate) will help to support the continued development.


