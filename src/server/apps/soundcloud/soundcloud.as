

const string clientId = "iHwMAYPsv39tFIqaL5hR3Un41wI7JhXO";
const string baseUrl = "https://api.soundcloud.com";


string start() {
	// Return the available features.
	
	return "";
}


string findAlbum(string name) {
	// Send query for albums (/playlists)
	// TODO: HTML encode the query string (spaces, etc.).
	string query = baseUrl + "/playlists?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results.
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	
	string output = "";
	for (int i = 0; i < root.get_size(); ++i) {
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
	
	return output;
}


string findTrack(string name) {
	// Send query for tracks (/tracks)
	// TODO: HTML encode the query string (spaces, etc.).
	string query = baseUrl + "/tracks?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, get the 
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	
	string output = "";
	for (int i = 0; i < root.get_size(); ++i) {
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
	string query = baseUrl + "/users?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, get the 
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	
	string output = "";
	for (int i = 0; i < root.get_size(); ++i) {
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
	string query = baseUrl + "/playlists/" + id + "?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, get the 
	JSONFile json;
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
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
	// Use the provided ID to retrieve the album, then stream each individual track on it.
	//string url = "https://api.soundcloud.com/tracks/547239669/stream?client_id=" + clientId;
	string url = "https://api.soundcloud.com/tracks/" + id + "/stream?client_id=" + clientId;
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
		return "Commands:\n. help\n.find (album|track|artist) <query>\n.play (album|track) <query>";
	}
	
	// FIXME: handle spaces in search strings.
	array<string> bits = input.split(" ");
	if (bits.length() < 3) {
		// Cannot be a valid command. Return error.
		return "Invalid command.";
	}
	
	if (bits[0] == "find") {
		if (bits[1] == "album") {
			return findAlbum(bits[2]);
		}
		else if (bits[1] == "track") {
			return findTrack(bits[2]);
		}
		else if (bits[1] == "artist") {
			return findArtist(bits[2]);
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
