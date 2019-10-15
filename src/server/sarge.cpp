/*
	sarge.cpp - Implementation file for the Sarge command line argument parser project.
	
	Revision 0
	
	Features:
			- 
	
	Notes:
			-
			 
	2019/03/16, Maya Posch
	
*/


#include "sarge.h"

#include <iostream>


// --- SET ARGUMENT ---
void Sarge::setArgument(std::string arg_short, std::string arg_long, std::string desc, bool hasVal) {
	std::unique_ptr<Argument> arg(new Argument);
	arg->arg_short = arg_short;
	arg->arg_long = arg_long;
	arg->description = desc;
	arg->hasValue = hasVal;
	args.push_back(std::move(arg));
	
	// Set up links.
	if (!arg_short.empty()) {
		argNames.insert(std::pair<std::string, Argument*>(arg_short, args.back().get()));
	}
	
	if (!arg_long.empty()) {
		argNames.insert(std::pair<std::string, Argument*>(arg_long, args.back().get()));
	}
}
	

// --- SET ARGUMENTS ---
void Sarge::setArguments(std::vector<Argument> args) {
	for (Argument a : args) {
		setArgument(a.arg_short, a.arg_long, a.description, a.hasValue);
	}
}


// --- PARSE ARGUMENTS ---
bool Sarge::parseArguments(int argc, char** argv) {
	// The first argument is the name of the executable. After it we loop through the remaining
	// arguments, linking flags and values.
	execName = std::string(argv[0]);
	bool expectValue = false;
	std::map<std::string, Argument*>::const_iterator flag_it;
	for (int i = 1; i < argc; ++i) {
		// Each flag will start with a '-' character. Multiple flags can be joined together in the
		// same string if they're the short form flag type (one character per flag).
		std::string entry(argv[i]);
		
		if (expectValue) {
			// Copy value.
			flag_it->second->value = entry;			
			expectValue = false;
		}
		else if (entry.compare(0, 1, "-") == 0) {
			if (textArguments.size() > 0) { 
				std::cerr << "Flags not allowed after text arguments." << std::endl; 
			}
			
			// Parse flag.
			// First check for the long form.
			if (entry.compare(0, 2, "--") == 0) {
				// Long form of flag.
				entry.erase(0, 2); // Erase the double dash since we no longer need it.
			
				flag_it = argNames.find(entry);
				if (flag_it == argNames.end()) {
					// Flag wasn't found. Abort.
					std::cerr << "Long flag " << entry << " wasn't found." << std::endl;
					return false;
				}
				
				// Mark as found.
				flag_it->second->parsed = true;
				++flagCounter;
				
				if (flag_it->second->hasValue) {
					expectValue = true; // Next argument has to be a value string.
				}
			}
			else {
				// Parse short form flag. Parse all of them sequentially. Only the last one
				// is allowed to have an additional value following it.
				entry.erase(0, 1); // Erase the dash.				
				for (int i = 0; i < entry.length(); ++i) {
					std::string k(&(entry[i]), 1);
					flag_it = argNames.find(k);
					if (flag_it == argNames.end()) {
						// Flag wasn't found. Abort.
						std::cerr << "Short flag " << k << " wasn't found." << std::endl;
						return false;
					}
					
					// Mark as found.
					flag_it->second->parsed = true;
					++flagCounter;
					
					if (flag_it->second->hasValue) {
						if (i != (entry.length() - 1)) {
							// Flag isn't at end, thus cannot have value.
							std::cerr << "Flag " << k << " needs to be followed by a value string."
								<< std::endl;
							return false;
						} else {
							expectValue = true; // Next argument has to be a value string.
						}
					}
				}
			}
		}
		else {
			// Add to text argument vector.
			textArguments.push_back(entry);
		}
	}
	
	parsed = true;
	
	return true;
}


// --- GET FLAG ---
// Returns whether the flag was found, along with the value if relevant.
bool Sarge::getFlag(std::string arg_flag, std::string &arg_value) {
	if (!parsed) { return false; }
	
	std::map<std::string, Argument*>::const_iterator it = argNames.find(arg_flag);
	if (it == argNames.end()) { return false; }
	if (!it->second->parsed) { return false; }
	
	if (it->second->hasValue) { arg_value = it->second->value; }
	
	return true;
}


// --- EXISTS ---
// Returns whether the flag was found.
bool Sarge::exists(std::string arg_flag) {
	if (!parsed) { return false; }
	
	std::map<std::string, Argument*>::const_iterator it = argNames.find(arg_flag);
	if (it == argNames.end()) { return false; }	
	if (!it->second->parsed) { return false; }
	
	return true;
}


// --- GET TEXT ARGUMENT ---
// Updates the value parameter with the text argument (unbound value) if found.
// Index starts at 0.
// Returns true if found, else false.
bool Sarge::getTextArgument(uint32_t index, std::string &value) {
	if (index < textArguments.size()) { value = textArguments.at(index); return true; }
	
	return false;
}


// --- PRINT HELP ---
// Prints out the application description, usage and available options.
void Sarge::printHelp() {
	std::cout << std::endl << description << std::endl;
	std::cout << std::endl << "Usage:" << std::endl;
	std::cout << "\t" << usage << std::endl;
	std::cout << std::endl;
	std::cout << "Options: " << std::endl;
	
	// Print out the options.
	std::vector<std::unique_ptr<Argument> >::const_iterator it;
	for (it = args.cbegin(); it != args.cend(); ++it) {
		std::cout << "-" << (*it)->arg_short << "\t--" << (*it)->arg_long << "\t\t" << (*it)->description << std::endl;
	}
}
