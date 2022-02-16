/*
	nc_apps.h - Header for the NymphCast Apps module header.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			-
			
	2021/05/30, Maya Posch
*/


#include "nc_apps.h"

#include <Poco/URI.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>
#include <Poco/Timestamp.h>

#include <nymph/nymph.h>
#include "databuffer.h"

#include <angelscript/json/json.h>
#include <angelscript/regexp/regexp.h>

#include <fstream>
#include <streambuf>

// Debug
#include <iostream>


// Static initialisations.
std::string NCApps::appsFolder;
std::string NCApps::activeAppId;
std::mutex NCApps::activeAppMutex;


// --- CONSTRUCTOR ---
NCApps::NCApps() {
	// Create the script engine
	engine = asCreateScriptEngine();
	if (engine == 0) {
		std::cout << "Failed to create script engine." << std::endl;
		return;
	}
	
	// The script compiler will write any compiler messages to the callback.
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
	
	// Register the script string type
	RegisterStdString(engine);
	RegisterScriptArray(engine, false);
	RegisterStdStringUtils(engine);
	
	// Register functions.
	int r;
	r = engine->RegisterGlobalFunction(
								"bool performHttpQuery(string, string &out)", 
								asFUNCTION(NCApps::performHttpQuery), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool performHttpsQuery(string, string &out)", 
								asFUNCTION(NCApps::performHttpsQuery), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"void clientSend(int, string)", 
								asFUNCTION(NCApps::clientSend), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool streamTrack(string)", 
								asFUNCTION(NCApps::streamTrack), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool readValue(string, string &out, uint64)", 
								asFUNCTION(NCApps::readValue), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool storeValue(string, string &in)", 
								asFUNCTION(NCApps::storeValue), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool readTemplate(string, string &in)", 
								asFUNCTION(NCApps::readTemplate), asCALL_CDECL);
								
	// Register further modules.
	initJson(engine);
	initRegExp(engine);
}


// --- DESTRUCTOR ---
NCApps::~NCApps() {
	//
}


// --- SET APPS FOLDER ---
void NCApps::setAppsFolder(std::string folder) {
	appsFolder = folder;
}


// --- ADD APP ---
bool NCApps::addApp(std::string name, NymphCastApp app) {
	mutex.lock();
	if (apps.find(name) != apps.end()) { 
		mutex.unlock(); 
		return false;  // Return if name exists.
	}
	
	apps.insert(std::pair<std::string, NymphCastApp>(name, app));
	names.push_back(name);
	mutex.unlock();
	
	return true;
}


// --- REMOVE APP ---
bool NCApps::removeApp(std::string name) {
	std::map<std::string, NymphCastApp>::iterator it;
	mutex.lock();
	it = apps.find(name);
	if (it == apps.end()) { 
		mutex.unlock();
		return false; 	// Return if app name not found.
	}
	
	apps.erase(it);
	
	for (int i = 0; i < names.size(); ++i) {
		if (names[i] == name) {
			names.erase(names.begin() + i);
		}
	}
	
	mutex.unlock();
	
	return true;
}


// --- FIND APP ---	
NymphCastApp& NCApps::findApp(std::string name) {
	std::map<std::string, NymphCastApp>::iterator it;
	mutex.lock();
	it = apps.find(name);
	if (it == apps.end()) { 
		mutex.unlock();
		return defaultApp; 	// Return if app name not found.
	}
	
	NymphCastApp& app = it->second;
	mutex.unlock();
	
	return app;
}


