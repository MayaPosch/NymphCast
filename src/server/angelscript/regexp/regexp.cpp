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
								"void createRegExp(string &re)", 
								asMETHOD(RegExp, createRegExp), 
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int extract(string &subject, string &str, int options = 0))", 
								asMETHOD(RegExp, extract), 
								asCALL_THISCALL);
    engine->RegisterObjectMethod("RegExp", 
								"int extract(string &subject, int offset, string &str, int options = 0))", 
								asMETHOD(RegExp, extract), 
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
int RegExp::extract(const std::string &subject, std::string &str, int options = 0) {
	if (!regexp) { return 0; }
	return regexp.extract(subject, str, options);
}


// --- EXTRACT ---
int RegExp::extract(const std::string &subject, int offset, std::string &str, int options = 0) {
	if (!regexp) { return 0; }
	return regexp.extract(subject, offset, str, options);
}
