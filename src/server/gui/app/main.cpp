//EmulationStation, a graphical front-end for ROM browsing. Created by Alec "Aloshi" Lofquist.
//http://www.aloshi.com

#include "guis/GuiDetectDevice.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
#include "utils/ProfilingUtil.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "platform.h"
#include "PowerSaver.h"
#include "ScraperCmdLine.h"
#include "Settings.h"
#include "SystemData.h"
#include "SystemScreenSaver.h"
#include <SDL_events.h>
#include <SDL_main.h>
#include <SDL_timer.h>
#include <iostream>
#include <time.h>
#ifdef WIN32
#include <Windows.h>
#endif

#include <FreeImage.h>

bool scrape_cmdline = false;

bool parseArgs(int argc, char* argv[])
{
	Utils::FileSystem::setExePath(argv[0]);

	// We need to process --home before any call to Settings::getInstance(), because settings are loaded from homepath
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--home") == 0)
		{
			if(i >= argc - 1)
			{
				std::cerr << "Invalid home path supplied.";
				return false;
			}

			Utils::FileSystem::setHomePath(argv[i + 1]);
			break;
		}
	}

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--resolution") == 0)
		{
			if(i >= argc - 2)
			{
				std::cerr << "Invalid resolution supplied.";
				return false;
			}

			int width = atoi(argv[i + 1]);
			int height = atoi(argv[i + 2]);
			i += 2; // skip the argument value
			Settings::getInstance()->setInt("WindowWidth", width);
			Settings::getInstance()->setInt("WindowHeight", height);
		}else if(strcmp(argv[i], "--screensize") == 0)
		{
			if(i >= argc - 2)
			{
				std::cerr << "Invalid screensize supplied.";
				return false;
			}

			int width = atoi(argv[i + 1]);
			int height = atoi(argv[i + 2]);
			i += 2; // skip the argument value
			Settings::getInstance()->setInt("ScreenWidth", width);
			Settings::getInstance()->setInt("ScreenHeight", height);
		}else if(strcmp(argv[i], "--screenoffset") == 0)
		{
			if(i >= argc - 2)
			{
				std::cerr << "Invalid screenoffset supplied.";
				return false;
			}

			int x = atoi(argv[i + 1]);
			int y = atoi(argv[i + 2]);
			i += 2; // skip the argument value
			Settings::getInstance()->setInt("ScreenOffsetX", x);
			Settings::getInstance()->setInt("ScreenOffsetY", y);
		}else if (strcmp(argv[i], "--screenrotate") == 0)
		{
			if (i >= argc - 1)
			{
				std::cerr << "Invalid screenrotate supplied.";
				return false;
			}

			int rotate = atoi(argv[i + 1]);
			++i; // skip the argument value
			Settings::getInstance()->setInt("ScreenRotate", rotate);
		}else if(strcmp(argv[i], "--gamelist-only") == 0)
		{
			Settings::getInstance()->setBool("ParseGamelistOnly", true);
		}else if(strcmp(argv[i], "--ignore-gamelist") == 0)
		{
			Settings::getInstance()->setBool("IgnoreGamelist", true);
		}else if(strcmp(argv[i], "--show-hidden-files") == 0)
		{
			Settings::getInstance()->setBool("ShowHiddenFiles", true);
		}else if(strcmp(argv[i], "--draw-framerate") == 0)
		{
			Settings::getInstance()->setBool("DrawFramerate", true);
		}else if(strcmp(argv[i], "--no-exit") == 0)
		{
			Settings::getInstance()->setBool("ShowExit", false);
		}else if(strcmp(argv[i], "--no-confirm-quit") == 0)
		{
			Settings::getInstance()->setBool("ConfirmQuit", false);
		}else if(strcmp(argv[i], "--no-splash") == 0)
		{
			Settings::getInstance()->setBool("SplashScreen", false);
		}else if(strcmp(argv[i], "--debug") == 0)
		{
			Settings::getInstance()->setBool("Debug", true);
			Settings::getInstance()->setBool("HideConsole", false);
			Log::setReportingLevel(LogDebug);
		}else if(strcmp(argv[i], "--fullscreen-borderless") == 0)
		{
			Settings::getInstance()->setBool("FullscreenBorderless", true);
		}else if(strcmp(argv[i], "--windowed") == 0)
		{
			Settings::getInstance()->setBool("Windowed", true);
		}else if(strcmp(argv[i], "--vsync") == 0)
		{
			bool vsync = (strcmp(argv[i + 1], "on") == 0 || strcmp(argv[i + 1], "1") == 0) ? true : false;
			Settings::getInstance()->setBool("VSync", vsync);
			i++; // skip vsync value
		}else if(strcmp(argv[i], "--scrape") == 0)
		{
			scrape_cmdline = true;
		}else if(strcmp(argv[i], "--max-vram") == 0)
		{
			int maxVRAM = atoi(argv[i + 1]);
			Settings::getInstance()->setInt("MaxVRAM", maxVRAM);
		}
		else if (strcmp(argv[i], "--force-kiosk") == 0)
		{
			Settings::getInstance()->setBool("ForceKiosk", true);
		}
		else if (strcmp(argv[i], "--force-kid") == 0)
		{
			Settings::getInstance()->setBool("ForceKid", true);
		}
		else if (strcmp(argv[i], "--force-disable-filters") == 0)
		{
			Settings::getInstance()->setBool("ForceDisableFilters", true);
		}
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
#ifdef WIN32
			// This is a bit of a hack, but otherwise output will go to nowhere
			// when the application is compiled with the "WINDOWS" subsystem (which we usually are).
			// If you're an experienced Windows programmer and know how to do this
			// the right way, please submit a pull request!
			AttachConsole(ATTACH_PARENT_PROCESS);
			freopen("CONOUT$", "wb", stdout);
