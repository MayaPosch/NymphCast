/*
	ffplaydummy.h - Header file for the dummy Ffplay class.
	
	Revision 0
	
	Notes:
			- 
		
	2021/09/29 - Maya Posch
*/


#ifndef FFPLAYDUMMY_H
#define FFPLAYDUMMY_H


#include <iostream>
#include <atomic>
#include <queue>
#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mutex.h>


#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "ffplay/types.h"
#include "chronotrigger.h"


struct FileMetaInfo {
	std::atomic<uint64_t> duration;		// seconds
	std::atomic<double> position;		// seconds with remainder.
	std::atomic<uint32_t> width;		// pixels
	std::atomic<uint32_t> height;		// pixels
	std::atomic<uint32_t> video_rate;	// kilobits per second
	std::atomic<uint32_t> audio_rate;	// kilobits per second
	std::atomic<uint32_t> framrate;
	std::atomic<uint8_t> audio_channels;
	std::string title;
	std::string artist;
	std::string album;
	Poco::Mutex mutex;	// Use only with non-atomic entries.
	
	void setDuration(uint64_t d) {
		std::cout << "SetDuration. " << d << std::endl;
		duration = d;
	}
	
	uint64_t getDuration() { return duration; }
	
	void setPosition(double p) {
		if (std::isnan(p)) { position = 0; }
		else { position = p; }
	}
	
	double getPosition() { return position; }
	
	void setTitle(std::string t) { mutex.lock(); title = t; mutex.unlock(); }
	std::string getTitle() { mutex.lock(); std::string t = title; mutex.unlock(); return t; }
	
	void setArtist(std::string a) { mutex.lock(); artist = a; mutex.unlock(); }
	std::string getArtist() { mutex.lock(); std::string a = artist; mutex.unlock(); return a; }
	
	void setAlbum(std::string a) { mutex.lock(); album = a; mutex.unlock(); }
	std::string getAlbum() { mutex.lock(); std::string a = album; mutex.unlock(); return a; }
};


// --- Globals ---
extern FileMetaInfo file_meta;
	
void finishPlayback(); // Defined in NymphCastServer.cpp
void sendGlobalStatusUpdate();


typedef std::function<void(uint8_t*, uint32_t)> dummyReadCallback;

	
class FfplayDummy : public Poco::Runnable {
    VideoState* is = 0;
	static std::string loggerName;
	std::atomic<bool> running = { false };
	std::atomic<bool> playerStarted = { false };
	std::string castUrl;
	std::atomic<bool> castingUrl = { false };
	std::atomic<bool> playingTrack = { false };
	static ChronoTrigger ct;
	static int buf_size;
	static uint8_t* buf;
	static uint8_t count;
	static dummyReadCallback verifycb;
	
	static void triggerRead(int);
	static void cleanUp();
	static int media_read(void* opaque, uint8_t* buf, int buf_size);
	static int64_t media_seek(void* opaque, int64_t pos, int whence);
	
public:
	virtual void run();
	uint8_t getVolume();
	void setVolume(uint8_t volume);
	void setVerifyCallback(dummyReadCallback cb);
	bool playbackActive() { return playerStarted.load(); }
	bool streamTrack(std::string url);
	bool playTrack();
	void quit();
};


#endif
