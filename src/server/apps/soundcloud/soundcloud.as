

const string clientId = "iHwMAYPsv39tFIqaL5hR3Un41wI7JhXO";


string start() {
	// Return the available features.
	
	return "";
}


string findAlbum(string name) {
	// Send query for albums (/albums)
	string query = "";
	string response;

	if (!performHttpQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, return them.
	return response;
}


string findTrack(string name) {
	// Send query for tracks (/tracks)
	string query = "";
	string response;

	if (!performHttpQuery(query, response)) {
		// Something went wrong.
		return "HTTP error.";
	}
	
	// Parse results, return them.
	return response;
}


string findArtist(string name) {
	string query = "";
	string response;

	if (!performHttpQuery(query, response)) {
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
		}
		else if (bits[1] == "track") {
			playTrack(parseInt(bits[2]));
		}
		else {
			// Error.
			return "Invalid command.";
		}
	}
	
	return "Invalid command.";
}
