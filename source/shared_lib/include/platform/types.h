// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2005 Matthias Braun <matze@braunis.de>
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_TYPES_H_
#define _SHARED_PLATFORM_TYPES_H_

#if HAVE_PTHREAD && USE_PTHREAD
#	include <pthread.h>
#endif

#if USE_SDL
#	include <SDL.h>
#	if !USE_PTHREAD
#		include <SDL_thread.h>
#	endif
#	include <glob.h>
#elif defined(WIN32) || defined(WIN64)
#	define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#	include <windows.h>
#endif

#include <boost/version.hpp>
#if BOOST_VERSION < 103600
#	error This version of boost is screwed up, please use 1.39 or later.
#elif BOOST_VERSION < 103900 // The latest version that I know to work with -fno-rtti
#	error This version of boost may be screwed up.  You should make sure you are compiling with \
	RTTI disabled (on gcc, use the -fno-rtti flag).  If you have done so and are not getting \
	errors due to use of RTTI features in boost header files, please edit this code and \
	appropriately adjust the boost version check.
#endif

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

namespace Shared { namespace Platform {

// threads & synchronization
#if defined(USE_PTHREAD)
	typedef pthread_t * ThreadType;
	typedef void *ThreadFuncReturnType;
	typedef int ThreadId;
	typedef pthread_mutex_t* MutexType;
	typedef pthread_cond_t* ConditionType;
#elif defined(USE_SDL)
	typedef SDL_Thread* ThreadType;
	typedef int ThreadFuncReturnType;
	typedef int ThreadId;
	typedef SDL_mutex* MutexType;
	typedef SDL_cond* ConditionType;
#elif defined(WIN32) || defined(WIN64)
	typedef HANDLE ThreadType;
#	define ThreadFuncReturnType DWORD WINAPI
	typedef DWORD ThreadId;
	typedef CRITICAL_SECTION MutexType;
#	ifdef WIN64
		typedef CONDITION_VARIABLE ConditionType;
#	else
		typedef HANDLE ConditionType;
#	endif
#endif

#ifdef USE_SDL
	// windows & graphics
	// These don't have a real meaning in the SDL port
	typedef void* WindowHandle;
	typedef void* DeviceContextHandle;
	typedef void* GlContextHandle;
	typedef void* PROC;

	// input devices
	typedef SDLKey NativeKeyCode;
	typedef unsigned short NativeKeyCodeCompact;

	typedef float float32;
	typedef double float64;
	// don't use Sint8 here because that is defined as signed char
	// and some parts of the code do std::string str = (int8*) var;
	typedef char int8;
	typedef Uint8 uint8;
	typedef Sint16 int16;
	typedef Uint16 uint16;
	typedef Sint32 int32;
	typedef Uint32 uint32;
	typedef Sint64 int64;
	typedef Uint64 uint64;
#elif defined(WIN32)  || defined(WIN64)
	// windows & graphics
	typedef HWND WindowHandle;
	typedef HDC DeviceContextHandle;
	typedef HGLRC GlContextHandle;

	// input devices
	typedef DWORD NativeKeyCode;
	typedef unsigned char NativeKeyCodeCompact;

	typedef float float32;
	typedef double float64;
	typedef char int8;
	typedef unsigned char uint8;
	typedef short int int16;
	typedef unsigned short int uint16;
	typedef int int32;
	typedef unsigned int uint32;
	typedef long long int64;
	typedef unsigned long long uint64;

#	define strcasecmp stricmp
#	define bzero(dest, size) memset(dest, 0, size)
#	define snprintf _snprintf
#endif

#ifdef USE_POSIX_SOCKETS
	typedef int SOCKET;
#endif

}}//end namespace

#endif
