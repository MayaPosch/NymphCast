NymphCast Release Notes
===

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