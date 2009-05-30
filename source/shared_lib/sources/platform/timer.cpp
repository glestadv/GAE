// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "timer.h"

#include <stdexcept>

#include "leak_dumper.h"

namespace Shared { namespace Platform {


// =====================================================
//	class Chrono
// =====================================================

bool Chrono::initialized = Chrono::init();
int64 Chrono::freq;

/**
 * Returns a timestamp with micro-second precision in the format of YYYY-mm-dd HH:MM:SS.ssssss.
 */
string Chrono::getTimestamp(const int64 t) {
	struct tm timeinfo;
	char buf [80];
	time_t secondsUtc = static_cast<time_t>(t / 1000000);

	localtime_r(&secondsUtc, &timeinfo);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S.", &timeinfo);
	
	size_t len = strlen(buf);
	snprintf(&buf[len], sizeof(buf) - len, "%06u", static_cast<unsigned int>(t % 1000000));

	return string(buf);
}

#if defined(_CHRONO_USE_POSIX)

struct timezone Chrono::tz;

bool Chrono::init() {
	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;
	freq = 1000000;
	return true;
}

#elif defined(_CHRONO_USE_WIN)

bool Chrono::init() {
	if(!QueryPerformanceFrequency((LARGE_INTEGER*) &freq)) {
		throw runtime_error("Performance counters not supported");
	}
	return true;
}

#elif defined(_CHRONO_USE_SDL)

bool Chrono::init() {
	freq = 1000;
	return true;
}

#endif


// =====================================================
//	class FixedIntervalTimer
// =====================================================
FixedIntervalTimer::FixedIntervalTimer(float fps, size_t maxConsecutiveExecutions,
			bool restrictBacklogProcessing, size_t maxBacklog,
   			int64 curTime)
		: Scheduleable(curTime, 0)
		, fps(fps)
		, interval(static_cast<int64>(1000000.f / fps))
		, consecutiveExecutions(0)
		, maxConsecutiveExecutions(maxConsecutiveExecutions)
		, restrictBacklogProcessing(restrictBacklogProcessing)
		, maxBacklog(maxBacklog) {
//	reset();
}

void FixedIntervalTimer::setFps(float v) {
	fps = v;
	interval = static_cast<int64>(1000000.f / v);
	setNextExecution(getLastExecution() + interval);
}

bool FixedIntervalTimer::_isTime(const int64 &curTime) {
	// If we're restricting max consecutive executions and this prescribed number have occured
	// then we set the next execution time to interval uS from now, clear the count and return
	// false
	if(maxConsecutiveExecutions && consecutiveExecutions == maxConsecutiveExecutions) {
		setNextExecution(curTime + interval);
		consecutiveExecutions = 0;
		return false;
	}
	// Otherwise, we increment the count and check for backlog execution restrictions.
	++consecutiveExecutions;
	int64 elapsed = curTime - getLastExecution();
	if(restrictBacklogProcessing && elapsed / interval >= maxBacklog) {
		// if we've reached the max backlog, we must reset next & last execution time to
		// appear as if 
		int64 newLastExecTime = curTime - interval * maxBacklog;
		int64 newNextExecTime = newLastExecTime + interval;
		setLastExecution(newLastExecTime);
		setNextExecution(newNextExecTime);
	} else {
		setLastExecution(getLastExecution() + interval);
		setNextExecution(getNextExecution() + interval);
	}
	return true;
}

}}//end namespace
