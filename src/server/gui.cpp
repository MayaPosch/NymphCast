/*
	gui.h - Source file for the GUI module.
	
	Revision 0
	
	2021/06/01, Maya Posch
*/


#include "gui.h"


#include "ffplay/sdl_renderer.h"

#include "gui/core/Log.h"
#include "gui/core/Settings.h"
#include "gui/core/platform.h"
#include "gui/core/PowerSaver.h"
#include "gui/core/InputManager.h"
#include "gui/app/SystemData.h"
#include "gui/core/guis/GuiDetectDevice.h"
#include "gui/core/guis/GuiMsgBox.h"
#include "gui/core/utils/FileSystemUtil.h"
#include "gui/core/utils/ProfilingUtil.h"
#include "gui/app/views/ViewController.h"
#include "CollectionSystemManager.h"
#include "MameNames.h"

#include <SDL_main.h>
#include <SDL_timer.h>

#include <chrono>


// Static definitions.
std::thread* Gui::guiThread = 0;
std::atomic<bool> Gui::running = { false };
Window Gui::window;
SystemScreenSaver* Gui::screensaver = 0;
std::string Gui::resourceFolder;
NymphCastClient* Gui::client = 0;
std::condition_variable Gui::resumeCv;
std::mutex Gui::resumeMtx;
std::atomic<bool> Gui::active;
uint32_t Gui::windowId = 0;
bool Gui::ps_standby;
int Gui::lastTime;


bool Gui::init(std::string resFolder) {
	resourceFolder = resFolder;
	
	client = new NymphCastClient;
	
	if (resFolder.empty()) {
		//if (!Utils::FileSystem::exists(configDir)) {
			//
		//}
		
		// If ~/.emulationstation doesn't exist and cannot be created, return false;
		if (!verifyHomeFolderExists()) {
			// Clear home folder setting.
			Utils::FileSystem::setHomePath(resFolder);
			
			// Set home path.
			// Use this setting if home folder doesn't contain the .emulationstation folder?
			std::string exepath = Utils::FileSystem::getCWDPath();
			
			LOG(LogInfo) << "Attempt to use CWD path: " << exepath;
			Utils::FileSystem::setExePath(exepath);
		}
	}
	else {
		LOG(LogInfo) << "Setting exe path to: " << resFolder;
		Utils::FileSystem::setExePath(resFolder);
	}
	
	screensaver = new SystemScreenSaver(&window);
	PowerSaver::init();
	ViewController::init(&window);
	CollectionSystemManager::init(&window);
	MameNames::init();
	window.pushGui(ViewController::get());
	
	if (!window.init()) {
		LOG(LogError) << "Window failed to initialize!";
		return false;
	}

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");
	if (splashScreen) {
		std::string progressText = "Loading...";
		if (splashScreenProgress) {
			progressText = "Loading system config...";
		}
		
		window.renderLoadingScreen(progressText);
	}

	if (!SystemData::loadConfig(&window)) {
		LOG(LogError) << "Error while parsing systems configuration file!";
		return false;
	}

	if (SystemData::sSystemVector.size() == 0) {
		LOG(LogError) << "No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)";
		return false;
	}
	
	active = false;
	
	return true;
}


// --- VERIFY HOME FOLDER EXISTS ---
bool Gui::verifyHomeFolderExists() {
	//make sure the config directory exists
	std::string home = Utils::FileSystem::getHomePath();
	std::string configDir = home + "/.emulationstation/resources";
	
	LOG(LogInfo) << "Checking config dir: " << configDir;
	
	if (!Utils::FileSystem::exists(configDir)) {
		/* std::cout << "Creating config directory \"" << configDir << "\"\n";
		Utils::FileSystem::createDirectory(configDir);
		if (!Utils::FileSystem::exists(configDir)) {
			std::cerr << "Config directory could not be created!\n";
			return false;
		} */
		
		LOG(LogInfo) << "Config dir doesn't exist.";
		
		return false;
	}

	return true;
}


