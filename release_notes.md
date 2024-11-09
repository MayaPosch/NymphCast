NymphCast Release Notes
===

## v0.2 ##

**Alpha 0**

New:
- Platform support for Haiku (see notes) and Android (see notes).
- Haiku: 	Requires R1 B5 or up, SDL2/Mesa bug (BLock unlock, issue #6400) => https://dev.haiku-os.org/ticket/6400
- Android: 	API 27+, supports Android & Android/Google TV.
- Server:	Use Desktop start file for Linux Video/GUI mode instead of systemd service. (experimental)
- Local media feature added to NCS. Configured via INI file. (experimental).

Notes:
- Multi-room playback partially validated.

Known Issues:
- Subtitles: PGS works, text-based not yet.
- Android 9: Some specific files result in a crash due to a presumed AAudio issue.

## v0.1 ##

**New**

- Everything (initial release).
- Implements core streaming and playback functionality using the Client, Server & MediaServer components.
- Implements the Audio-only mode.
- Implements the Video mode (Audio + Video output).
- Implements the Screensaver mode (Video + screensaver while idle).
- Implements LCDProc support.

**Experimental**

- AngelScript-based applications (NymphCast Apps).
- Multi-cast feature.
- GUI-based interface mode (based on EmulationStation).

**Issues resolved**

- Initial release, ergo no previous issues.

**Known issues**

- Only non-text subtitles (e.g. BluRay/DVD-style PGS) are supported in this release.
- Experimental features are what it says on the tin. Stability and other issues are expected.
	
**Notes**

As an initial release, NymphCast v0.1 is primarily focused on the core functionality of streaming media content, whether from a (HTTP/RTSP) URL, or a NymphCast Client or MediaServer instance. 

Supported media formats are those which are supported by the used ffmpeg (LibAV) libraries. This includes all common types and many less common ones. Playback performance is limited by the used hardware.

The current NymphCast development process target is a v1.0 release, with each subsequent minor (0.x) release intended to add new features. Although care is taken to minimise the number of breaking changes between releases, long-term stability is not expected until v1.0.