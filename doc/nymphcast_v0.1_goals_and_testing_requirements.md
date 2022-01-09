# NymphCast v0.1 #

**Goals and Testing Requirements**

## 0. Overview ##

This document covers the goals and requirements to finalise NymphCast v0.1 development.

## 1. Requirements ##

There are four distinct groups within NymphCast, one pertaining to **NymphCast Audio** (NC-A) - the audio-only version - and **NymphCast** (NC), which is the superset of NymphCast Audio that adds video support. There is the **Player** (NCP) component as the third item, as a demonstrator of the NymphCast SDK.

Finally, there is the **NymphCast MediaServer** (NCMS), which is both a client and server, allowing control by an NCP to independently stream content to an NC-A or NC instance.

**1.1. NymphCast Audio**

NymphCast Audio mode is created by disabling video output in the NymphCast server's configuration file. This also disables the screensaver feature, and any media file with a video track will only play the audio track.

This mode has the following **requirements**:

1. Play back any media file for which the codec is supported by ffmpeg.
2. Support streaming media via any ffmpeg-supported format.
3. Support AngelScript-based extensions ('*NymphCast apps*') that use the app extension API (see AngelScript extension API reference) provided by the NymphCast server. => Preview in v0.1, targeting v0.2 for release.

**1.2. NymphCast**

NymphCast is a super-set of NymphCast Audio, containing all of the features of NymphCast Audio, while adding video playback and a screensaver mode:

1. All NymphCast Audio features.
2. Video playback, limited by the capabilities of the hardware platform's decoding and display features.
3. Screensaver mode (optional), which displays wallpapers while no media is being played back.

**1.3. NymphCast Player**

The NymphCast Player is the primary demonstrator of the NymphCast client functionality. It uses the NymphCast client SDK. NymphCast Player's performance is considered to be a direct test of the SDK, which is why both are grouped together.

The Player's requirements are:

1. Provide a way to select local media files for remote playback.
2. Provide a way to select media streams for remote playback.
3. Implement a reasonable subset of the features provided by the NymphCast SDK to:
	1. Start, pause and stop playback.
	2. Control volume.
	3. Control NymphCast apps on the remote server.
	4. Find and connect to NymphCast servers on the local network.
4. Target the desktop (Windows, Linux, MacOS) and mobile (Android) platforms.

**1.4. NymphCast MediaServer**

NCMS shares a lot of code with the NC and NCP projects, even so it adds a few additional features:

1. Share a list of available media files to clients.
2. Allow for the playing back of these files to any NC(-A) receivers.
3. Support playlists (M3U format).


## 2. Goals ##

In order for NymphCast to qualify for release, it must reach the following goals:

1. Provide stable and reliable media playback as defined in *1.1.1* & *1.1.2*.
2. Reliable NC & NC-A discovery (NyanSD).
3. Reliable status & meta data (of media files) updates.
4. Reliable playback controls across NCP instances.