// --- READ APP LIST ---	
bool NCApps::readAppList(std::string path) {
	INIReader appList(path);
	if (appList.ParseError() != 0) {
		std::cerr << "Failed to parse the '" << path << "' file." << std::endl;
		return false;
	}
	
	std::set<std::string> sections = appList.Sections();
	
	// Read out the information per app section.
	NymphCastApp app;
	std::set<std::string>::const_iterator it;
	for (it = sections.cbegin(); it != sections.cend(); ++it) {
		app.id = appList.Get(*it, "name", "");
		if (app.id.empty()) {
			std::cerr << "App name was empty. Skipping..." << std::endl;
			continue;
		}
		
		std::string loc = appList.Get(*it, "location", "");
		if (loc == "local") {
			app.location = NYMPHCAST_APP_LOCATION_LOCAL;
		}
		else if (loc == "remote") {
			app.location = NYMPHCAST_APP_LOCATION_HTTP;
		}
		else {
			// Default is to ignore this app.
			std::cerr << "Invalid location for app " << app.id << ". Ignoring..." << std::endl;
			continue;
		}
		
		app.url = appList.Get(*it, "url", "");
		if (app.url.empty()) {
			std::cerr << "Invalid URL for app " << app.id << std::endl;
			continue; 
		}
		
		std::cout << "Adding app: " << app.id << std::endl;
		
		if (!addApp(app.id, app)) { return false; }
	}
	
	if (apps.size() > 0) {
		defaultApp = apps.begin()->second;
	}
	
	// TODO: Update apps.html file using the template if apps.ini is newer than the HTML file.
	
	
	return true;
}


// --- APP NAMES ---
std::vector<std::string> NCApps::appNames() {
	return names;
}


// --- MESSAGE CALLBACK ---
// Angel Script runtime callback for messages.
void NCApps::MessageCallback(const asSMessageInfo *msg, void *param) {
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING )
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = "INFO";

	std::cout << msg->section << " (" << msg->row << ", " << msg->col << ") : " << type << " : " 
				<< msg->message << std::endl;
}


std::chrono::time_point<std::chrono::steady_clock> NCApps::timeGetTime() {
	std::chrono::time_point<std::chrono::steady_clock> now;
	now = std::chrono::steady_clock::now();
	return now;
}


void NCApps::LineCallback(asIScriptContext* ctx, std::chrono::time_point<std::chrono::steady_clock>* timeOut) {
	// If the time out is reached we abort the script
	if (*timeOut < NCApps::timeGetTime()) {
		ctx->Abort();
	}

	// It would also be possible to only suspend the script,
	// instead of aborting it. That would allow the application
	// to resume the execution where it left of at a later 
	// time, by simply calling Execute() again.
}


// --- CLIENT SEND ---
// Send a message to a client by a NymphCast app.
void NCApps::clientSend(uint32_t id, std::string message) {
	// Send a message to a client for an app, if the cliend ID exists.
	std::vector<NymphType*> values;
	std::string result;
	if (!NymphRemoteClient::callCallback(DataBuffer::getSessionHandle(), "ReceiveFromAppCallback", 
																				values, result)) {
		std::cerr << "Calling callback failed: " << result << std::endl;
		return;
	}
}


// --- PERFORM HTTP QUERY ---
bool NCApps::performHttpQuery(std::string query, std::string &response) {
	// Create Poco HTTP query, send it off, wait for response.
	Poco::URI uri(query);
	std::string path(uri.getPathAndQuery());
	if (path.empty()) { path = "/"; }
	
	Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, 
											Poco::Net::HTTPMessage::HTTP_1_1);
	session.sendRequest(req);
	Poco::Net::HTTPResponse httpResponse;
	std::istream& rs = session.receiveResponse(httpResponse);
	std::cout << httpResponse.getStatus() << " " << httpResponse.getReason() << std::endl;
	if (httpResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
		Poco::StreamCopier::copyToString(rs, response);
		return true;
	}
	else {
		// Something went wrong.
		return false;
	}
	
	
	return true;
}


// --- PERFORM HTTPS QUERY ---
bool NCApps::performHttpsQuery(std::string query, std::string &response) {
	// Create Poco HTTPS query, send it off, wait for response.
	Poco::URI uri(query);
	std::string path(uri.getPathAndQuery());
	if (path.empty()) { path = "/"; }
	
	// Debug:
	std::cout << "HTTPS query: " << query << std::endl;
	
	const Poco::Net::Context::Ptr context = new Poco::Net::Context(
		Poco::Net::Context::CLIENT_USE, "", "", "",
		Poco::Net::Context::VERIFY_RELAXED, 9, false,
		"ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	
	Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
	//Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort());
	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, 
											Poco::Net::HTTPMessage::HTTP_1_1);
	req.setContentLength(0);
	req.set("user-agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:39.0) Gecko/20100101 Firefox/75.0");
	
	try {
		session.sendRequest(req);
	} 
	catch (Poco::Exception& exc) {
		std::cout << "Exception caught while attempting to connect." << std::endl;
		std::cerr << exc.displayText() << std::endl;
		return false;
	}
	
	Poco::Net::HTTPResponse httpResponse;
	std::istream& rs = session.receiveResponse(httpResponse);
	
	//
	std::cout << httpResponse.getStatus() << " " << httpResponse.getReason() << std::endl;
	
	if (httpResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
		Poco::StreamCopier::copyToString(rs, response);
		
		// Debug
		std::cout << "HTTP response:\n---------------\n";
		std::cout << response;
		std::cout << "\n---------------\n";
		
		return true;
	}
	else {
		// Something went wrong.
		Poco::StreamCopier::copyToString(rs, response);
		std::cout << "HTTPS query failed. Path: " << path << ". Query: " << query << std::endl;
		std::cout << "Response: \n" << response << std::endl;
		return false;
	}
	
	
	return true;
}


