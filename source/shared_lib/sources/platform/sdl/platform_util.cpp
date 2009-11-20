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

#include "pch.h"
#include "platform_util.h"

#include <iostream>
#include <sstream>
#include <cassert>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>

#include <SDL.h>

#include "util.h"
#include "conversion.h"
//#include "sdl_private.h"
#include "window.h"
#include "noimpl.h"
#include "platform_exception.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

namespace Private {
	bool shouldBeFullscreen = false;
	int ScreenWidth;
	int ScreenHeight;
}

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::singleton = NULL;

PlatformExceptionHandler::PlatformExceptionHandler() : installed(false) {
	memset(&old_sigill, 0, sizeof(old_sigill));
	memset(&old_sigsegv, 0, sizeof(old_sigsegv));
	memset(&old_sigabrt, 0, sizeof(old_sigabrt));
	memset(&old_sigfpe, 0, sizeof(old_sigfpe));
	memset(&old_sigbus, 0, sizeof(old_sigbus));
	assert(!singleton);
	singleton = this;
}

PlatformExceptionHandler::~PlatformExceptionHandler() {
	assert(singleton == this);
	if(installed) {
		uninstall();
	}
	singleton = NULL;
}

void PlatformExceptionHandler::install() {
	// FIXME: check return values of sigaction calls and report any errors (throw a nice PosixException)
	assert(this);
	assert(singleton == this);
	assert(!installed);
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = NULL;
	//action.sa_mask
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	action.sa_sigaction = PlatformExceptionHandler::handler;
#ifndef DEBUG
#endif
	sigaction(SIGILL, &action, &old_sigill);
	sigaction(SIGSEGV, &action, &old_sigsegv);
	sigaction(SIGABRT, &action, &old_sigabrt);
	sigaction(SIGFPE, &action, &old_sigfpe);
	sigaction(SIGBUS, &action, &old_sigbus);
	installed = true;
}

void PlatformExceptionHandler::uninstall() {
	assert(this);
	assert(singleton == this);
	assert(installed);
#ifndef DEBUG
#endif
	sigaction(SIGILL, &old_sigill, NULL);
	sigaction(SIGSEGV, &old_sigsegv, NULL);
	sigaction(SIGABRT, &old_sigabrt, NULL);
	sigaction(SIGFPE, &old_sigfpe, NULL);
	sigaction(SIGBUS, &old_sigbus, NULL);
	installed = false;
}

