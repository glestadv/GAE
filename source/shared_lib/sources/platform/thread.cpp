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

#include "pch.h"
#include "thread.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "platform_exception.h"

#if USE_PTHREAD || USE_SDL
#	include "noimpl.h"
#endif

#include "leak_dumper.h"

namespace Shared { namespace Platform {

// =====================================================
// class Thread
// =====================================================

Thread::Thread() {
#if USE_PTHREAD
#endif
}

Thread::~Thread() {
}

/**
 * @fn void Thread::start()
 *
 */
/**
 * @fn void Thread::setPriority(Thread::Priority priority)
 *
 */
/**
 * @fn Thread::Priority Thread::getPriority() const
 *
 */
/**
 * @fn void Thread::kill()
 *
 */
/**
 * @fn void Thread::start()
 *
 */
/**
 * @fn bool Thread::join(int maxWaitMillis)
 * Waits a max of maxWaitMillis milliseconds for thread to die.
 * @return true if the thread terminated, false otherwise (i.e., maxWaitMillis lapsed or some
 * other error condition occured.
 */
#if USE_PTHREAD
sched_get_priority_max() and sched_get_priority_min() f
void Thread::start() {
	pthread_create(&thread, NULL, beginExecution, this);
}

void Thread::setPriority(Thread::Priority priority) {
	pthread_setschedprio(thread)

	pthread_t             thread;
	int                   rc = 0;
	struct sched_param    param;
	int                   policy = SCHED_OTHER;
	int                   theChangedPriority = 0;

	printf("Enter Testcase - %s\n", argv[0]);

	printf("Create thread using default attributes\n");
	rc = pthread_create(&thread, NULL, threadfunc, NULL);
	checkResults("pthread_create()\n", rc);

	sleep(2);  /* Sleep is not a very robust way to serialize threads */

	memset(&param, 0, sizeof(param));
	/* Bump the priority of the thread a small amount */
	if (thePriority - BUMP_PRIO >= PRIORITY_MIN_NP) {
		param.sched_priority = thePriority - BUMP_PRIO;
	}

	printf("Set scheduling parameters, prio=%d\n",
		   param.sched_priority);
	rc = pthread_setschedparam(thread, policy, &param);
	checkResults("pthread_setschedparam()\n", rc);

	/* Let the thread fill in its own last priority */
	theChangedPriority = showSchedParam(thread);

	if (thePriority == theChangedPriority ||
			param.sched_priority != theChangedPriority) {
		printf("The thread did not get priority set correctly, "
			   "first=%d last=%d expected=%d\n",
			   thePriority, theChangedPriority, param.sched_priority);
		exit(1);
	}

	sleep(5);  /* Sleep is not a very robust way to serialize threads */
	printf("Main completed\n");
	return 0;

}

Thread::Priority Thread::getPriority() const {
	struct sched_param param;
	int policy;
	int err;

	err = pthread_getschedparam(thread, &policy, &param);
	if(err) {
		PosixException::coldThrow("Failed to retreive thread priority.",
				"pthread_getschedparam", NULL, __FILE__, __LINE__, err);
	}
	return param.sched_priority;
}

void Thread::kill() {
	pthread_kill(thread, SIG_TERM);
}

bool Thread::join(int maxWaitMillis) {
	return !pthread_join(pthread_t thread, NULL);
}

#elif USE_SDL

void Thread::start() {
	thread = SDL_CreateThread(beginExecution, this);
	id = SDL_GetThreadID(thread);
}

void Thread::setPriority(Priority priority) {
	NOIMPL;
}

void Thread::kill() {
	SDL_KillThread(thread);
}

bool Thread::join(int maxWaitMillis) {
	SDL_WaitThread(thread, 0);
	return true;
}

#elif defined(WIN32)  || defined(WIN64)

void Thread::start() {
	thread = ::CreateThread(NULL, 0, beginExecution, this, 0, &id);
}

void Thread::setPriority(Priority priority) {
	::SetThreadPriority(thread, threadPriority);
}

void Thread::kill() {
	::TerminateThread(thread, 1);
}

bool Thread::join(int maxWaitMillis) {
	return ::WaitForSingleObject(thread, maxWaitMillis) == WAIT_OBJECT_0;
}

#endif

ThreadFuncReturnType Thread::beginExecution(void* param) {
	static_cast<Thread*>(param)->execute();
	return 0;
}

// =====================================================
// class Condition
// =====================================================

__cold void Condition::coldThrow(const char* func) {
	throw std::runtime_error("Call to " + std::string(func) + " failed.");
}

#if defined(WIN32)

Condition::Condition() : cond(CreateEvent(NULL, TRUE, FALSE, NULL)) {
	if(!cond) {
		throw WindowsException("Failed to create event.",
				"CreateEvent(NULL, TRUE, FALSE, NULL)",
				NULL, __FILE__, __LINE__);
	}
}

void Condition::signal() {
	if(!SetEvent(cond)) {
		throw WindowsException("Failed to set event.",
				"SetEvent(cond)",
				NULL, __FILE__, __LINE__);
	}
}

void Condition::wait(Mutex &mutex) {
	mutex.v();
	if(WaitForSingleObject(cond, INFINITE) == WAIT_FAILED) {
		mutex.p();
		throw WindowsException("Failed to wait on event/condition.",
				"WaitForSingleObject(cond, INFINITE)",
				NULL, __FILE__, __LINE__);
	}
	mutex.p();
}

bool Condition::wait(Mutex &mutex, size_t max) {
	mutex.v();
	int ret = WaitForSingleObject(cond, max);
	mutex.p();
	switch(ret) {
	case WAIT_OBJECT_0:
		return true;

	case WAIT_TIMEOUT:
		return false;

	default:
		throw WindowsException("Failed to wait on event/condition.",
				"WaitForSingleObject(cond, max)",
				NULL, __FILE__, __LINE__);
	}
}

#endif

}}//end namespace
