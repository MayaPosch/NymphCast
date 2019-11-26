

const string clientId = "iHwMAYPsv39tFIqaL5hR3Un41wI7JhXO";
const string baseUrl = "https://api.soundcloud.com";


string start() {
	// Return the available features.
	
	return "";
}


string findAlbum(string name) {
	// Send query for albums (/playlists)
	string query = baseUrl + "/playlists?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, get the 
	JSONFile json; // = JSONFile();
	if (!json.fromString(response)) {
		return "Failed to parse JSON response.";
	}
	
	JSONValue root = json.getRoot();
	
	string output = "";
	for (int i = 0; i < root.get_size(); ++i) {
		JSONValue jv = root[i];
		
		// Get the ID, artist and album name from the Object.
		string id = formatUInt(jv.get("id").getUInt());
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
	string query = baseUrl + "/tracks?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, return them.
	return response;
}


string findArtist(string name) {
	string query = baseUrl + "/tracks?client_id=" + clientId + "&q=" + name;
	string response;

	if (!performHttpsQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, return them.
	return response;
}


bool playAlbum(int id) {
	// Obtain the data for the album, then play each individual track.
	
	
	// Use the provided ID to start streaming the track.
	string url = "";
	return streamTrack(url);
}


bool playTrack(int id) {
	// Use the provided ID to retrieve the album, then stream each individual track on it.
	string url = "https://api.soundcloud.com/tracks/547239669/stream?client_id=" + clientId;
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
