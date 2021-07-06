/*
	gui.h - Source file for the GUI module.
	
	Revision 0
	
	2021/06/01, Maya Posch
*/


#include "gui.h"


//#include "ffplay/sdl_renderer.h"

#include "gui/core/Log.h"
#include "gui/core/Settings.h"
#include "gui/core/platform.h"
#include "gui/core/PowerSaver.h"
#include "gui/core/InputManager.h"
#include "gui/app/SystemData.h"
#include "gui/app/SystemScreenSaver.h"
#include "gui/core/guis/GuiDetectDevice.h"
#include "gui/core/guis/GuiMsgBox.h"
#include "gui/core/utils/FileSystemUtil.h"
#include "gui/core/utils/ProfilingUtil.h"
#include "gui/app/views/ViewController.h"

#include <SDL_events.h>
#include <SDL_main.h>
#include <SDL_timer.h>

//#include <GL/glew.h>


// Static definitions.
std::thread* Gui::sdl = 0;
std::atomic<bool> Gui::running = false;
Window Gui::window;
NymphCastClient Gui::client;
std::condition_variable Gui::resumeCv;
std::mutex Gui::resumeMtx;
std::atomic<bool> Gui::active;


bool Gui::init(std::string document) {
	//SdlRenderer::initGui(document);

	if (!SystemData::loadConfig(&window)) {
		LOG(LogError) << "Error while parsing systems configuration file!";
		return false;
	}

	if (SystemData::sSystemVector.size() == 0) {
		LOG(LogError) << "No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)";
		return false;
	}
	
	active = false;
	
	/* if (!loadSystemConfigFile(splashScreen && splashScreenProgress ? &window : nullptr, &errorMsg)) {
		// something went terribly wrong
		if (errorMsg == NULL) {
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
	} */
	
	return true;
}


// --- START ---
bool Gui::start() {
	//sdl = new std::thread(SdlRenderer::run_gui_loop);
	
	LOG(LogInfo) << "NymphCast GUI - v" << __VERSION;
	
	SystemScreenSaver screensaver(&window);
	PowerSaver::init();
	ViewController::init(&window);
	//CollectionSystemManager::init(&window);
	//MameNames::init();
	window.pushGui(ViewController::get());

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");
	
	if(!window.init()) {
		LOG(LogError) << "Window failed to initialize!";
		return false;
	}

	if (splashScreen) {
		std::string progressText = "Loading...";
		if (splashScreenProgress) {
			progressText = "Loading system config...";
		}
		
		window.renderLoadingScreen(progressText);
	}

	// preload what we can right away instead of waiting for the user to select it
	// this makes for no delays when accessing content, but a longer startup time
	ViewController::get()->preload();

	if(splashScreen && splashScreenProgress) {
		window.renderLoadingScreen("Done.");
	}

	//choose which GUI to open depending on if an input configuration already exists
	//if(errorMsg == NULL)
	//{
	if (Utils::FileSystem::exists(InputManager::getConfigPath()) && 
				InputManager::getInstance()->getNumConfiguredDevices() > 0) {
		ViewController::get()->goToStart();
	}
	else {
		window.pushGui(new GuiDetectDevice(&window, true, [] { ViewController::get()->goToStart(); }));
	}
	//}

	// flush any queued events before showing the UI and starting the input handling loop
	const Uint32 event_list[] = {
			SDL_JOYAXISMOTION, SDL_JOYBALLMOTION, SDL_JOYHATMOTION, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP,
			SDL_KEYDOWN, SDL_KEYUP
		};
	SDL_PumpEvents();
	for (Uint32 ev_type: event_list) {
		SDL_FlushEvent(ev_type);
	}

	int lastTime = SDL_GetTicks();
	int ps_time = SDL_GetTicks();
	
	active = true;

	running = true;
	while (running) {
		SDL_Event event;
		bool ps_standby = PowerSaver::getState() && (int) SDL_GetTicks() - ps_time > PowerSaver::getMode();

		if (ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout()) : SDL_PollEvent(&event)) {
			do {
				InputManager::getInstance()->parseEvent(event, &window);

				if (event.type == SDL_QUIT) {
					running = false;
				}
			} 
			while(SDL_PollEvent(&event));

			// triggered if exiting from SDL_WaitEvent due to event
			if (ps_standby)
				// show as if continuing from last event
				lastTime = SDL_GetTicks();

			// reset counter
			ps_time = SDL_GetTicks();
		}
		else if (ps_standby) {
			// If exitting SDL_WaitEventTimeout due to timeout. Trail considering
			// timeout as an event
			ps_time = SDL_GetTicks();
		}

		if (window.isSleeping()) {
			lastTime = SDL_GetTicks();
			SDL_Delay(1); // this doesn't need to be accurate, we're just giving up our CPU time until something wakes us up
			continue;
		}

		int curTime = SDL_GetTicks();
		int deltaTime = curTime - lastTime;
		lastTime = curTime;

		// cap deltaTime if it ever goes negative
		if (deltaTime < 0) { deltaTime = 1000; }

		window.update(deltaTime);
		window.render();
		Renderer::swapBuffers();

		Log::flush();
	}

	while(window.peekGui() != ViewController::get()) {
		delete window.peekGui();
	}
	
	window.deinit();
	
	active = false;
	
	return true;
}


// --- STOP ---
bool Gui::stop() {
	//SdlRenderer::stop_gui_loop();
	
	// Wait for thread to rejoin.
	//sdl->join();
	
	// Delete thread object.
	//delete sdl;
	
	running = false;
	active = false;
	
	return true;
}


// --- QUIT ---
bool Gui::quit() {
	//SdlRenderer::quitGui();
	
	return true;
}
