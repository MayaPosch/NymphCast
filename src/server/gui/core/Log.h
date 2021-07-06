#pragma once
#ifndef ES_CORE_LOG_H
#define ES_CORE_LOG_H

#include <sstream>

#define LOG(level) \
if(level > Log::getReportingLevel()) ; \
else Log().get(level)

enum LogLevel { LogError, LogWarning, LogInfo, LogDebug };

class Log {
public:
	//Log();
	~Log();
	std::ostringstream& get(LogLevel level = LogInfo);

	static LogLevel getReportingLevel();
	static void setReportingLevel(LogLevel level);

	static std::string getLogPath();

	static void flush();
	static void init();
	static void open();
	static void close();
protected:
	std::ostringstream os;
	static FILE* file;
private:
	static LogLevel reportingLevel;
	static FILE* getOutput();
	LogLevel messageLevel;
};

#endif // ES_CORE_LOG_H
