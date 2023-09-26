/*
   AngelCode Scripting Library
   Copyright (c) 2003-2015 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_configgroup.h
//
// This class holds configuration groups for the engine
//



#ifndef AS_CONFIGGROUP_H
#define AS_CONFIGGROUP_H

#include "as_config.h"
#include "as_string.h"
#include "as_array.h"
#include "as_objecttype.h"

BEGIN_AS_NAMESPACE

class asCConfigGroup
{
public:
	asCConfigGroup();
	~asCConfigGroup();

	// Memory management
	int AddRef();
	int Release();

	asCTypeInfo *FindType(const char *name);
	void RefConfigGroup(asCConfigGroup *group);

	bool HasLiveObjects();
	void RemoveConfiguration(asCScriptEngine *engine, bool notUsed = false);

	void AddReferencesForFunc(asCScriptEngine *engine, asCScriptFunction *func);
	void AddReferencesForType(asCScriptEngine *engine, asCTypeInfo *type);

	asCString groupName;
	int refCount;

	asCArray<asCTypeInfo*>       types;
	asCArray<asCScriptFunction*> scriptFunctions;
	asCArray<asCGlobalProperty*> globalProps;
	asCArray<asCConfigGroup*>    referencedConfigGroups;

	// This array holds the generated template instances that are used 
	// by the config group as part of function signature or property
	asCArray<asCObjectType*>     generatedTemplateInstances;
};

END_AS_NAMESPACE

#endif
