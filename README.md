# NymphCast #

NymphCast is a server-client project that allows for video and audio to be streamed over the network. It also supports generic commands between both sides, allowing for the construction of additional functionality ('apps') on top of this foundation. 

## Goals ##

* Server (receiver) should be usable on any Single-Board Computer (SBC), including Raspberry Pi and derivatives.
* Casting of local media to a NymphCast server.
* Casting of URLs, for streaming by the server without client interaction.
* Casting of commands, to control volume, open NymphCast apps, etc.

## Platforms ##

The server targets SBCs, but like the client (and SDK) should work on any platform that supports a C++17 toolchain and is supported by the LibPoco dependency:

* Windows XP+ 
* MacOS
* Linux (desktop & embedded)
* Solaris
* *BSD
* HP-UX
* AIX
* QNX
* VxWorks
* Android
* iOS
* Windows Embedded

## Releases ##

As NymphCast is currently pre-alpha software, no releases are being made available yet. 


## Status ##

* Casting of audio & video to a NymphCast server works.
* API is being extended to support URLs and commands/apps.

## Building ##

To build NymphCast, one needs the following dependencies in addition to a C++ toolchain with C++17 support.

Run the Makefile in the `client` and `server` folders, which should output a binary into the newly created `bin/` folder.


### Server dependencies ###

* [NymphRPC](https://github.com/MayaPosch/NymphRPC)
* [LibAV](https://trac.ffmpeg.org/wiki/Using%20libav*) (v4+) 
* LibSDL2
* LibPOCO (1.5+)

### Client dependencies ###

* [NymphRPC](https://github.com/MayaPosch/NymphRPC)
* LibPOCO (1.5+)


## Running ##

The server binary can be started as-is, and will listen on all network interfaces for incoming connections.

The client binary has to be provided with the filename of a media file that should be sent to the remote server, with an optional IP address of the remote server:

	$ nymphcast_client <filename>
	$ nymphcast_client <IP> <filename>

## Limitations ##

* The client can only play one file before exiting.
* The server is assumed to have 100 MB heap space free for caching.