#endif
			std::cout <<
				"EmulationStation, a graphical front-end for ROM browsing.\n"
				"Written by Alec \"Aloshi\" Lofquist.\n"
				"Version " << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING << "\n\n"
				"Command line arguments:\n"
				"--resolution [width] [height]  try and force a particular resolution\n"
				"--screenrotate [n]             rotate a quarter turn clockwise for each n\n"
				"--screensize [width] [height]  for a canvas smaller than the full resolution,\n"
				"                               or if rotating into portrait mode\n"
				"--screenoffset [x] [y]         move the canvas by x,y pixels\n"
				"--gamelist-only                skip automatic game search, only read from gamelist.xml\n"
				"--ignore-gamelist              ignore the gamelist (useful for troubleshooting)\n"
				"--draw-framerate               display the framerate\n"
				"--no-exit                      don't show the exit option in the menu\n"
				"--no-confirm-quit              omit confirm dialog on actions of quit menu\n"
				"--no-splash                    don't show the splash screen\n"
				"--debug                        more logging, show console on Windows\n"
				"--scrape                       scrape using command line interface\n"
				"--windowed                     not fullscreen, should be used with --resolution\n"
				"--vsync [1/on or 0/off]        turn vsync on or off (default is on)\n"
				"--max-vram [size]              max VRAM to use in MB before swapping. 0 for unlimited\n"
				"--force-kid                    force the UI mode to be Kid\n"
				"--force-kiosk                  force the UI mode to be Kiosk\n"
				"--force-disable-filters        force the UI to ignore applied filters in gamelist\n"
				"--home [path]                  directory to use as home path\n"
				"--help, -h                     summon a sentient, angry tuba\n\n"
				"More information available in README.md.\n";
			return false; //exit after printing help
		}
	}

	return true;
}

