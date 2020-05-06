/*
	regexp.h - Header for the NymphCast regular expression AngelScript API.
	
	2020/04/29, Maya Posch
*/


#ifndef REGEXP_H
#define REGEXP_H


#include <angelscript.h>
#include <scriptarray/scriptarray.h>

#include <Poco/RegularExpression.h>

#include <string>


void initRegExp(asIScriptEngine* engine);


class RegExp {
	Poco::RegularExpression* regexp;
	
public:
	explicit RegExp();
	~RegExp();
	
	void createRegExp(std::string re);
	int extract(const std::string &subject, std::string &str, int options = 0);
	int extract(const std::string &subject, int offset, std::string &str, int options = 0);
	int findall(const std::string &subject, CScriptArray* matches);
	int findfirst(const std::string &subject, std::string &str);
};

#endif
