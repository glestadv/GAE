// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "platform_exception.h"
#include "leak_dumper.h"

typedef int error_t;

namespace Shared { namespace Platform {

// ===============================
//  class PosixException
// ===============================

PosixException::PosixException(
		const string &msg,
		const string &operation,
		const GlestException *rootCause,
		const string &fileName,
		long lineNumber,
		int err) throw()
		: GlestException(msg, operation, rootCause, fileName, lineNumber, err, getPosixErrorDesc(err)) {
}

PosixException::~PosixException() throw() {
}

PosixException *PosixException::clone() const throw () {
	return new PosixException(*this);
}

string PosixException::getType() const throw() {
	return string("PosixException");
}

string PosixException::getPosixErrorDesc(int err) throw() {
	char buf[1024];
#if defined(WIN32) || defined(WIN64)
	// stupid fuckers in redmond
	error_t result = strerror_s(buf, sizeof(buf), err);
	if(result) {
		snprintf(buf, sizeof(buf), "m$ crap can't find an error message, suprised?"
				"strerror_s returned %d.", result);
	}
	return string(buf);
#elif defined(_GNU_SOURCE)
	// GNU
	return string(strerror_r(err, buf, sizeof(buf)));
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
	// XSI
	strerror_r(err, buf, sizeof(buf));
	return string(buf);
#else
	// older libc
	return string(strerror(err));
#endif

}

#if defined(WIN32)  || defined(WIN64)

// ===============================
//  class WindowsException
// ===============================

WindowsException::WindowsException(
		const string &msg,
		const string &operation,
		const GlestException *rootCause,
		const string &fileName,
		long lineNumber,
		DWORD err) throw()
		: GlestException(msg, operation, rootCause, fileName, lineNumber, err, getWindowsErrorDesc(err)) {
}

WindowsException::~WindowsException() throw() {
}

WindowsException *WindowsException::clone() const throw () {
	return new WindowsException(*this);
}

string WindowsException::getType() const throw() {
	return string("WindowsException");
}

string WindowsException::getWindowsErrorDesc(DWORD err) throw() {
	LPCTSTR pszMessage;
	string str;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&pszMessage,
		0, NULL );
	str = pszMessage;
	::LocalFree((LPTSTR)pszMessage);
	return str;
}

// ===============================
//  class WinsockException
// ===============================

WinsockException::WinsockException(
		const string &msg,
		const string &operation,
		const GlestException *rootCause,
		const string &fileName,
		long lineNumber,
		DWORD err) throw()
	: WindowsException("Network error: " + msg, operation, rootCause, fileName, lineNumber, err) {
}

WinsockException::~WinsockException() throw() {
}

WinsockException *WinsockException::clone() const throw () {
	return new WinsockException(*this);
}

string WinsockException::getType() const throw() {
	return string("WinsockException");
}

#endif
}} // end namespace