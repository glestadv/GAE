// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_TIMER_H_
#define _SHARED_PLATFORM_TIMER_H_

// Prefer posix, native windows call (which could have uS precision) and finally SDL, which lacks
// microsecond precision
#if defined(HAVE_SYS_TIME_H)
	// use gettimeofday with microsecond precision, although the actual resolution may be anywhere
	// from one microsecond to 100 milliseconds.
#	define	_CHRONO_USE_POSIX
#	include <sys/time.h>
#elif defined(WIN32) || defined(WIN64)
	// use QueryPerformanceCounter with variable precision
	inline struct tm* localtime_r (const time_t *clock, struct tm *result) {
	if (!clock || !result) return NULL;
	memcpy(result,localtime(clock),sizeof(*result));
	return result;
}
#	define	_CHRONO_USE_WIN
#	include <winbase.h>
#elif defined(USE_SDL)
	// use SDL_GetTicks() with millisecond precision.
#	define	_CHRONO_USE_SDL
#	include <SDL.h>
#else
#	error No usable timer
#endif

#include <cassert>
#include <string>

#include "patterns.h"

using Shared::Platform::int64;
using namespace std;

namespace Shared { namespace Platform {

// =====================================================
//	class Chrono
// =====================================================

/**
 * Core class encapsulating low-level timing functionality.  The "current time" is gauranteed only
 * to be current within the process, it isn't gauranteed to match the system time since the goal
 * of this class is to provide the most accurate timing mechanism possible given the available
 * libraries, first trying Posix gettimeofday(), then Windows QueryPerformanceCounter() and finally
 * SDL's SDL_GetTicks() if none of the others are found.
 */
class Chrono {
private:
	int64 startTime;
	int64 stopTime;
	int64 accumTime;
#ifdef _CHRONO_USE_POSIX
	static struct timezone tz;
#endif
	static int64 freq;
	static bool initialized;

public:
	Chrono() : startTime(0), stopTime(0), accumTime(0) {
		assert(initialized);/*
		if(!initialized) {
			initialized = init();
		}*/
	}

	void start() {
	    getCurTicks(startTime);
	    stopTime = 0;
	}

	void stop() {
	    getCurTicks(stopTime);
		accumTime += stopTime - startTime;
	}

	const int64 &getStartTime() const	{return startTime;}
	const int64 &getStopTime() const	{return stopTime;}
	const int64 &getAccumTime() const	{return accumTime;}
	int64 getMicros() const				{return queryCounter(1000000);}
	int64 getMillis() const				{return queryCounter(1000);}
	int64 getSeconds() const			{return queryCounter(1);}
	float getFloatSeconds() const		{return static_cast<float>(queryCounter(1000000)) / 1000000.f;}
	double getDoubleSeconds() const		{return static_cast<double>(queryCounter(1000000)) / 1000000.;}
	static const int64 &getResolution()	{return freq;}
	static string getTimestamp(const int64 t = getCurMicros());

#if defined(_CHRONO_USE_POSIX)

	static int64 getCurMicros()			{return getCurTicks();}
	static int64 getCurMillis()			{return getCurTicks() / 1000;}
	static int64 getCurSeconds()		{return getCurTicks() / 1000000;}
	static void getCurTicks(int64 &dest){dest = getCurTicks();}
//	static int64 getCurMicros()			{return getCurTicks();}
	static int64 getCurTicks() {
		struct timeval now;
		gettimeofday(&now, &tz);
		return 1000000LL * now.tv_sec + now.tv_usec;
	}

#elif defined(_CHRONO_USE_WIN)

	static int64 getCurMicros()			{return getCurTicks() * 1000000 / freq;}
	static int64 getCurMillis()			{return getCurTicks() * 1000 / freq;}
	static int64 getCurSeconds()		{return getCurTicks() / freq;}
	static void getCurTicks(int64 &dest){QueryPerformanceCounter((LARGE_INTEGER*) &dest);}
	static int64 getCurTicks() {
		int64 now;
		QueryPerformanceCounter((LARGE_INTEGER*) &now);
		return now;
	}

#elif defined(_CHRONO_USE_SDL)

	static int64 getCurMicros()			{return SDL_GetTicks() * 1000;}
	static int64 getCurMillis()			{return SDL_GetTicks();}
	static int64 getCurSeconds()		{return SDL_GetTicks() / 1000;}
	static void getCurTicks(int64 &dest){dest = getCurTicks();}
	static int64 getCurTicks()			{return SDL_GetTicks();}

#endif

private:
	static bool init();
	int64 queryCounter(int multiplier) const {
		return multiplier * (accumTime + (stopTime ? 0 : getCurTicks() - startTime)) / freq;
	}
};

// =====================================================
//	class FixedIntervalTimer
// =====================================================

/**
 * A timer object using a fixed interval supporting maximum continuious execution and backlog
 * control.
 */
class FixedIntervalTimer : public Scheduleable {
private:
	float fps;							/** executions (or "frames") per second */
	int64 interval;						/** Same as fps, expressed in microseconds */
	size_t consecutiveExecutions;		/** The current number of consecutive executions that have occured */
	size_t maxConsecutiveExecutions;	/** The maximum number consecutive executions allowed or zero if no restriction */
	bool restrictBacklogProcessing;		/** If true, and execution occurs at a time when more than one execution should have already occured, maxBacklog will be used to determine how many executions should be made to catch up.  If false, excess time is discarded. */
	size_t maxBacklog;					/** If restrictBacklogProcessing is true, the number of backlogged executions to execute before discarding the lost time.  If restrictBacklogProcessing is false, this field is ignored. */

public:
	FixedIntervalTimer(float fps, size_t maxConsecutiveExecutions = 0,
			bool restrictBacklogProcessing = false, size_t maxBacklog = 0,
   			int64 curTime = Chrono::getCurMicros());

	float getFps() const						{return fps;}
	int64 getInterval() const					{return interval;}

	void setMaxConsecutiveExecutions(size_t v)	{maxConsecutiveExecutions = v;}
	void setBacklogRestrictions(bool restrictBacklogProcessing, size_t maxBacklog) {
		this->restrictBacklogProcessing = restrictBacklogProcessing;
		this->maxBacklog = maxBacklog;
	}
	void setFps(float v);
	void execute();
	void reset() {
		int64 curTime = Chrono::getCurMicros();
		setLastExecution(curTime - interval);
		setNextExecution(curTime);
	}

	bool isTime() {
		// allow inlining for simple part which will probably be executed most often and a function
		// call for the more complex portion that will be called less often.
		int64 curTime = Chrono::getCurMicros();
		if(curTime >= getNextExecution()) {
			return _isTime(curTime);
		}
		consecutiveExecutions = 0;
		return false;
	}

private:
	bool _isTime(const int64 &curTime);
};

}}//end namespace

#endif // _SHARED_PLATFORM_TIMER_H_
