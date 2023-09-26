/*
   AngelCode Scripting Library
   Copyright (c) 2003-2013 Andreas Jonsson

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
// as_tokenizer.cpp
//
// This class identifies tokens from the script code
//



#ifndef AS_TOKENIZER_H
#define AS_TOKENIZER_H

#include "as_config.h"
#include "as_tokendef.h"
#include "as_map.h"
#include "as_string.h"

BEGIN_AS_NAMESPACE

class asCTokenizer
{
public:
	eTokenType GetToken(const char *source, size_t sourceLength, size_t *tokenLength, asETokenClass *tc = 0) const;
	
	static const char *GetDefinition(int tokenType);

protected:
	friend class asCScriptEngine;

	asCTokenizer();
	~asCTokenizer();

	asETokenClass ParseToken(const char *source, size_t sourceLength, size_t &tokenLength, eTokenType &tokenType) const;
	bool IsWhiteSpace(const char *source, size_t sourceLength, size_t &tokenLength, eTokenType &tokenType) const;
	bool IsComment(const char *source, size_t sourceLength, size_t &tokenLength, eTokenType &tokenType) const;
	bool IsConstant(const char *source, size_t sourceLength, size_t &tokenLength, eTokenType &tokenType) const;
	bool IsKeyWord(const char *source, size_t sourceLength, size_t &tokenLength, eTokenType &tokenType) const;
	bool IsIdentifier(const char *source, size_t sourceLength, size_t &tokenLength, eTokenType &tokenType) const;
	bool IsDigitInRadix(char ch, int radix) const;

	const asCScriptEngine *engine;

	const sTokenWord **keywordTable[256];
};

END_AS_NAMESPACE

#endif

