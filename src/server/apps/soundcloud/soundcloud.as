

string clientId;
//const string baseUrl = "https://api.soundcloud.com";
const string baseUrl = "https://api-v2.soundcloud.com";


string start() {
	// Return the available features.
	
	return "";
}


// --- GET CLIENT ID ---
// Get a fresh client ID from the SoundCloud server.
// Returns an empty string if failed.
string getClientId() {
	// Obtain the JavaScript file containing the client ID.
	string query = "https://soundcloud.com/discover";
	string response;
	string id;
	if (!performHttpsQuery(query, response)) {
		return id;
	}
	
	// Parse the client ID from the JS file. This requires that we find the right JS file that
	// contains this information.
	array<string> matches;
	RegExp re;
	re.createRegExp("=\"(https://a-v2\.sndcdn\.com/assets/.*.js)\"");
	RegExp kre;
	kre.createRegExp("exports={\"api-v2\".*client_id:\"(\w*)\"");
	int n = re.findall(response, matches);
	if (n == 0) { return id; }
	
	for (int i = 0; i < matches.length(); ++i) {
		string js;
		if (!performHttpsQuery(matches[i], js)) {
			return id;
		}
		
		// Extract the ID.
		if (kre.extract(js, id, 0) == 1) {
			return id;	// We're done.
		}
	}
	
	// No client ID found.
	return id;
}


// --- UPDATE CLIENT ID ---
bool updateClientId() {
	// Check that the cached client ID is still current.
	string key = "clientId";
	uint64 age = 86400000000; // 24 hours in microseconds.
	if (readValue(key, clientId, age)) { return true; }
	
	// The client ID either wasn't found in the KV store, or expired.
	// Get a fresh client ID from the remote.
	clientId = getClientId();
	if (clientId.isEmpty()) { return false; }
	storeValue(key, clientId);
	
	return true;
}


string findAlbum(string name) {
	// Send query for albums (/playlists)
	// TODO: HTML encode the query string (spaces, etc.).
	//string query = baseUrl + "/playlists?client_id=" + clientId + "&q=" + name;
	string query = baseUrl + "/search/albums?q=" 
							+ name + "&client_id=" + clientId 
							+ "&limit=20&app_locale=en";
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error. Query: " + query;
	}
	
	// Parse results.
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	JSONValue collection = root.get("collection");
	
	string output = "";
	for (int i = 0; i < collection.get_size(); ++i) {
		JSONValue jv = collection[i];
		
		// Get the ID, artist and album name from the Object.
		JSONValue idVal = jv.get("id");
		string id = formatUInt(idVal.getUInt());
		string title = jv.get("title").getString();
		JSONValue user = jv.get("user");
		string user_id = formatUInt(user.get("id").getUInt());
		string username = user.get("username").getString();
		
		output += id + "\t" + title + "\t" + user_id + "\t" + username + "\n";
	}
	
	return output;
}


string findTrack(string name) {
	// Send query for tracks (/tracks)
	// TODO: HTML encode the query string (spaces, etc.).
	//string query = baseUrl + "/tracks?client_id=" + clientId + "&q=" + name;
	string query = baseUrl + "/search/tracks?q=" 
							+ name + "&client_id=" + clientId 
							+ "&limit=20&app_locale=en";
	string response;
	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error. Query: " + query;
	}
	
	// Parse results, get the JSON root.
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	JSONValue collection = root.get("collection");
	
	string output = "";
	for (int i = 0; i < collection.get_size(); ++i) {
		JSONValue jv = root[i];
		
		// Get the ID, artist and album name from the Object.
		JSONValue idVal = jv.get("id");
		string id = formatUInt(idVal.getUInt());
		string title = jv.get("title").getString();
		JSONValue user = jv.get("user");
		string user_id = formatUInt(user.get("id").getUInt());
		string username = user.get("username").getString();
		
		output += id + "\t" + title + "\t" + user_id + "\t" + username + "\n";
	}
	
	
	// Parse results, return them.
	return output;
}


