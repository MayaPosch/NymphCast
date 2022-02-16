/*
	nc_apps.h - Header for the NymphCast Apps module header.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			-
			
	2021/05/30, Maya Posch
*/


#ifndef NC_APPS_H
#define NC_APPS_H


#include <string>
#include <chrono>
#include <mutex>
#include <map>
#include <vector>

#include <angelscript.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptarray/scriptarray.h>

#include "INIReader.h"

#include "nymphcast_client.h"

// Forward declarations.
bool streamTrack(std::string url);


enum NymphCastAppLocation {
	NYMPHCAST_APP_LOCATION_LOCAL = 1,
	NYMPHCAST_APP_LOCATION_HTTP = 2
};


struct NymphCastApp {
	std::string id;
	NymphCastAppLocation location;
	std::string url;
	
	asIScriptContext* asContext = 0;
	asIScriptFunction* asFunction = 0;
	asIScriptFunction* asHtmlFunction = 0;
};


class NCApps {
	std::map<std::string, NymphCastApp> apps;
	std::vector<std::string> names;
	std::mutex mutex;
	NymphCastApp defaultApp;
	asIScriptEngine* engine = 0;
	asIScriptContext* soundcloudContext = 0;
	asIScriptFunction* soundcloudFunction = 0;
	std::chrono::time_point<std::chrono::steady_clock> timeOut;
	static std::string appsFolder;
	static std::string activeAppId;
	static std::mutex activeAppMutex;
	
	static void MessageCallback(const asSMessageInfo *msg, void *param);
	static std::chrono::time_point<std::chrono::steady_clock> timeGetTime();
	static void LineCallback(asIScriptContext *ctx, std::chrono::time_point<std::chrono::steady_clock> *timeOut);
	
	static void clientSend(uint32_t id, std::string message);
	static bool performHttpQuery(std::string query, std::string &response);
	static bool performHttpsQuery(std::string query, std::string &response);
	static bool streamTrack(std::string url);
	static bool storeValue(std::string key, std::string &value);
	static bool readValue(std::string key, std::string &value, uint64_t age = 0);
	static bool readTemplate(std::string name, std::string &contents);
	
public:
	NCApps();
	~NCApps();
	
	void setAppsFolder(std::string folder);
	bool addApp(std::string name, NymphCastApp app);
	bool removeApp(std::string name);
	NymphCastApp& findApp(std::string name);
	bool readAppList(std::string path);
	std::vector<std::string> appNames();
	
	bool runApp(std::string name, std::string message, uint8_t format, std::string &result);
};


#endif