bool verifyHomeFolderExists()
{
	//make sure the config directory exists
	std::string home = Utils::FileSystem::getHomePath();
	std::string configDir = home + "/.emulationstation";
	if(!Utils::FileSystem::exists(configDir))
	{
		std::cout << "Creating config directory \"" << configDir << "\"\n";
		Utils::FileSystem::createDirectory(configDir);
		if(!Utils::FileSystem::exists(configDir))
		{
			std::cerr << "Config directory could not be created!\n";
			return false;
		}
	}

	return true;
}

// Returns true if everything is OK,
bool loadSystemConfigFile(Window* window, const char** errorString)
{
	*errorString = NULL;

	if(!SystemData::loadConfig(window))
	{
		LOG(LogError) << "Error while parsing systems configuration file!";
		*errorString = "IT LOOKS LIKE YOUR SYSTEMS CONFIGURATION FILE HAS NOT BEEN SET UP OR IS INVALID. YOU'LL NEED TO DO THIS BY HAND, UNFORTUNATELY.\n\n"
			"VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";
		return false;
	}

	if(SystemData::sSystemVector.size() == 0)
	{
		LOG(LogError) << "No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)";
		*errorString = "WE CAN'T FIND ANY SYSTEMS!\n"
			"CHECK THAT YOUR PATHS ARE CORRECT IN THE SYSTEMS CONFIGURATION FILE, "
			"AND YOUR GAME DIRECTORY HAS AT LEAST ONE GAME WITH THE CORRECT EXTENSION.\n\n"
			"VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION.";
		return false;
	}

	return true;
}

//called on exit, assuming we get far enough to have the log initialized
void onExit()
{
	Log::close();
}

