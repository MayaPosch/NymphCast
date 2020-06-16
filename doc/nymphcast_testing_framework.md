# NymphCast Testing #

Describing a framework to do quick unit and integration testing of NymphCast.

**Updated:** 2020/06/04

## 0. Goals ##

The goals of this testing framework are to:

* Decrease the round-time for a single test.
* Reduce manual testing.
* Ease localising of regressions and new issues.

## 1. Scenarios ##

Current focus is on these scenarios:

* Playback of local (client) audio media.
* Playback of local (client) video media.
* Requesting NymphCast App list.
* Starting and running App (comms).
* Playback of streaming (remote) media.

## 2. Complications ##

Complicating testing of NymphCast are the myriad interfaces which do not produce an easy to validate output, such as the SDL audio/video output routines. Since the goal is not to test SDL, however, the output to and from ffmpeg is a better target to test.

## 3. Audio playback ##

This scenario involves the playback of an audio file via the NymphCast client's 'cast file' function. This streams the audio file data to an in-memory buffer on the server's side, where it is decoded by the ffmpeg-based player. This player then outputs the decoded audio data to a buffer that is used by the SDL audio callback function.

Involved are the following interfaces:

1.  RPC call: cast_file.
2.  Ring buffer.
3.  Ffmpeg read callback.
4.  Ffmpeg seek callback.
5.  Ffmpeg packet decoding.
6.  Ffmpeg frame decoding.
7.  Audio callback buffer.

**3.1. RPC call:**

1. File data should be read verbatim.
2. Seeking in file data should occur properly, with out of bounds and EOF handled.
3. Starting a new file for playback should not result in any exceptions or unhandled situations.

*3.1.1. Testing procedure:*

A NymphCast TestServer (see documentation) is used to run integration tests against a NymphCast TestClient instance.

*Reading:*

1. Test starts TestServer.
2. TestClient is started with instruction to stream a test file to the TestServer.
3. Data received by the TestServer is compared with the test file data.
4. Matching data is a pass.

*Seeking:*

1. TestClient is started with instruction to stream a test file to the TestServer.
2. TestServer attempts to seek in the test file: 
	1. position 0 				-> should succeed, streaming new data.
	2. last byte 				-> should succeed, streaming new data.
	3. random position in file 	-> should succeed, EOF flag reset, streaming new data. 
	4. position past EOF		-> should return error, leave state unchanged.
3. Pass is when none of of those actions lead to an unrecoverable error condition.

*Second read:*

1. TestClient is started with the instruction first stream a test file, then a second file.
2. TestServer compares the received data for each file.
3. Pass is when all data matches and no exception occurs.

**3.2. Ring buffer:**

* File data is written verbatim into the ring buffer.
* Indices are maintained at all times, whether writing or seeking.
* Overwritten data is cleaned up properly.

**3.3. Ffmpeg read callback:**

