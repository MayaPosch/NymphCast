#pragma once
#ifndef ES_CORE_PLATFORM_H
#define ES_CORE_PLATFORM_H

#include <string>

//why the hell this naming inconsistency exists is well beyond me
#ifdef WIN32
	#define sleep Sleep
#endif

enum QuitMode
{
	QUIT = 0,
	RESTART = 1,
	SHUTDOWN = 2,
	REBOOT = 3
};

int runSystemCommand(const std::string& cmd_utf8); // run a utf-8 encoded in the shell (requires wstring conversion on Windows)
int quitES(QuitMode mode = QuitMode::QUIT);
void processQuitMode();

#endif // ES_CORE_PLATFORM_H
