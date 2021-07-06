#pragma once
#ifndef ES_CORE_POWER_SAVER_H
#define ES_CORE_POWER_SAVER_H

class PowerSaver
{
public:
	enum mode : int { DISABLED = -1, INSTANT = 200, ENHANCED = 3000, DEFAULT = 10000 };

	// Call when you want PS to reload all state and settings
	static void init();

	// Get timeout to wake up from for the next event
	static int getTimeout();
	// Update currently set timeouts after User changes Timeout settings
	static void updateTimeouts();

	// Use this to check which mode you are in or get the mode timeout
	static mode getMode();
	// Called when user changes mode from Settings
	static void updateMode();

	// Get current state of PS. Not to be confused with Mode
	static bool getState();
	// State is used to temporarily pause and resume PS
	static void setState(bool state);

	// Paired calls when you want to pause PS briefly till you finish animating
	// or processing over cycles
	static void pause() { setState(false); }
	static void resume() { setState(true); }

	// This is used by ScreenSaver to let PS know when to switch to SS timeouts
	static void runningScreenSaver(bool state);
	static bool isScreenSaverActive();

private:
	static bool mState;
	static bool mRunningScreenSaver;

	static mode mMode;
	static int mWakeupTimeout;
	static int mScreenSaverTimeout;

	static void loadWakeupTime();
};

#endif // ES_CORE_POWER_SAVER_H
