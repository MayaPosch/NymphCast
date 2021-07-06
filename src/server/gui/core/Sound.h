#pragma once
#ifndef ES_CORE_SOUND_H
#define ES_CORE_SOUND_H

#include "SDL_audio.h"
#include <map>
#include <memory>
#include <string>

class ThemeData;

class Sound
{
	std::string mPath;
    SDL_AudioSpec mSampleFormat;
	Uint8 * mSampleData;
    Uint32 mSamplePos;
    Uint32 mSampleLength;
	bool playing;

public:
	static std::shared_ptr<Sound> get(const std::string& path);
	static std::shared_ptr<Sound> getFromTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& elem);

	~Sound();

	void init();
	void deinit();

	void loadFile(const std::string & path);

	void play();
	bool isPlaying() const;
	void stop();

	const Uint8 * getData() const;
	Uint32 getPosition() const;
	void setPosition(Uint32 newPosition);
	Uint32 getLength() const;
	Uint32 getLengthMS() const;

private:
	Sound(const std::string & path = "");
	static std::map< std::string, std::shared_ptr<Sound> > sMap;
};

#endif // ES_CORE_SOUND_H
