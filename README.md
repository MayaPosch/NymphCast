# NymphCast #

NymphCast is a server-client project that will allow for video and audio to be streamed over the network.

## Dependencies ##

To build NymphCast, one needs the following dependencies in addition to a C++ toolchain with C++17 support.

### Server ###

* Libvlc
* [NymphRPC](https://github.com/MayaPosch/NymphRPC)
* LibSDL2
* LibPOCO

### Client ###

* [NymphRPC](https://github.com/MayaPosch/NymphRPC)
* LibPOCO

## Building ##

Run the Makefile in the `client` and `server` folders, which should output a binary into the newly created `bin/` folder.

## Running ##

The server binary can be started as-is, and will listen on all network interfaces for incoming connections.

The client binary has to be provided with the filename of a media file that should be sent to the remote server.

## Limitations ##

* The client is hard-coded to use the local loopback connection.
* The client can only play one file before exiting.

## Issues ##

* Audio playback seems stable, but video playback is mostly non-functional. LibVLC topic: [https://forum.videolan.org/viewtopic.php?f=32&t=150785](https://forum.videolan.org/viewtopic.php?f=32&t=150785)
