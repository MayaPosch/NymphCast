# NymphCast #

NymphCast is a server-client project that allows for video and audio to be streamed over the network.

## Dependencies ##

To build NymphCast, one needs the following dependencies in addition to a C++ toolchain with C++17 support.

### Server ###

* [NymphRPC](https://github.com/MayaPosch/NymphRPC)
* [LibAV](https://trac.ffmpeg.org/wiki/Using%20libav*)
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
* The server is assumed to have 100 MB heap space free for caching.

## Status ##

This is alpha-level code. It's only guaranteed to have lots of unresolved issues. Feel free to submit tickets for any issues you have foun.



