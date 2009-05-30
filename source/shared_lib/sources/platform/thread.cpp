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

#if defined(USE_PTHREAD) || defined(USE_SDL)
#	include "noimpl.h"
#endif

#include "leak_dumper.h"

namespace Shared { namespace Platform {

// =====================================================
// class Thread
// =====================================================
Thread::Thread()		{}
Thread::~Thread()		{}

#if defined(USE_PTHREAD)

void Thread::start()	{pthread_create(thread, NULL, beginExecution, this);}
void Thread::setPriority(Thread::Priority priority);{}
void Thread::suspend()	{NOIMPL;}
void Thread::resume()	{NOIMPL;}
void Thread::kill()		{pthread_kill(thread, SIG_TERM);}

/**
 * Waits a max of maxWaitMillis milliseconds for thread to die.
 * @return true if the thread terminated, false otherwise (i.e., maxWaitMillis lapsed or some
 * other error condition occured.
 */
bool Thread::join(int maxWaitMillis) {
	return !pthread_join(pthread_t thread, NULL);
}
#elif defined(USE_SDL)

void Thread::start()	{thread = SDL_CreateThread(beginExecution, this); id = SDL_GetThreadID(thread);}
void Thread::setPriority(Priority priority) {NOIMPL;}
void Thread::suspend()	{NOIMPL;}
void Thread::resume()	{NOIMPL;}
void Thread::kill()		{SDL_KillThread(thread);}
bool Thread::join(int maxWaitMillis) {SDL_WaitThread(thread, 0); return true;}

#elif defined(WIN32)  || defined(WIN64)

void Thread::start()	{thread = ::CreateThread(NULL, 0, beginExecution, this, 0, &id);}
void Thread::setPriority(Priority priority) {::SetThreadPriority(thread, threadPriority);}
void Thread::suspend()	{::SuspendThread(thread);}
void Thread::resume()	{::ResumeThread(thread);}
void Thread::kill()		{::TerminateThread(thread, 1);}
bool Thread::join(int maxWaitMillis) {return ::WaitForSingleObject(thread, maxWaitMillis) == WAIT_OBJECT_0;}

#endif

ThreadFuncReturnType Thread::beginExecution(void* param) {
	static_cast<Thread*>(param)->execute();
	return 0;
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
