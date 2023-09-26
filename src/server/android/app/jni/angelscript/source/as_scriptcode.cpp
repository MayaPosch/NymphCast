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
// as_scriptcode.cpp
//
// A container class for the script code to be compiled
//



#include "as_config.h"
#include "as_scriptcode.h"

BEGIN_AS_NAMESPACE

asCScriptCode::asCScriptCode()
{
	lineOffset = 0;
	code = 0;
	codeLength = 0;
	sharedCode = false;
}

asCScriptCode::~asCScriptCode()
{
	if( !sharedCode && code ) 
	{
		asDELETEARRAY(code);
	}
}

int asCScriptCode::SetCode(const char *in_name, const char *in_code, bool in_makeCopy)
{
	return SetCode(in_name, in_code, 0, in_makeCopy);
}

int asCScriptCode::SetCode(const char *in_name, const char *in_code, size_t in_length, bool in_makeCopy)
{
	if( !in_code) return asINVALID_ARG;
	this->name = in_name ? in_name : "";
	if( !sharedCode && code ) 
		asDELETEARRAY(code);

	if( in_length == 0 )
		in_length = strlen(in_code);
	if( in_makeCopy )
	{
		codeLength = in_length;
		sharedCode = false;
		code = asNEWARRAY(char, in_length);
		if( code == 0 )
			return asOUT_OF_MEMORY;
		memcpy(code, in_code, in_length);
	}
	else
	{
		codeLength = in_length;
		code = const_cast<char*>(in_code);
		sharedCode = true;
	}

	// Find the positions of each line
	linePositions.PushLast(0);
	for( size_t n = 0; n < in_length; n++ )
		if( in_code[n] == '\n' ) linePositions.PushLast(n+1);
	linePositions.PushLast(in_length);

	return asSUCCESS;
}

void asCScriptCode::ConvertPosToRowCol(size_t pos, int *row, int *col)
{
	if( linePositions.GetLength() == 0 ) 
	{
		if( row ) *row = lineOffset;
		if( col ) *col = 1;
		return;
	}

	// Do a binary search in the buffer
	int max = (int)linePositions.GetLength() - 1;
	int min = 0;
	int i = max/2;

	for(;;)
	{
		if( linePositions[i] < pos )
		{
			// Have we found the largest number < programPosition?
			if( min == i ) break;

			min = i;
			i = (max + min)/2;
		}
		else if( linePositions[i] > pos )
		{
			// Have we found the smallest number > programPoisition?
			if( max == i ) break;

			max = i;
			i = (max + min)/2;
		}
		else
		{
			// We found the exact position
			break;
		}
	}

	if( row ) *row = i + 1 + lineOffset;
	if( col ) *col = (int)(pos - linePositions[i]) + 1;
}

bool asCScriptCode::TokenEquals(size_t pos, size_t len, const char *str)
{
	if( pos + len > codeLength ) return false;
	if( strncmp(code + pos, str, len) == 0 && strlen(str) == len )
		return true;
	return false;
}

END_AS_NAMESPACE
