# NymphCast Video Setup Guide #

NymphCast Video (NC-V) is a configuration of the NymphCast receiver that supports the playback of all media content, including video output. This document covers the steps required to set up a NymphCast Video receiver system.

## Features of NymphCast Video ##

When the receiver is configured in NC-V mode, the following features are supported:

- Playback of all media content.
- Control with NymphCast clients (including the Player).
- NC-V window is hidden when playback is not active.
- NymphCast Apps.

The audio and video playback quality and other details are dependent on the underlying hardware.

## Requirements ##

In order to set up an NC-V system, the following hardware and software requirements have to be met:

- System running a supported operating system (Linux, Windows, others are experimental).
- Audio and video output (including hardware acceleration) configured in the OS.

For detailed requirements, see the [Compiling NymphCast Server manually](building_nymphcast_server.md) documentation.

## Setup: Linux ##

These instructions are targeted at Linux-based systems, as well as Linux-compatible environments, such as MacOS and BSD. NymphCast has been tested on:

- **Debian**-based distros (e.g. Ubuntu, Mint, Raspberry Pi OS)
- **Arch**-based distros (e.g. Manjaro)
- **Alpine**-based

**1. Download the project**

To use the current development version, use Git to clone the project, or obtain an archive of the project. To quickly get an archive file and extract it, use the following commands. This downloads a copy of the latest revision, extracts them to a folder and moves the command line into that folder:

```
wget https://github.com/MayaPosch/NymphCast/archive/refs/heads/master.zip
tar -xvf master.zip
cd master
```

**2. Run installer**

Inside the root of the NymphCast project folder, we find the `install_linux.sh` file, which is a shell script that can be executed to start the installation process, as well as the compilation of the project if needed:

```
./install_linux.sh
```

This will:

1. Install any dependencies required.
2. Compile NymphCast Server (if not compiled already).
3. Install NymphCast Server.
4. Install a system service (systemd/OpenRC) to auto-start the NymphCast Server.

During the installation, the question is asked which configuration of NC to use. Choose `video` here to get the NC-V configuration.


Alternatively, it's also possible to compile the server separately, and optionally install it and the associated system service:

```
./setup.sh
cd src/server
sudo make install
sudo make install-systemd (or install-openrc)
```

By default, the generic `nymphcast_config.ini` configuration is installed. Either edit this file to match one of the other provided configuration files, or copy it over (and renaming).

## Setup: Windows (with MSYS2) ##

These instructions are targeted at Windows platforms that have the [MSYS2 environment](http://msys2.org/) installed.

**1. Download the project**

To use the current development version, use Git to clone the project, or obtain an archive of the project. To quickly get an archive file and extract it, use the following commands. This downloads a copy of the latest revision, extracts them to a folder and moves the command line into that folder:

```
wget https://github.com/MayaPosch/NymphCast/archive/refs/heads/master.zip
tar -xvf master.zip
cd master
```

**2. Compile**

Inside the root of the NymphCast project folder, we find the `setup.sh` file, which is a shell script that will install any required dependencies and compile the server:

```
./setup.sh
```

After this, `the nymphcast_server.exe` binary can be found in the `src/server/bin/x86_64-w64-mingw32` folder or similar, depending on the exact MSYS2 shell used.


## Other platforms ##

- The NymphCast Server is experimentally supported on MacOS, though there may be issues with some dependencies (installed via HomeBrew).
- On BSD, GNU Make must be used. No automated dependency installation is provided yet.