void PlatformExceptionHandler::handler(int signo, siginfo_t *info, void *context) {
	// how many times we've been here
	static int recursionCount = 0;
	static bool logged = false;
	++recursionCount;

	if(recursionCount > 3) {
		// if we've crashed more than three times, it means we can't even fprintf so just fuck it.
		exit(1);
	}

	if(recursionCount > 2) {
		// if we've crashed more than twice, it means that our error handler crashed in both pretty
		// and non-pretty modes
		fprintf(stderr, "Multiple segfaults, cannot correctly display error.  If you're lucky, the "
				"log is at least populated.\n");
		exit(1);
	}

	if (!logged && recursionCount == 1) {
		const char* signame;
		const char* sigcode;
		char description[128];

		void *array[256];
		size_t size = 256;
		char **strings;

		switch (signo) {

		case SIGILL:
			signame = "SIGILL";

			switch (info->si_code) {

			case ILL_ILLOPC:
				sigcode = "illegal opcode";
				break;

			case ILL_ILLOPN:
				sigcode = "illegal operand";
				break;

			case ILL_ILLADR:
				sigcode = "illegal addressing mode";
				break;

			case ILL_ILLTRP:
				sigcode = "illegal trap";
				break;

			case ILL_PRVOPC:
				sigcode = "privileged opcode";
				break;

			case ILL_PRVREG:
				sigcode = "privileged register";
				break;

			case ILL_COPROC:
				sigcode = "coprocessor error";
				break;

			case ILL_BADSTK:
				sigcode = "internal stack error";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGFPE:
			signame = "SIGFPE";

			switch (info->si_code) {

			case FPE_INTDIV:
				sigcode = "integer divide by zero";
				break;

			case FPE_INTOVF:
				sigcode = "integer overflow";
				break;

			case FPE_FLTDIV:
				sigcode = "floating point divide by zero";
				break;

			case FPE_FLTOVF:
				sigcode = "floating point overflow";
				break;

			case FPE_FLTUND:
				sigcode = "floating point underflow";
				break;

			case FPE_FLTRES:
				sigcode = "floating point inexact result";
				break;

			case FPE_FLTINV:
				sigcode = "floating point invalid operation";
				break;

			case FPE_FLTSUB:
				sigcode = "subscript out of range";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGSEGV:
			signame = "SIGSEGV";

			switch (info->si_code) {

			case SEGV_MAPERR:
				sigcode = "address not mapped to object";
				break;

			case SEGV_ACCERR:
				sigcode = "invalid permissions for mapped object";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGBUS:
			signame = "SIGBUS";

			switch (info->si_code) {

			case BUS_ADRALN:
				sigcode = "invalid address alignment";
				break;

			case BUS_ADRERR:
				sigcode = "non-existent physical address";
				break;

			case BUS_OBJERR:
				sigcode = "object specific hardware error";
				break;

			default:
				sigcode = "unrecognized si_code";
				break;
			}

			break;

		case SIGABRT:
			signame = "SIGABRT";
			sigcode = "";
			break;

		default:
			signame = "unexpected sinal";
			sigcode = "(wtf?)";
		}
		snprintf(description, sizeof(description), "%s: %s", signame, sigcode);

		size = backtrace(array, size);
		strings = backtrace_symbols(array, size);
		if(!strings) {
			fprintf(stderr, "%s\n", description);
			backtrace_symbols_fd(array, size, STDERR_FILENO);
		}
		singleton->log(description, info->si_addr, (const char**)strings, size, NULL);
		logged = true;
		free(strings);
	}

	// If this is our 1st entry, try a pretty, opengl-based error message.  Otherwise, resort to
	// primative format.
	singleton->notifyUser(recursionCount == 1);

	--recursionCount;

	exit(1);
}

// =====================================================
// class DirectoryListing
// =====================================================

DirectoryListing::DirectoryListing(const string &path_param)
		: path(path_param)
		, exists(false)
		, i(-1) {
	bzero(&globbuf, sizeof(globbuf));

	/* Stupid windows is searching for all files without extension when *. is
	 * specified as wildcard, so we have to make this behave the same
	 */
	if(path.compare(path.size() - 2, 2, "*.") == 0) {
		path = path.substr(0, path.size() - 2);
		path += "*";
	}

	if(glob(path.c_str(), 0, 0, &globbuf) < 0) {
		throw PosixException(
				"Error searching for files in directory '" + path + "'.  errno may be zero, in this case, please report as a bug.",
				"glob(path.c_str(), 0, 0, &globbuf)",
				NULL, __FILE__, __LINE__);
	}
	exists = true;
}

DirectoryListing::~DirectoryListing() {
	if(exists) {
		globfree(&globbuf);
	}
}

const char *DirectoryListing::getNext() {
	return ++i < globbuf.gl_pathc ? globbuf.gl_pathv[i] : NULL;
}


// =====================================
//         Misc
// =====================================


void mkdir(const string &path, bool ignoreDirExists) {
	if(::mkdir(path.c_str(), S_IRWXU | S_IROTH | S_IXOTH)) {
		int e = errno;
		if(!(ignoreDirExists && e == EEXIST)) {
			char buf[0x400];
			*buf = 0;
			strerror_r(e, buf, sizeof(buf));
			throw runtime_error("Failed to create directory " + path + ": " + buf);
		}
	}
}

size_t getFileSize(const string &path) {
	struct stat s;
	if(stat(path.c_str(), &s)) {
		char buf[0x400];
		*buf = 0;
		strerror_r(errno, buf, sizeof(buf));
		throw runtime_error("Failed to stat file " + path + ": " + buf);
	}
	return s.st_size;
}

bool changeVideoMode(int resW, int resH, int colorBits, int ) {
	Private::shouldBeFullscreen = true;
	return true;
}

void restoreVideoMode() {
}

void message(string message) {
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";
}

bool ask(string message) {
	std::cerr << "Confirmation: " << message << "\n";
	int res;
	std::cin >> res;
	return res != 0;
}

void exceptionMessage(const exception &excp) {
	std::cerr << "Exception: " << excp.what() << std::endl;
}

}}//end namespace
