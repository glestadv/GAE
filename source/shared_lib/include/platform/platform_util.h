// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2005 Matthias Braun <matze@braunis.de>
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <sstream>
#include <errno.h>

#ifdef USE_SDL
#	include <glob.h>
	// this is actually posix stuff, not sdl, but close enough for now...
#	include <signal.h>
#elif defined(WIN32) || defined(WIN64)
#	include <io.h>
#endif

#include "types.h"
#include "patterns.h"

using std::string;
using std::vector;
using std::exception;
using std::runtime_error;

using Shared::Platform::int64;

namespace Shared { namespace Platform {

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

//LONG WINAPI UnhandledExceptionFilter2(struct _EXCEPTION_POINTERS *ExceptionInfo);

class PlatformExceptionHandler : Uncopyable {
private:
	static PlatformExceptionHandler *singleton;

	bool installed;
#ifdef USE_SDL
	struct sigaction old_sigill;
	struct sigaction old_sigsegv;
	struct sigaction old_sigabrt;
	struct sigaction old_sigfpe;
	struct sigaction old_sigbus;
#elif defined(WIN32) || defined(WIN64)
	LPTOP_LEVEL_EXCEPTION_FILTER old_filter;
#endif

public:
	PlatformExceptionHandler();
	virtual ~PlatformExceptionHandler();

	void install();
	void uninstall();
	virtual __cold void log(const char *description, void *address, const char **backtrace, size_t count, const exception *e) = 0;
	virtual __cold void notifyUser(bool pretty) = 0;

private:
#ifdef USE_SDL
	static void handler(int signo, siginfo_t *info, void *context);
#elif defined(WIN32) || defined(WIN64)
	static string codeToStr(DWORD code);
	static LONG WINAPI handler(LPEXCEPTION_POINTERS pointers);
#endif
};

// =====================================================
// class DirectoryListing
// =====================================================

class DirectoryListing {
private:
	string path;
	bool exists;
#ifdef USE_SDL
	int i;
	glob_t globbuf;
#elif defined(WIN32) || defined(WIN64)
	int count;
	intptr_t handle;
	struct __finddata64_t fi;
#endif

public:
	DirectoryListing(const string &path);
	~DirectoryListing();

	const char *getNext();
	bool isExists() const	{return exists;}
};
/*
typedef struct _DirIterator {
	struct __finddata64_t fi;
	intptr_t handle;
} DirIterator;

inline char *initDirIterator(const string &path, DirIterator &di) {
	if((di.handle = _findfirst64(path.c_str(), &di.fi)) == -1) {
		if(errno == ENOENT) {
			return NULL;
		}
		throw runtime_error("Error searching for files '" + path + "': " + strerror(errno));
	}
	return di.fi.name;
}

inline char *getNextFile(DirIterator &di) {
	assert(di.handle != -1);
	return !_findnext64(di.handle, &di.fi) ? di.fi.name : NULL;
}

inline void freeDirIterator(DirIterator &di) {
	assert(di.handle != -1);
	_findclose(di.handle);
}*/

// =====================================================
//	Misc
// =====================================================

void mkdir(const string &path, bool ignoreDirExists = false);
size_t getFileSize(const string &path);

bool changeVideoMode(int resH, int resW, int colorBits, int refreshFrequency);
void restoreVideoMode();

void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);

#ifdef USE_SDL
	inline int getScreenW()			{return SDL_GetVideoSurface()->w;}
	inline int getScreenH()			{return SDL_GetVideoSurface()->h;}
	inline void sleep(int millis)	{SDL_Delay(millis);}
	inline void showCursor(bool b)	{SDL_ShowCursor(b ? SDL_ENABLE : SDL_DISABLE);}
#elif defined(WIN32) || defined(WIN64)
	inline int getScreenW()			{return GetSystemMetrics(SM_CXSCREEN);}
	inline int getScreenH()			{return GetSystemMetrics(SM_CYSCREEN);}
	inline void sleep(int millis)	{Sleep(millis);}
	inline void showCursor(bool b)	{ShowCursor(b);}
#endif

string getCommandLine();

}}//end namespace

#endif