int main(int argc, char* argv[])
{
	srand((unsigned int)time(NULL));

	std::locale::global(std::locale("C"));

	if(!parseArgs(argc, argv))
		return 0;

	// only show the console on Windows if HideConsole is false
#ifdef WIN32
	// MSVC has a "SubSystem" option, with two primary options: "WINDOWS" and "CONSOLE".
	// In "WINDOWS" mode, no console is automatically created for us.  This is good,
	// because we can choose to only create the console window if the user explicitly
	// asks for it, preventing it from flashing open and then closing.
	// In "CONSOLE" mode, a console is always automatically created for us before we
	// enter main. In this case, we can only hide the console after the fact, which
	// will leave a brief flash.
	// TL;DR: You should compile ES under the "WINDOWS" subsystem.
	// I have no idea how this works with non-MSVC compilers.
	if(!Settings::getInstance()->getBool("HideConsole"))
	{
		// we want to show the console
		// if we're compiled in "CONSOLE" mode, this is already done.
		// if we're compiled in "WINDOWS" mode, no console is created for us automatically;
		// the user asked for one, so make one and then hook stdin/stdout/sterr up to it
		if(AllocConsole()) // should only pass in "WINDOWS" mode
		{
			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "wb", stdout);
			freopen("CONOUT$", "wb", stderr);
		}
	}else{
		// we want to hide the console
		// if we're compiled with the "WINDOWS" subsystem, this is already done.
		// if we're compiled with the "CONSOLE" subsystem, a console is already created;
		// it'll flash open, but we hide it nearly immediately
		if(GetConsoleWindow()) // should only pass in "CONSOLE" mode
			ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
#endif

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif

	//if ~/.emulationstation doesn't exist and cannot be created, bail
	if(!verifyHomeFolderExists())
		return 1;

	//start the logger
	Log::init();
	Log::open();
	LOG(LogInfo) << "EmulationStation - v" << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING;

	//always close the log on exit
	atexit(&onExit);

	Window window;
	SystemScreenSaver screensaver(&window);
	PowerSaver::init();
	ViewController::init(&window);
	CollectionSystemManager::init(&window);
	MameNames::init();
	window.pushGui(ViewController::get());

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

	if(!scrape_cmdline)
	{
		if(!window.init())
		{
			LOG(LogError) << "Window failed to initialize!";
			return 1;
		}

		if (splashScreen)
		{
			std::string progressText = "Loading...";
			if (splashScreenProgress)
				progressText = "Loading system config...";
			window.renderLoadingScreen(progressText);
		}
	}

	const char* errorMsg = NULL;
	if(!loadSystemConfigFile(splashScreen && splashScreenProgress ? &window : nullptr, &errorMsg))
	{
		// something went terribly wrong
		if(errorMsg == NULL)
		{
			LOG(LogError) << "Unknown error occured while parsing system config file.";
			if(!scrape_cmdline)
				Renderer::deinit();
			return 1;
		}

		// we can't handle es_systems.cfg file problems inside ES itself, so display the error message then quit
		window.pushGui(new GuiMsgBox(&window,
			errorMsg,
			"QUIT", [] {
				SDL_Event* quit = new SDL_Event();
				quit->type = SDL_QUIT;
				SDL_PushEvent(quit);
			}));
	}

	//run the command line scraper then quit
	if(scrape_cmdline)
	{
		return run_scraper_cmdline();
	}

	// preload what we can right away instead of waiting for the user to select it
	// this makes for no delays when accessing content, but a longer startup time
	ViewController::get()->preload();

	if(splashScreen && splashScreenProgress)
		window.renderLoadingScreen("Done.");

	//choose which GUI to open depending on if an input configuration already exists
	if(errorMsg == NULL)
	{
		if(Utils::FileSystem::exists(InputManager::getConfigPath()) && InputManager::getInstance()->getNumConfiguredDevices() > 0)
		{
			ViewController::get()->goToStart();
		}else{
			window.pushGui(new GuiDetectDevice(&window, true, [] { ViewController::get()->goToStart(); }));
		}
	}

	// flush any queued events before showing the UI and starting the input handling loop
	const Uint32 event_list[] = {
			SDL_JOYAXISMOTION, SDL_JOYBALLMOTION, SDL_JOYHATMOTION, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP,
			SDL_KEYDOWN, SDL_KEYUP
		};
	SDL_PumpEvents();
	for(Uint32 ev_type: event_list) {
		SDL_FlushEvent(ev_type);
	}

	int lastTime = SDL_GetTicks();
	int ps_time = SDL_GetTicks();

	bool running = true;

	while(running)
	{
		SDL_Event event;
		bool ps_standby = PowerSaver::getState() && (int) SDL_GetTicks() - ps_time > PowerSaver::getMode();

		if(ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout()) : SDL_PollEvent(&event))
		{
			do
			{
				InputManager::getInstance()->parseEvent(event, &window);

				if(event.type == SDL_QUIT)
					running = false;
			} while(SDL_PollEvent(&event));

			// triggered if exiting from SDL_WaitEvent due to event
			if (ps_standby)
				// show as if continuing from last event
				lastTime = SDL_GetTicks();

			// reset counter
			ps_time = SDL_GetTicks();
		}
		else if (ps_standby)
		{
			// If exitting SDL_WaitEventTimeout due to timeout. Trail considering
			// timeout as an event
			ps_time = SDL_GetTicks();
		}

		if(window.isSleeping())
		{
			lastTime = SDL_GetTicks();
			SDL_Delay(1); // this doesn't need to be accurate, we're just giving up our CPU time until something wakes us up
			continue;
		}

		int curTime = SDL_GetTicks();
		int deltaTime = curTime - lastTime;
		lastTime = curTime;

		// cap deltaTime if it ever goes negative
		if(deltaTime < 0)
			deltaTime = 1000;

		window.update(deltaTime);
		window.render();
		Renderer::swapBuffers();

		Log::flush();
	}

	while(window.peekGui() != ViewController::get())
		delete window.peekGui();
	window.deinit();

	MameNames::deinit();
	CollectionSystemManager::deinit();
	SystemData::deleteSystems();

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_DeInitialise();
#endif

	processQuitMode();

	ProfileDump();

	LOG(LogInfo) << "EmulationStation cleanly shutting down.";

	return 0;
}