// --- STREAM TRACK ---
// Attempt to stream from the indicated URL.
bool NCApps::streamTrack(std::string url) {
	// FIXME: Call global function. Could be cleaner.
	return ::streamTrack(url);
}


// --- STORE VALUE ---
// App-level storage: store a single key/value pair for an NC app.
bool NCApps::storeValue(std::string key, std::string &value) {
	Poco::Data::Session db("SQLite", appsFolder + activeAppId + "/" + activeAppId + ".db");
	
	std::cout << "Opened database file for " << activeAppId << std::endl;
	
	// Create table if it doesn't exist yet.
	try {
		db << "CREATE TABLE IF NOT EXISTS data (id TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL, updated INTEGER)", 
			Poco::Data::Keywords::now;
	}
	catch(Poco::Data::SQLite::SQLiteException &exc) {
		std::cout << "SQL error: " << exc.displayText() << std::endl;
	}
	catch (...) {
		std::cerr << "Creating table failed." << std::endl;
		return false;
	}
		
	std::cout << "Table 'data' exists." << std::endl;
		
	// Get the current timestamp for database insertion.
	Poco::Timestamp ts;
	int64_t updated = (int64_t) ts.epochMicroseconds();
	
	std::cout << "Writing value into database. Key: " << key << ", value: " << value << std::endl;
	
	// Write or update the key/value pair in the table.
	db << "INSERT OR REPLACE INTO data (id, value, updated) VALUES (:key, :value, :updated)",
		Poco::Data::Keywords::use(key),
		Poco::Data::Keywords::use(value),
		Poco::Data::Keywords::use(updated),
		Poco::Data::Keywords::now;
		
	std::cout << "Updated table." << std::endl;
		
	return true;
}


// --- READ VALUE ---
// App-level storage: read a single value given a key for an NC app.
// The 'age' parameter (in microseconds) sets the maximum allowed age of the value since its last
// update. Omitting it or setting it to 0 means that any age is acceptable.
bool NCApps::readValue(std::string key, std::string &value, uint64_t age) {
	Poco::Data::Session db("SQLite", appsFolder + activeAppId + "/" + activeAppId + ".db");
	
	std::cout << "Opened database file for " << activeAppId << std::endl;
	
	// Return false if the table doesn't exist.
	Poco::Data::Statement statement = db << 
							"SELECT name FROM sqlite_master WHERE type='table' AND name='data'";
	try {
		while (!statement.done()) { statement.execute(); }
	}
	catch(Poco::Data::SQLite::SQLiteException &exc) {
		std::cout << "SQL error: " << exc.displayText() << std::endl;
	}
	catch(...) {
		std::cout << "Failed to execute query." << std::endl;
		return false;
	}
	
	if (statement.rowsExtracted() < 1) { 
		std::cout << "Table 'data' does not exist. Returning..." << std::endl;
		return false;
	}
	
	// Read out value and return true.
	int64_t updated = 0;
	db << "SELECT value, updated FROM data WHERE id=:key", 
		Poco::Data::Keywords::use(key), 
		Poco::Data::Keywords::into(value),
		Poco::Data::Keywords::into(updated),
		Poco::Data::Keywords::now;
		
	if (updated == 0) { 
		std::cout << "Updated: 0, key: " << key << ", value: " << value << std::endl;
		return false;
	}
		
	// If 'age' parameter has been set, check whether value has expired.
	Poco::Timestamp ts;
	Poco::Timestamp uts(updated);
	if ((uts + age) < ts) {
		std::cout << "Value was older than age. Dropping..." << std::endl;
		return false;
	}
	
	return true;
}


