#include "PowerSaver.h"

#include "AudioManager.h"
#include "Settings.h"

bool PowerSaver::mState = false;
bool PowerSaver::mRunningScreenSaver = false;

int PowerSaver::mWakeupTimeout = -1;
int PowerSaver::mScreenSaverTimeout = -1;
PowerSaver::mode PowerSaver::mMode = PowerSaver::DISABLED;

void PowerSaver::init()
{
	setState(true);
	updateMode();
}

int PowerSaver::getTimeout()
{
	if (SDL_GetAudioStatus() == SDL_AUDIO_PAUSED)
		AudioManager::getInstance()->deinit();

	// Used only for SDL_WaitEventTimeout. Use `getMode()` for modes.
	return mRunningScreenSaver ? mWakeupTimeout : mScreenSaverTimeout;
}

void PowerSaver::loadWakeupTime()
{
	// TODO : Move this to Screensaver Class
	std::string behaviour = Settings::getInstance()->getString("ScreenSaverBehavior");
	if (behaviour == "random video")
		mWakeupTimeout = Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") - getMode();
	else if (behaviour == "slideshow")
		mWakeupTimeout = Settings::getInstance()->getInt("ScreenSaverSwapImageTimeout") - getMode();
	else // Dim and Blank
		mWakeupTimeout = -1;
}

void PowerSaver::updateTimeouts()
{
	mScreenSaverTimeout = (unsigned int) Settings::getInstance()->getInt("ScreenSaverTime");
	mScreenSaverTimeout = mScreenSaverTimeout > 0 ? mScreenSaverTimeout - getMode() : -1;
	loadWakeupTime();
}

PowerSaver::mode PowerSaver::getMode()
{
	return mMode;
}

void PowerSaver::updateMode()
{
	std::string mode = Settings::getInstance()->getString("PowerSaverMode");

	if (mode == "disabled") {
		mMode = DISABLED;
	} else if (mode == "instant") {
		mMode = INSTANT;
	} else if (mode == "enhanced") {
		mMode = ENHANCED;
	} else {
		mMode = DEFAULT;
	}
	updateTimeouts();
}

bool PowerSaver::getState()
{
	return mState;
}

void PowerSaver::setState(bool state)
{
	bool ps_enabled = Settings::getInstance()->getString("PowerSaverMode") != "disabled";
	mState = ps_enabled && state;
}

void PowerSaver::runningScreenSaver(bool state)
{
	mRunningScreenSaver = state;
	if (mWakeupTimeout < mMode)
	{
		// Disable PS if wake up time is less than mode as PS will never trigger
		setState(!state);
	}
}

bool PowerSaver::isScreenSaverActive()
{
	return mRunningScreenSaver;
}