// --- START ---
bool Gui::start() {
	LOG(LogInfo) << "NymphCast GUI - " << __VERSION;

	bool splashScreen = Settings::getInstance()->getBool("SplashScreen");
	bool splashScreenProgress = Settings::getInstance()->getBool("SplashScreenProgress");

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
	
	// Get the window ID.
	windowId = Renderer::getWindowId();

	if(splashScreen && splashScreenProgress) {
		window.renderLoadingScreen("Done.");
	}

	//choose which GUI to open depending on if an input configuration already exists
	if (Utils::FileSystem::exists(InputManager::getConfigPath()) && 
				InputManager::getInstance()->getNumConfiguredDevices() > 0) {
		ViewController::get()->goToStart();
	}
	else {
		window.pushGui(new GuiDetectDevice(&window, true, [] { ViewController::get()->goToStart(); }));
	}

	// flush any queued events before showing the UI and starting the input handling loop
	/* const Uint32 event_list[] = {
			SDL_JOYAXISMOTION, SDL_JOYBALLMOTION, SDL_JOYHATMOTION, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP,
			SDL_KEYDOWN, SDL_KEYUP
		}; */
	//SDL_PumpEvents();
	/* for (Uint32 ev_type: event_list) {
		SDL_FlushEvent(ev_type);
	} */

	lastTime = SDL_GetTicks();
	//int ps_time = SDL_GetTicks();
	
	//guiThread = new std::thread(run_updates);
	
	active = true;
	
	// Enable events for the GUI.
	SdlRenderer::guiEvents(true);
	
	// --------------

	/* running = true;
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
	} */
	
	// --------------

	/* while(window.peekGui() != ViewController::get()) {
		delete window.peekGui();
	}
	
	window.deinit();

	MameNames::deinit();
	CollectionSystemManager::deinit();
	SystemData::deleteSystems();
	
	active = false; */
	
	return true;
}


// --- HANDLE EVENT ---
void Gui::handleEvent(SDL_Event &event) {
	// TODO: Adding this filter blocks many keyboard/controller events. Just remove it?
	//if (event.window.windowID != windowId) { return; }
	
	// TODO: move power saver stuff.
	

	//if (ps_standby ? SDL_WaitEventTimeout(&event, PowerSaver::getTimeout()) : SDL_PollEvent(&event)) {
		//do {
			
			InputManager::getInstance()->parseEvent(event, &window);

			/* if (event.type == SDL_QUIT) {
				running = false;
			} */
		//} 
		//while(SDL_PollEvent(&event));

		// triggered if exiting from SDL_WaitEvent due to event
		//if (ps_standby) {
			// show as if continuing from last event
			//lastTime = SDL_GetTicks();
		//}

		// reset counter
		//ps_time = SDL_GetTicks();
	/* }
	else if (ps_standby) {
		// If exitting SDL_WaitEventTimeout due to timeout. Trail considering
		// timeout as an event
		ps_time = SDL_GetTicks();
	} */

	/* if (window.isSleeping()) {
		lastTime = SDL_GetTicks();
		SDL_Delay(1); // this doesn't need to be accurate, we're just giving up our CPU time until something wakes us up
		return;
	} */

	/* int curTime = SDL_GetTicks();
	int deltaTime = curTime - lastTime;
	lastTime = curTime;

	// cap deltaTime if it ever goes negative
	if (deltaTime < 0) { deltaTime = 1000; }

	window.update(deltaTime);
	window.render();
	Renderer::swapBuffers();

	Log::flush(); */
}


// --- RUN UPDATES ---
void Gui::run_updates() {
	// TODO: handle power saving feature in a more global manner.
	//bool ps_standby = PowerSaver::getState() && (int) SDL_GetTicks() - ps_time > PowerSaver::getMode();
	bool ps_standby = false;
	
	int curTime = SDL_GetTicks();
	int deltaTime = curTime - lastTime;
	lastTime = curTime;

	// cap deltaTime if it ever goes negative
	if (deltaTime < 0) { deltaTime = 1000; }
	window.update(deltaTime);
	window.render();
	Renderer::swapBuffers();;
	Log::flush();
}


// --- STOP ---
bool Gui::stop() {
	LOG(LogInfo) << "Stopping the NymphCast GUI..";
	
	SdlRenderer::guiEvents(false);
	
	while(window.peekGui() != ViewController::get()) {
		delete window.peekGui();
	}
	
	window.deinit();

	MameNames::deinit();
	CollectionSystemManager::deinit();
	SystemData::deleteSystems();
	
	running = false;
	active = false;
	
	return true;
}


// --- QUIT ---
bool Gui::quit() {
	LOG(LogInfo) << "Quitting the NymphCast GUI...";
	
	if (screensaver) {
		delete screensaver;
	}
	
	if (client) {
		delete client;
	}
	
	return true;
}