string findArtist(string name) {
	// Send query for artists (/users).
	// TODO: HTML encode the query string (spaces, etc.).
	//string query = baseUrl + "/users?client_id=" + clientId + "&q=" + name;
	string query = baseUrl + "/search/people?q=" 
							+ name + "&client_id=" + clientId 
							+ "&limit=20&app_locale=en";
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error. Query: " + query;
	}
	
	// Parse results, get the JSON root.
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	JSONValue collection = root.get("collection");
	
	string output = "";
	for (int i = 0; i < collection.get_size(); ++i) {
		JSONValue user = root[i];
		
		// Get the ID, artist name from the Object.
		string user_id = formatUInt(user.get("id").getUInt());
		string username = user.get("username").getString();
		
		output += user_id + "\t" + username + "\n";
	}
	
	// Parse results, return them.
	return output;
}


bool playAlbum(int id) {
	// Obtain the data for the album, then play each individual track.
	//string query = baseUrl + "/playlists/" + id + "?client_id=" + clientId;
	string query = baseUrl + "/playlists/" + id + "&client_id=" + clientId + "&app_locale=en";
	string response;
	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return false; //"HTTP error.";
	}
	
	// Parse results, get the JSON root.
	JSONFile json;
	if (!json.fromString(response)) {
		return false; //"Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	
	// TODO: Parse the list of track IDs, send them to the queue.
	JSONValue tracks = root.get("tracks");
	for (int i = 0; i < tracks.get_size(); ++i) {
		JSONValue track = tracks[i];
		int track_id = track.get("id").getUInt();
		
		// Add track ID to queue.
		if (!playTrack(track_id)) {
			return false;
		}
	}
	
	return true;
}


bool playTrack(int id) {
	// Use the provided ID to retrieve the track, then stream it.
	//string url = "https://api.soundcloud.com/tracks/547239669/stream?client_id=" + clientId;
	//string url = "https://api.soundcloud.com/tracks/" + id + "/stream?client_id=" + clientId;
	string query = baseUrl + "/tracks/" + id + "&client_id=" + clientId;
	string response;
	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return false; //"HTTP error.";
	}
	
	// Parse results, get the JSON root.
	JSONFile json;
	if (!json.fromString(response)) {
		return false; //"Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	JSONValue transcodings = root.get("media").get("transcodings");
	string url = transcodings[1].get("url").getString();
	
	return streamTrack(url);
}


string command_processor(string input) {
	// We support a number of commands:
	// . help
	// . find album <search string>
	// . find track <search string>
	// . find artist <search string>
	// . play album <album ID>
	// . play track <track ID>
	if (input == "help") {
		return "Commands:\nhelp\nfind (album|track|artist) <query>\nplay (album|track) <query>";
	}
	
	// FIXME: handle spaces in search strings.
	array<string> bits = input.split(" ");
	int len = bits.length();
	if (len < 3) {
		// Cannot be a valid command. Return error.
		return "Invalid command.";
	}
	
	// Ensure the client ID is up to date.
	if (!updateClientId()) {
		return "Updating client ID failed.";
	}
	
	if (bits[0] == "find") {
		string name = bits[2];
		if (len > 3) {
			for (int i = 3; i < len; i++) { 
				name += bits[i];
				if ((i + 2) < len) { name += "%20"; }
			}
		}
		
		if (bits[1] == "album") {
			return findAlbum(name);
		}
		else if (bits[1] == "track") {
			return findTrack(name);
		}
		else if (bits[1] == "artist") {
			return findArtist(name);
		}
		else {
			// Error.
			return "Invalid command.";
		}
	}
	else if (bits[0] == "play") {
		if (bits[1] == "album") {
			playAlbum(parseInt(bits[2]));
			
			return "Playing album...";
		}
		else if (bits[1] == "track") {
			if (playTrack(parseInt(bits[2]))) {
				return "Streaming failed.";
			}
			
			return "Streaming track...";
		}
		else {
			// Error.
			return "Invalid command.";
		}
	}
	
	return "Invalid command.";
}
