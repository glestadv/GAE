// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
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
#include <iostream>
#include <sstream>
#include <SDL.h>
#include <glob.h>

#include "types.h"

using std::string;
using std::vector;
using std::exception;

using Shared::Platform::int64;

namespace Shared{ namespace Platform{

// =====================================================
//	class PerformanceTimer
// =====================================================

class PerformanceTimer{
private:
	Uint32 lastTicks;
	Uint32 updateTicks;

	int times;			// number of consecutive times
	int maxTimes;		// maximum number consecutive times

public:
	PerformanceTimer(float fps, int maxTimes= -1) {
		lastTicks = SDL_GetTicks();
		updateTicks = static_cast<int>(1000.f / fps);
		times = 0;
		this->maxTimes = maxTimes;
	}

	bool isTime() {
		if(!timeToWait() && times < maxTimes) {
			lastTicks += updateTicks;
			++times;
			return true;
		}
		times = 0;
		return false;
	}

	uint32 timeToWait() {
		uint32 elapsed = SDL_GetTicks() - lastTicks;
		return elapsed >= updateTicks ? 0 : updateTicks - elapsed;
	}

	void reset() 			{lastTicks= SDL_GetTicks();}
	void setFps(float fps)	{updateTicks= static_cast<int>(1000./fps);}
};

// =====================================================
//	class Chrono
// =====================================================

class Chrono {
private:
	Uint32 startCount;
	Uint32 accumCount;
	Uint32 freq;
	bool stopped;

public:
	Chrono();
	void start() {
		stopped= false;
		startCount = SDL_GetTicks();
	}

	void stop() {
		Uint32 endCount = SDL_GetTicks();
		accumCount += endCount-startCount;
		stopped= true;
	}

	int64 getMicros() const	{return queryCounter(1000000);}
	int64 getMillis() const {return queryCounter(1000);}
	int64 getSeconds() const {return queryCounter(1);}

private:
	int64 queryCounter(int multiplier) const {
		return multiplier * (accumCount + (stopped ? 0 : SDL_GetTicks() - startCount)) / freq;
	}

};

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
public:
	virtual ~PlatformExceptionHandler() {}
	void install() {}
	virtual void handle(string description, void *address) = 0;
};

// =====================================================
//	Misc
// =====================================================

typedef struct _DirIterator {
	int i;
	glob_t globbuf;
} DirIterator;

char *initDirIterator(const string &path, DirIterator &di);

inline char *getNextFile(DirIterator &di) {
	return ++di.i < di.globbuf.gl_pathc ? di.globbuf.gl_pathv[di.i] : NULL;
}

inline void freeDirIterator(DirIterator &di) {
	globfree(&di.globbuf);
}

void mkdir(const string &path, bool ignoreDirExists = false);
size_t getFileSize(const string &path);

bool changeVideoMode(int resH, int resW, int colorBits, int refreshFrequency);
void restoreVideoMode();

void message(string message);
bool ask(string message);

inline void exceptionMessage(const exception &excp) {
	std::cerr << "Exception: " << excp.what() << std::endl;
}

inline int getScreenW() {
	return SDL_GetVideoSurface()->w;
}

inline int getScreenH() {
	return SDL_GetVideoSurface()->h;
}

inline void sleep(int millis) {
	SDL_Delay(millis);
}

inline void showCursor(bool b) {
	SDL_ShowCursor(b ? SDL_ENABLE : SDL_DISABLE);
}

bool isKeyDown(int virtualKey);
string getCommandLine();

}}//end namespace

#endif
