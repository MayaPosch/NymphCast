/*
   AngelCode Scripting Library
   Copyright (c) 2003-2014 Andreas Jonsson

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
// as_atomic.cpp
//
// The implementation of the atomic class for thread safe reference counting
//

#include "as_atomic.h"

BEGIN_AS_NAMESPACE

asCAtomic::asCAtomic()
{
	value = 0;
}

asDWORD asCAtomic::get() const
{
	// A very high ref count is highly unlikely. It most likely a problem with
	// memory that has been overwritten or is being accessed after it was deleted.
	asASSERT(value < 1000000);

	return value;
}

void asCAtomic::set(asDWORD val)
{
	// A very high ref count is highly unlikely. It most likely a problem with
	// memory that has been overwritten or is being accessed after it was deleted.
	asASSERT(value < 1000000);

	value = val;
}

asDWORD asCAtomic::atomicInc()
{
	// A very high ref count is highly unlikely. It most likely a problem with
	// memory that has been overwritten or is being accessed after it was deleted.
	asASSERT(value < 1000000);

	return asAtomicInc((int&)value);
}

asDWORD asCAtomic::atomicDec()
{
	// A very high ref count is highly unlikely. It most likely a problem with
	// memory that has been overwritten or is being accessed after it was deleted.
	asASSERT(value < 1000000);

	return asAtomicDec((int&)value);
}

//
// The following code implements the atomicInc and atomicDec on different platforms
//
#if defined(AS_NO_THREADS) || defined(AS_NO_ATOMIC)

int asAtomicInc(int &value)
{
	return ++value;
}

int asAtomicDec(int &value)
{
	return --value;
}

#elif defined(AS_XENON) /// XBox360

END_AS_NAMESPACE
#include <xtl.h>
BEGIN_AS_NAMESPACE

int asAtomicInc(int &value)
{
	return InterlockedIncrement((LONG*)&value);
}

int asAtomicDec(int &value)
{
	return InterlockedDecrement((LONG*)&value);
}

#elif defined(AS_WIN)

END_AS_NAMESPACE
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
BEGIN_AS_NAMESPACE

int asAtomicInc(int &value)
{
	return InterlockedIncrement((LONG*)&value);
}

int asAtomicDec(int &value)
{
	asASSERT(value > 0);
	return InterlockedDecrement((LONG*)&value);
}

#elif defined(AS_LINUX) || defined(AS_BSD) || defined(AS_ILLUMOS) || defined(AS_ANDROID)

//
// atomic_inc_and_test() and atomic_dec_and_test() from asm/atomic.h is not meant 
// to be used outside the Linux kernel. Instead we should use the GNUC provided 
// __sync_add_and_fetch() and __sync_sub_and_fetch() functions.
//
// Reference: http://golubenco.org/blog/atomic-operations/
//
// These are only available in GCC 4.1 and above, so for older versions we 
// use the critical sections, though it is a lot slower.
// 

int asAtomicInc(int &value)
{
	return __sync_add_and_fetch(&value, 1);
}

int asAtomicDec(int &value)
{
	return __sync_sub_and_fetch(&value, 1);
}

#elif defined(AS_MAC) || defined(AS_IPHONE)

END_AS_NAMESPACE
#include <libkern/OSAtomic.h>
BEGIN_AS_NAMESPACE

int asAtomicInc(int &value)
{
	return OSAtomicIncrement32((int32_t*)&value);
}

int asAtomicDec(int &value)
{
	return OSAtomicDecrement32((int32_t*)&value);
}

#else

// If we get here, then the configuration in as_config.h
//  is wrong for the compiler/platform combination. 
int ERROR_PleaseFixTheConfig[-1];

#endif

END_AS_NAMESPACE

