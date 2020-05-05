/*
	regexp.cpp - Implementation of the NymphCast regular expression API for AngelScript.
	
	2020/04/29, Maya Posch
*/


#include "regexp.h"


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
								"int extract(string &in, string &out, int)", 
								asMETHODPR(RegExp, extract, 
								(const std::string &, std::string &, int), int), 
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int extract(string &in, int offset, string &out, int)", 
								asMETHODPR(RegExp, extract, 
								(const std::string &, int, std::string &, int), int),
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int findall(string &in, array<string> &out)", 
								asMETHODPR(RegExp, findall, 
								(const std::string &, CScriptArray* &), int),
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
	if (regexp) {
		delete regexp;
	}
	
	regexp = new Poco::RegularExpression(re);
}


// --- EXTRACT ---
int RegExp::extract(const std::string &subject, std::string &str, int options) {
	if (!regexp) { return 0; }
	return regexp->extract(subject, str, options);
}


// --- EXTRACT ---
int RegExp::extract(const std::string &subject, int offset, std::string &str, int options) {
	if (!regexp) { return 0; }
	return regexp->extract(subject, offset, str, options);
}


// --- FIND ALL ---
int RegExp::findall(const std::string &subject, CScriptArray* &matches) {
	if (!regexp) { return 0; }
	
	Poco::RegularExpression::MatchVec matchesVector;
	int n = regexp->match(subject, 0, matchesVector);
	if (n == 0) { return 0; }
	
	// MatchVec is an std::vector of Match structs. The latter contains the length and offset of
	// the match in the original string.
	// We want to extract these substrings and return them.
	matches->Resize(matchesVector.size());
    for (int i = 0; i < matchesVector.size(); i++) {
      // Set the value of each element
      std::string substr = subject.substr(matchesVector[i].offset, matchesVector[i].length);
      matches->InsertAt(i, &substr);
    }
	
	return n;
}