// --- READ TEMPLATE ---
bool NCApps::readTemplate(std::string name, std::string &contents) {
	// Try to read indicated file from the active app's template folder.
	std::ifstream t(appsFolder + activeAppId + "/" + name);
	if (!t.is_open()) { return false; }
	
	t.seekg(0, std::ios::end);   
	contents.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	contents.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	
	return true;
}


// --- RUN APP ---
// string app_send(string appId, string data)
bool NCApps::runApp(std::string name, std::string message, uint8_t format, std::string &result) {
	NymphCastApp app = findApp(name);
	if (app.id.empty()) { 
		std::cerr << "Failed to find a matching application for '" << name << "'." << std::endl;
		result = "Failed to find a matching application for '" + name + "'.";
		return false; 
	}
	
	// Update active app ID.
	activeAppMutex.lock();
	activeAppId = name;
	activeAppMutex.unlock();
	
	// Compile the app if it hasn't been compiled yet.
	if (app.asFunction == 0) {
		// Initialise new instance of the app.
		int r;
		
		std::cout << "Loading " << app.id << " app..." << std::endl;

		std::string script;
		int len;
		if (app.location == NYMPHCAST_APP_LOCATION_LOCAL) {
			// We will load the script from a file on the disk.
			FILE *f = fopen((appsFolder + app.url).c_str(), "rb");
			if (f == 0) {
				std::cout << "Failed to open the script file '" << app.url << "'." << std::endl;
				result = "Failed to open the script file.";
				return false;
			}

			// Determine the size of the file	
			fseek(f, 0, SEEK_END);
			len = ftell(f);
			fseek(f, 0, SEEK_SET);

			// On Win32 it is possible to do the following instead
			// int len = _filelength(_fileno(f));

			// Read the entire file
			script.resize(len);
			size_t c = fread(&script[0], len, 1, f);
			fclose(f);

			if (c == 0) {
				std::cerr << "Failed to load script file." << std::endl;
				result = "Failed to load script file.";
				return false;
			}
		}
		else if (app.location == NYMPHCAST_APP_LOCATION_HTTP) {
			// Load the script file from a remote location (HTTP or HTTPS).			
			// Determine whether to call the HTTP or HTTPS function.
			std::string response;
			if (app.url.substr(0, 5) == "https") {
				std::string query = "";
				if (!performHttpsQuery(query, response)) {
					std::cerr << "Error while performing HTTPS query: " << query << std::endl;
					result = "Error while performing HTTPS query.";
					return false;
				}
			}
			else if (app.url.substr(0, 5) == "http:") {
				std::string query = "";
				if (!performHttpQuery(query, response)) {
					std::cerr << "Error while performing HTTP query: " << query << std::endl;
					result = "Error while performing HTTP query.";
					return false;
				}
			}
			
			// Response string should contain the script.
			script = response;
			len = script.length();
		}
		
		std::cout << "Creating module." << std::endl;

		// Add the script sections that will be compiled into executable code.
		// If we want to combine more than one file into the same script, then 
		// we can call AddScriptSection() several times for the same module and
		// the script engine will treat them all as if they were one. The script
		// section name, will allow us to localize any errors in the script code.
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->AddScriptSection("script", &script[0], len);
		if (r < 0) {
			std::cout << "AddScriptSection() failed" << std::endl;
			result = "AddScriptSection() failed";
			return false;
		}
		
		std::cout << "Compile script." << std::endl;
		
		// Compile the script. If there are any compiler messages they will
		// be written to the message stream that we set right after creating the 
		// script engine. If there are no errors, and no warnings, nothing will
		// be written to the stream.
		r = mod->Build();
		if (r < 0) {
			std::cout << "Build() failed" << std::endl;
			result = "Build() failed";
			return false;
		}

		// The engine doesn't keep a copy of the script sections after Build() has
		// returned. So if the script needs to be recompiled, then all the script
		// sections must be added again.

		// If we want to have several scripts executing at different times but 
		// that have no direct relation with each other, then we can compile them
		// into separate script modules. Each module use their own namespace and 
		// scope, so function names, and global variables will not conflict with
		// each other.
		
		std::cout << "Creating context." << std::endl;
		
		// Create a context that will execute the script.
		app.asContext = engine->CreateContext();
		if (app.asContext == 0) {
			std::cout << "Failed to create the context." << std::endl;
			result = "Failed to create the context.";
			engine->Release();
			return false;
		}
		
		std::cout << "Setting line callback." << std::endl;

		// We don't want to allow the script to hang the application, e.g. with an
		// infinite loop, so we'll use the line callback function to set a timeout
		// that will abort the script after a certain time. Before executing the 
		// script the timeOut variable will be set to the time when the script must 
		// stop executing. 
		r = app.asContext->SetLineCallback(asFUNCTION(NCApps::LineCallback), &timeOut, asCALL_CDECL);
		if (r < 0) {
			std::cout << "Failed to set the line callback function." << std::endl;
			result = "Failed to set the line callback function.";
			app.asContext->Release();
			engine->Release();
			return false;
		}
		
		std::cout << "Find function." << std::endl;

		// Find the function for the function we want to execute.
		app.asFunction = engine->GetModule(0)->GetFunctionByDecl("string command_processor(string, int)");
		if (app.asFunction == 0) {
			std::cout << "The function 'string command_processor(string, int)' was not found." << std::endl;
			result = "The function 'string command_processor(string, int)' was not found.";
			app.asContext->Release();
			engine->Release();
			return false;
		}
		
		app.asHtmlFunction = engine->GetModule(0)->GetFunctionByDecl("string html_processor(string)");
		if (app.asFunction == 0) {
			std::cout << "The function 'string command_processor(string, int)' was not found." << std::endl;
			result = "The function 'string command_processor(string, int)' was not found.";
			app.asContext->Release();
			engine->Release();
			return false;
		}
	}
	
	std::cout << "Preparing script context." << std::endl;
				
	// Prepare the script context with the function we wish to execute. Prepare()
	// must be called on the context before each new script function that will be
	// executed. Note, that if you intend to execute the same function several 
	// times, it might be a good idea to store the function returned by 
	// GetFunctionByDecl(), so that this relatively slow call can be skipped.
	int r = 0;
	if (format == 1) {
		r = app.asContext->Prepare(app.asHtmlFunction);
	}
	else {
		r = app.asContext->Prepare(app.asFunction);
	}
	
	if (r < 0) {
		std::cout << "Failed to prepare the context." << std::endl;
		result = "Failed to prepare the context.";
		app.asContext->Release();
		engine->Release();
		return false;
	}
	
	std::cout << "Setting app arguments." << std::endl;
	
	// Pass string to app.
	app.asContext->SetArgObject(0, (void*) &message);
	//app.asContext->SetArgAddress(1, &format);
	
	// Set the timeout before executing the function. Give the function 30 seconds
	// to return before we'll abort it.
	timeOut = timeGetTime() + std::chrono::seconds(30);

	// Execute the function.
	std::cout << "Executing the script." << std::endl;
	std::cout << "---" << std::endl;
	r = app.asContext->Execute();
	std::cout << "---" << std::endl;
	if (r != asEXECUTION_FINISHED) {
		// The execution didn't finish as we had planned. Determine why.
		if (r == asEXECUTION_ABORTED) {
			std::cout << "The script was aborted before it could finish. Probably it timed out." 
						<< std::endl;
		}
		else if (r == asEXECUTION_EXCEPTION) {
			std::cout << "The script ended with an exception." << std::endl;

			// Write some information about the script exception
			asIScriptFunction* func = app.asContext->GetExceptionFunction();
			std::cout << "func: " << func->GetDeclaration() << std::endl;
			std::cout << "modl: " << func->GetModuleName() << std::endl;
			std::cout << "sect: " << func->GetScriptSectionName() << std::endl;
			std::cout << "line: " << app.asContext->GetExceptionLineNumber() << std::endl;
			std::cout << "desc: " << app.asContext->GetExceptionString() << std::endl;
		}
		else
			std::cout << "The script ended for some unforeseen reason (" << r << ")." 
						<< std::endl;
	}
	else {
		// Retrieve the return value from the context
		result = *(std::string*) app.asContext->GetReturnObject();
		std::cout << "The script function returned: " << result << std::endl;
	}
	
	return true;
}

