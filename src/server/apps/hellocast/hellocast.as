// hellocast.as - 'Hello World' level demonstration app for NymphCast Apps.
//
// 2020/02/13, Maya Posch


string command_processor(string input, int type) {
	// Available commands:
	// - help
	// - play
	if (input == "help") {
		return "Commands:\n- help\n- play";
	}
	else if (input == "play") {
		// Play the standard track.
		string url = "https://github.com/MayaPosch/NymphCast/releases/download/v0.1-alpha/How_it_Began.mp3";
		if (streamTrack(url)) {
			return "Playing back track...";
		}
		else {
			return "Failed to play back track.";
		}
	}
	
	return "Invalid command.";
}


//
string html_processor(string input) {
	return command_processor(input, 1);
}
