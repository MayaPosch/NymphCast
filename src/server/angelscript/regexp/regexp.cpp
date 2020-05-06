/*
	regexp.cpp - Implementation of the NymphCast regular expression API for AngelScript.
	
	2020/04/29, Maya Posch
*/


#include "regexp.h"

#include <iostream>

#include <Poco/Exception.h>


static void ConstructRegExp(RegExp* ptr) {
    new(ptr) RegExp();
}

static void DestructRegExp(RegExp* ptr) {
    ptr->~RegExp();
}


void initRegExp(asIScriptEngine* engine) {
	engine->RegisterObjectType("RegExp", sizeof(RegExp), asOBJ_VALUE);
	
	engine->RegisterObjectBehaviour("RegExp", asBEHAVE_CONSTRUCT, "void f()", 	
									asFUNCTION(ConstructRegExp), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("RegExp", asBEHAVE_DESTRUCT, "void f()", 
									asFUNCTION(DestructRegExp), asCALL_CDECL_OBJLAST);
	
    engine->RegisterObjectMethod("RegExp", 
								"void createRegExp(string &in)", 
								asMETHOD(RegExp, createRegExp), 
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int extract(string &in, string &out, int = 0)", 
								asMETHODPR(RegExp, extract, 
								(const std::string &, std::string &, int), int), 
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int extract(string &in, int offset, string &out, int = 0)",
								asMETHODPR(RegExp, extract, 
								(const std::string &, int, std::string &, int), int),
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int findall(const string &in, array<string> @+ = null)", 
								asMETHODPR(RegExp, findall, 
								(const std::string &, CScriptArray*), int),
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int findfirst(const string &in, string &out)", 
								asMETHODPR(RegExp, findfirst, 
								(const std::string &, std::string &), int),
								asCALL_THISCALL);
}


// --- CONSTRUCTOR ---
RegExp::RegExp() {
	regexp = 0;
}


// --- DESTRUCTOR ---
RegExp::~RegExp() {
	if (regexp) {
		delete regexp;
	}
}


// --- SET REG EXP ---
void RegExp::createRegExp(std::string re) {
	if (regexp) { delete regexp; regexp = 0; }
	
	try {
		regexp = new Poco::RegularExpression(re);
	}
	catch (Poco::RegularExpressionException &exc) {
		std::cerr << "Couldn't parse regular expression: " << re << std::endl;
	}
}


// --- EXTRACT ---
int RegExp::extract(const std::string &subject, std::string &str, int options) {
	if (!regexp) { return 0; }
	int n = regexp->extract(subject, str, options);
	std::cout << "Found " << n << " matches." << std::endl;
	return n;
}


// --- EXTRACT ---
int RegExp::extract(const std::string &subject, int offset, std::string &str, int options) {
	if (!regexp) { return 0; }
	int n = regexp->extract(subject, offset, str, options);
	std::cout << "Found " << n << " matches." << std::endl;
	return n;
}


// --- FIND ALL ---
int RegExp::findall(const std::string &subject, CScriptArray* matches) {
	if (!regexp) { return 0; }
	
	Poco::RegularExpression::MatchVec matchesVector;
	std::string::size_type offset = 0;
	int n = 0;
	while (regexp->match(subject, offset, matchesVector)) {
		std::string substr = subject.substr(matchesVector[1].offset, matchesVector[1].length);
		if (matches) { matches->InsertLast(&substr); }
		n++;
		offset = matchesVector[0].offset + matchesVector[0].length;
	}
	
	std::cout << "RegExp::findall(): Found " << n << " matches." << std::endl;
	return n;
}


// --- FIND FIRST ---
int RegExp::findfirst(const std::string &subject, std::string &str) {
	if (!regexp) { return 0; }
	
	Poco::RegularExpression::MatchVec matchesVector;
	std::string::size_type offset = 0;
	int n = regexp->match(subject, offset, matchesVector);
	if (n == 0) { return 0; }
	
	str = subject.substr(matchesVector[1].offset, matchesVector[1].length);
	std::cout << "RegExp::findfirst(): Found " << n << " match." << std::endl;
	return n;
}
