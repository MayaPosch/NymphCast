
#ifndef TYPES_H
#define TYPES_H


void finishPlayback();


class Logger {
	//
	
public:
	void information(std::string msg, std::string file, std::string &line);
	void error(std::string msg, std::string file, std::string &line);
};


class NymphLogger {
	static Logger log;
	static Poco::Message::Priority priority;
	
public:
	static Logger& logger(std::string &name);
};

#endif