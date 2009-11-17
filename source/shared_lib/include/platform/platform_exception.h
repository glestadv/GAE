// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_PLATFORMEXCEPTION_H_
#define _SHARED_PLATFORM_PLATFORMEXCEPTION_H_

#include "exception_base.h"

using Shared::Util::GlestException;

namespace Shared { namespace Platform {

// ===============================
//  class PosixException
// ===============================

//#if defined(USE_SDL) || defined(USE_POSIX_SOCKETS)

/** Exception type to encapsulate posix errno and strerror_r calls. */
class PosixException : public GlestException {
public:
	PosixException(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			int err = errno) throw();
	virtual ~PosixException() throw();
	virtual PosixException *clone() const throw ();

	virtual string getType() const throw();

	static string getPosixErrorDesc(int err) throw();
	static __noreturn __cold void coldThrow(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			int err = errno);
};

// ===============================
//  class WindowsException
// ===============================

#if defined(WIN32) || defined(WIN64)

/** Exception type to encapsulate GetLastError and FormatMessage calls. */
class WindowsException : public GlestException {
public:
	WindowsException(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			DWORD err = GetLastError()) throw();
	virtual ~WindowsException() throw();
	virtual WindowsException *clone() const throw ();

	virtual string getType() const throw();

	static string getWindowsErrorDesc(DWORD err) throw();
	static __noreturn __cold void coldThrow(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			DWORD err = GetLastError());
};

// ===============================
//  class WinsockException
// ===============================

/** Exception type to encapsulate WSAGetLastError and FormatMessage calls. */
class WinsockException : public WindowsException {
public:
	WinsockException(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			DWORD err = WSAGetLastError()) throw();
	virtual ~WinsockException();
	virtual WinsockException *clone() const throw ();

	virtual string getType() const throw();

	static __noreturn __cold void coldThrow(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			DWORD err = WSAGetLastError());
};

#endif

}} // end namespace

#endif // _SHARED_PLATFORM_PLATFORMEXCEPTION_H_
