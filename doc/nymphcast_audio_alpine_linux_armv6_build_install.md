# NymphCast Audio: Alpine Linux #

This document covers how to set up Alpine Linux on a Raspberry Pi Zero (W) board and similar, followed by setting up NymphCast Audio (NCA).

For ARMv6-based RPi Zero no precompiled packages exist in the Alpine package repository (apk), requiring us to compile it ourselves. A precompiled package is also provided by the NymphCast project for v0.2+ releases.


## Alpine Linux ##

An advantage of using Alpine Linux for NCA is that by default Alpine runs fully in-memory, which makes it very safe and robust for embedded applications where the power can be turned off at any point without a safe shutdown.

For the Raspberry Pi (RPi), the Alpine Linux download page offers a few choices:

	=> https://alpinelinux.org/downloads/
	
The tar.gz archive is the easiest way to install. Simply extract the contents of the archive to a FAT32 partition on an SD card. The SD card is then inserted into the RPi and booted from. This starts the Alpine Linux setup procedure:

	=> https://wiki.alpinelinux.org/wiki/Installation
	
	=> https://wiki.alpinelinux.org/wiki/Raspberry_Pi
	
Boot from SD card, log in as 'root', no password. Next, run:

$ setup-alpine

Update Alpine & commit changes to SD card:

$ apk update
$ apk upgrade
$ lbu commit -d


## NymphCast Audio ##

**Install from Package**

NCA can be subsequently installed either from an existing package (for x86, ARMv7 & AArch64), or compiled. The package can be installed as follows:

$ apk add nymphcast

It may be necessary to add the 'edge' package repository if we get an error:

$ 

Make sure to save our changes:

$ lbu commit -d


**Compile source**

To compile NCA, we need to obtain the source code for NymphCast:

$ git clone --depth 1 https://github.com/MayaPosch/NymphCast

Next, run the `setup.sh` script:

$ cd NymphCast
$ ./setup.sh

Next, install NCA:

$ ./install_linux.sh

After this, we make sure to save the changes to the SD card again:

$ lbu commit -d


## Packaging ##

Creating a package of the compiled binaries and related files can be done using the NC Server Makefile:

$ cd NymphCast/src/server
$ make package

The Zip file can then be found under: `src/server/`.
