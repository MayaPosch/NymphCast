

string clientId;
//const string baseUrl = "https://api.soundcloud.com";
const string baseUrl = "https://api-v2.soundcloud.com";
string template_header;
string template_footer;


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
	array<string> matches(0);
	RegExp re;
	re.createRegExp("=\"(https://a-v2\\.sndcdn\\.com/assets/.*.js)\"");
	int n = re.findall(response, matches);
	if (n == 0) { return id; }
	
	re.createRegExp("exports={\"api-v2\".*client_id:\"(\\w*)\"");
	for (int i = 0; i < matches.length(); ++i) {
		string js;
		if (!performHttpsQuery(matches[i], js)) {
			return id;
		}
		
		// Extract the ID.
		if (re.findfirst(js, id) == 1) {
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


string findAlbum(string name, int type) {
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
	if (type == 1) {
		// Add HTML header for albums.
		output += template_header;
		output += "Albums:<br>";
		output += "<table>";
	}
	
	for (int i = 0; i < collection.get_size(); ++i) {
		JSONValue jv = collection[i];
		
		// Get the ID, artist and album name from the Object.
		JSONValue idVal = jv.get("id");
		string id = formatUInt(idVal.getUInt());
		string title = jv.get("title").getString();
		JSONValue user = jv.get("user");
		string user_id = formatUInt(user.get("id").getUInt());
		string username = user.get("username").getString();
		
		if (type == 1) {
			// HTML encode.
			output += "<tr><td><a href=\"SoundCloud/play/album/" + id + "\">" + username + " - " + 
						title + "</a></td></tr>";	
		}
		else {
			output += id + "\t" + title + "\t" + user_id + "\t" + username + "\n";
		}
	}
	
	if (type == 1) {
		// Add HTML footer for albums.
		output += "</table>";
		output += template_footer;
	}
	
	return output;
}


string findTrack(string name, int type) {
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
	if (type == 1) {
		// Add HTML header for tracks.
		output += template_header;
		output += "Tracks:<br>";
		output += "<table>";
	}
	
	for (int i = 0; i < collection.get_size(); ++i) {
		JSONValue jv = collection[i];
		
		// Get the ID, artist and album name from the Object.
		JSONValue idVal = jv.get("id");
		string id = formatUInt(idVal.getUInt());
		string title = jv.get("title").getString();
		JSONValue user = jv.get("user");
		string user_id = formatUInt(user.get("id").getUInt());
		string username = user.get("username").getString();
		
		if (type == 1) {
			// HTML encode.
			output += "<tr><td><a href=\"SoundCloud/play/track/" + id + "\">" + username + " - " + 
						title + "</a></td></tr>";	
		}
		else {
			output += id + "\t" + title + "\t" + user_id + "\t" + username + "\n";
		}
	}
	
	if (type == 1) {
		// Add HTML footer for tracks.
		output += "</table>";
		output += template_footer;
	}
	
	return output;
}


string findArtist(string name, int type) {
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
	if (type == 1) {
		// Add HTML header for artists.
		output += template_header;
		output += "Artists:<br>";
		output += "<table>";
	}
	
	for (int i = 0; i < collection.get_size(); ++i) {
		JSONValue user = collection[i];
		
		// Get the ID, artist name from the Object.
		string user_id = formatUInt(user.get("id").getUInt());
		string username = user.get("username").getString();
		
		if (type == 1) {
			// HTML encode.
			output += "<tr><td>" + username + " - " + 
						user_id + "</td></tr>";	
		}
		else {
			output += user_id + "\t" + username + "\n";
		}
	}
	
	if (type == 1) {
		// Add HTML footer for artists.
		output += "</table>";
		output += template_footer;
	}
	
	return output;
}


bool playAlbum(int id) {
	// Obtain the data for the album, then play each individual track.
	//string query = baseUrl + "/playlists/" + id + "?client_id=" + clientId;
	string query = baseUrl + "/playlists/" + id + "?client_id=" + clientId;
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
	string query = baseUrl + "/tracks/" + id + "?client_id=" + clientId;
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
	
	query = url + "?client_id=" + clientId;
	if (!performHttpsQuery(query, response)) {
		return false;
	}
	
	// Parse results for the media URL.
	JSONFile mediajson;
	if (!mediajson.fromString(response)) {
		return false;
	}
	
	JSONValue mediaroot = mediajson.getRoot();
	string media_url = mediaroot.get("url").getString();	
	
	return streamTrack(media_url);
}


// Arguments:
// - input 	=> the command(s) as a single string.
// - type 	=> the desired output format: 0 (tabs), 1 (HTML)
string command_processor(string input, int type) {
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
		return "Invalid command: " + input;
	}
	
	// Ensure the client ID is up to date.
	if (!updateClientId()) {
		return "Updating client ID failed.";
	}
	
	// Fetch templates if we're generating HTML.
	if (type == 1) {
		if (!readTemplate("header.html", template_header)) {
			return "Failed to read header template.";
		}
		
		if (!readTemplate("footer.html", template_footer)) {
			return "Failed to read footer template";
		}
	}
	
	if (bits[0] == "find") {
		string name = bits[2];
		if (len > 3) {
			for (int i = 3; i < len; i++) {
				if ((i + 2) < len) { name += "%20"; }
				name += bits[i];
			}
		}
		
		if (bits[1] == "album") {
			return findAlbum(name, type);
		}
		else if (bits[1] == "track") {
			return findTrack(name, type);
		}
		else if (bits[1] == "artist") {
			return findArtist(name, type);
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


//
string html_processor(string input) {
	return command_processor(input, 1);
}
