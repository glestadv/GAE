// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_THREAD_H_
#define _SHARED_PLATFORM_THREAD_H_

#if defined(USE_PTHREADS)
#	include "noimpl.h"
// we should be using pthreads in favor of SDL when possible (IMHO), but we aren't
#elif defined(USE_SDL)
#	include <SDL_thread.h>
#	include <SDL_mutex.h>
#	include <stdexcept>
#
#	include "noimpl.h"
#
#	define THREAD_PRIORITY_IDLE				0
#	define THREAD_PRIORITY_BELOW_NORMAL		1
#	define THREAD_PRIORITY_NORMAL			2
#	define THREAD_PRIORITY_ABOVE_NORMAL		3
#	define THREAD_PRIORITY_TIME_CRITICAL	4
#	define INFINITE -1
#elif defined(WIN32)  || defined(WIN64)
#	include <windows.h>
#	include <stdexcept>
	using std::runtime_error;
#endif

#include "patterns.h"

namespace Shared { namespace Platform {

// =====================================================
//	class Thread
// =====================================================
#if defined(WIN32) || defined(WIN64)
	typedef LPTHREAD_START_ROUTINE ThreadFunction;
#endif

/**
 * An OS-level thread.
 * @see http://www.libsdl.org/docs/html/thread.html.
 */
class Thread : Uncopyable {
public:
	enum Priority {
		pIdle		= THREAD_PRIORITY_IDLE,
		pLow		= THREAD_PRIORITY_BELOW_NORMAL,
		pNormal		= THREAD_PRIORITY_NORMAL,
		pHigh		= THREAD_PRIORITY_ABOVE_NORMAL,
		pRealTime	= THREAD_PRIORITY_TIME_CRITICAL
	};

private:
	ThreadType thread;
	ThreadId id;
	Priority threadPriority;

public:
	Thread();
	virtual ~Thread();

	void start();
	void setPriority(Priority priority);
	void suspend();
	void resume();
	void kill();
	bool join(int maxWaitMillis = INFINITE);
	ThreadId getId() const {return id;}

protected:
	virtual void execute() = 0;
	
private:
	static ThreadFuncReturnType beginExecution(void *param);
};

// =====================================================
//	class Mutex
// =====================================================
/**
 * A mutex class.
 * @see http://www.libsdl.org/docs/html/thread.html.
 */
class Mutex : Uncopyable {
	friend class Condition;
	MutexType mutex;

public:
#ifdef USE_SDL
	Mutex() : mutex(SDL_CreateMutex()) {if (!mutex) throw std::runtime_error("Couldn't initialize mutex");}
	~Mutex() {SDL_DestroyMutex(mutex);}
	void p() {SDL_mutexP(mutex);}
	void v() {SDL_mutexV(mutex);}
#elif defined(WIN32)  || defined(WIN64)
	Mutex()  {::InitializeCriticalSection(&mutex);}
	~Mutex() {::DeleteCriticalSection(&mutex);}
	void p() {::EnterCriticalSection(&mutex);}
	void v() {::LeaveCriticalSection(&mutex);}
#endif
	MutexType &getObject() {return mutex;}
};

/**
 * MutexLock is a convenicnce and safety class to manage locking and unlocking
 * a mutex and is intended to be created on the stack.  The advantage of using
 * MutexLock over explicitly calling Mutex.p() and .v() is that you can never
 * forget to unlock the mutex.  Even if an exception is thrown, the stack unwind
 * will cause the mutex to be unlocked.
 * 
 * Note: Do NOT extend this class unless you modify the destructor to virutal (and then delete this
 * notation :).
 */
class MutexLock : Uncopyable {
private:
	Mutex &mutex;

public:
	MutexLock(Mutex &mutex) : mutex(mutex) {mutex.p();}
	~MutexLock() {mutex.v();}
};

/**
 * A thread condition class.
 * @see http://msdn.microsoft.com/en-us/library/ms682052(VS.85).aspx
 * @see http://www.libsdl.org/docs/html/thread.html.
 */
class Condition : Uncopyable {
private:
	ConditionType cond;

public:
#ifdef USE_SDL
	Condition() : cond(SDL_CreateCond()) {}
	~Condition()						{SDL_DestroyCond(cond);}
	/** Signal the first thread waiting on this condition. */
	void signal()						{throwFit(SDL_CondSignal(cond), "SDL_CondSignal");}
	/** Signal all threads waiting on this condition. */
	void broadcast()					{throwFit(SDL_CondBroadcast(cond), "SDL_CondBroadcast");}
	/** Wait for condition to be signled. */
	void wait(Mutex &mutex)				{throwFit(SDL_CondWait(cond, mutex.getObject()), "SDL_CondWait");}
	/**
	 * Wait for condition to be signled, specifying a max wait time.
	 * @return true if the condition was signaled, false if it timed out.
	 */
	bool wait(Mutex &mutex, size_t max) {
		int ret = SDL_CondWaitTimeout(cond, mutex.getObject(), max);
		throwFit(ret, "SDL_CondWaitTimeout");
		return ret != SDL_MUTEX_TIMEDOUT;
	}
private:
	void throwFit(int ret, const char* func) {
		if(ret == -1) {
			throw std::runtime_error("Call to " + std::string(func) + " failed.");
		}
	}
#elif defined(WIN64)
	Condition()							{InitializeConditionVariable(&cond);}
	~Condition()						{}
	void signal()						{WakeConditionVariable(&cond);}
	void broadcast()					{WakeAllConditionVariable(&cond);}
	void wait(Mutex &mutex)				{SleepConditionVariableCS(&cond, &mutex.getObject(), INFINITE);}
	bool wait(Mutex &mutex, size_t max) {return SleepConditionVariableCS(&cond, &mutex.getObject(), max) != WAIT_TIMEOUT;}
#elif defined(WIN32)
	Condition();
	~Condition()						{CloseHandle(cond);}
	void signal();
	void broadcast()					{throw runtime_error("not implemented");}
	void wait(Mutex &mutex);
	bool wait(Mutex &mutex, size_t max);
#endif
};

}} //end namespace

#endif
