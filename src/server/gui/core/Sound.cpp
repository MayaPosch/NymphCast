#include "Sound.h"

#include "AudioManager.h"
#include "Log.h"
#include "Settings.h"
#include "ThemeData.h"

std::map< std::string, std::shared_ptr<Sound> > Sound::sMap;

std::shared_ptr<Sound> Sound::get(const std::string& path)
{
	auto it = sMap.find(path);
	if(it != sMap.cend())
		return it->second;

	std::shared_ptr<Sound> sound = std::shared_ptr<Sound>(new Sound(path));
	AudioManager::getInstance()->registerSound(sound);
	sMap[path] = sound;
	return sound;
}

std::shared_ptr<Sound> Sound::getFromTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element)
{
	LOG(LogInfo) << " req sound [" << view << "." << element << "]";

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "sound");
	if(!elem || !elem->has("path"))
	{
		LOG(LogInfo) << "   (missing)";
		return get("");
	}

	return get(elem->get<std::string>("path"));
}

Sound::Sound(const std::string & path) : mSampleData(NULL), mSamplePos(0), mSampleLength(0), playing(false)
{
	loadFile(path);
}

Sound::~Sound()
{
	deinit();
}

void Sound::loadFile(const std::string & path)
{
	mPath = path;
	init();
}

void Sound::init()
{
	if(mSampleData != NULL)
		deinit();

	if(mPath.empty())
		return;

	//load wav file via SDL
	SDL_AudioSpec wave;
	Uint8 * data = NULL;
    Uint32 dlen = 0;
	if (SDL_LoadWAV(mPath.c_str(), &wave, &data, &dlen) == NULL) {
		LOG(LogError) << "Error loading sound \"" << mPath << "\"!\n" << "	" << SDL_GetError();
		return;
	}
	//build conversion buffer
	SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq, AUDIO_S16, 2, 44100);
	//copy data to conversion buffer
	cvt.len = dlen;
    cvt.buf = new Uint8[cvt.len * cvt.len_mult];
    memcpy(cvt.buf, data, dlen);
	//convert buffer to stereo, 16bit, 44.1kHz
    if (SDL_ConvertAudio(&cvt) < 0) {
		LOG(LogError) << "Error converting sound \"" << mPath << "\" to 44.1kHz, 16bit, stereo format!\n" << "	" << SDL_GetError();
		delete[] cvt.buf;
	}
	else {
		//worked. set up member data
		SDL_LockAudio();
		mSampleData = cvt.buf;
		mSampleLength = cvt.len_cvt;
		mSamplePos = 0;
		mSampleFormat.channels = 2;
		mSampleFormat.freq = 44100;
		mSampleFormat.format = AUDIO_S16;
		SDL_UnlockAudio();
	}
	//free wav data now
    SDL_FreeWAV(data);
}

void Sound::deinit()
{
	playing = false;

	if(mSampleData != NULL)
	{
		SDL_LockAudio();
		delete[] mSampleData;
		mSampleData = NULL;
		mSampleLength = 0;
		mSamplePos = 0;
		SDL_UnlockAudio();
	}
}

void Sound::play()
{
	if(mSampleData == NULL)
		return;

	if(!Settings::getInstance()->getBool("EnableSounds"))
		return;

	AudioManager::getInstance();

	SDL_LockAudio();
	if (playing)
	{
		//replay from start. rewind the sample to the beginning
		mSamplePos = 0;

	}
	else
	{
		//flag our sample as playing
		playing = true;
	}
	SDL_UnlockAudio();
	//tell the AudioManager to start playing samples
	AudioManager::getInstance()->play();
}

bool Sound::isPlaying() const
{
	return playing;
}

void Sound::stop()
{
	//flag our sample as playing and rewind its position
	SDL_LockAudio();
	playing = false;
	mSamplePos = 0;
	SDL_UnlockAudio();
}

const Uint8 * Sound::getData() const
{
	return mSampleData;
}

Uint32 Sound::getPosition() const
{
	return mSamplePos;
}

void Sound::setPosition(Uint32 newPosition)
{
	mSamplePos = newPosition;
	if (mSamplePos >= mSampleLength) {
		//got to or beyond the end of the sample. stop playing
		playing = false;
		mSamplePos = 0;
	}
}

Uint32 Sound::getLength() const
{
	return mSampleLength;
}

Uint32 Sound::getLengthMS() const
{
	//44100 samples per second, 2 channels (stereo)
	//I have no idea why the *0.75 is necessary, but otherwise it's inaccurate
	return (Uint32)((mSampleLength / 44100.0f / 2.0f * 0.75f) * 1000);
}
