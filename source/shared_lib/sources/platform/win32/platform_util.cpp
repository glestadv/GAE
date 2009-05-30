// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "platform_util.h"
#include "platform_exception.h"

#include <io.h>
#include <cassert>

#include "util.h"
#include "conversion.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace std;

namespace Shared { namespace Platform {

// =====================================================
// class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::singleton = NULL;

PlatformExceptionHandler::PlatformExceptionHandler() : installed(false), old_filter(NULL) {
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

LONG WINAPI PlatformExceptionHandler::handler(LPEXCEPTION_POINTERS pointers) {

	DWORD exceptionCode = pointers->ExceptionRecord->ExceptionCode;
	ULONG_PTR *exceptionInfo = pointers->ExceptionRecord->ExceptionInformation;

	string description = codeToStr(exceptionCode);

	if (exceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		description += " (";
		description += exceptionInfo[0] == 0 ? "Reading" : "Writing";
		description += " address 0x" + intToHex(static_cast<int>(exceptionInfo[1])) + ")";
	}

	singleton->log(description.c_str(), pointers->ExceptionRecord->ExceptionAddress, NULL, 0, NULL);
	singleton->notifyUser(false);

	return EXCEPTION_EXECUTE_HANDLER;
}

void PlatformExceptionHandler::install() {
	assert(this);
	assert(!installed);
	assert(singleton == this);
	old_filter = SetUnhandledExceptionFilter(handler);
	installed = true;
}

void PlatformExceptionHandler::uninstall() {
	assert(this);
	assert(installed);
	assert(singleton == this);
	SetUnhandledExceptionFilter(old_filter);
	installed = false;
}

string PlatformExceptionHandler::codeToStr(DWORD code) {
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:		return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:	return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:				return "Breakpoint";
	case EXCEPTION_DATATYPE_MISALIGNMENT:	return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:	return "Float denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:		return "Float divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:		return "Float inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:	return "Float invalid operation";
	case EXCEPTION_FLT_OVERFLOW:			return "Float overflow";
	case EXCEPTION_FLT_STACK_CHECK:			return "Float stack check";
	case EXCEPTION_FLT_UNDERFLOW:			return "Float underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:		return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:			return "In page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:		return "Integer divide by zero";
	case EXCEPTION_INT_OVERFLOW:			return "Integer overflow";
	case EXCEPTION_INVALID_DISPOSITION:		return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:		return "Private instruction";
	case EXCEPTION_SINGLE_STEP:				return "Single step";
	case EXCEPTION_STACK_OVERFLOW:			return "Stack overflow";
	default:								return "Unknown exception code";
	}
}

// =====================================================
// class DirectoryListing
// =====================================================

DirectoryListing::DirectoryListing(const string &path)
		: path(path)
		, exists(false)
		, count(0)
		, handle(NULL) {
	bzero(&fi, sizeof(fi));
	handle = _findfirst64(path.c_str(), &fi);

	if(handle != -1) {
		exists = true;
	} else if(errno != ENOENT) {
		throw PosixException(
				("Error searching for files in directory '" + path + "'").c_str(),
				"_findfirst64(path.c_str(), &fi)",
				NULL, __FILE__, __LINE__);
	}
}

DirectoryListing::~DirectoryListing() {
	if(handle != -1) {
		_findclose(handle);
	}
}

const char *DirectoryListing::getNext() {
	// ok, this works fine and is concise, but is probably too obscure
	//return exists && (!count++ || !_findnext64(handle, &fi)) ? fi.name : NULL;

	if(!exists) {
		return NULL;
	}

	// if this is the 1st element, just return fi.name, otherwise, get the next element.
	if(!count++) {
		return fi.name;
	} else {
		return !_findnext64(handle, &fi) ? fi.name : NULL;
	}
}

// =====================================================
// class Misc
// =====================================================

void mkdir(const string &path, bool ignoreDirExists) {
	if (!CreateDirectory(path.c_str(), NULL)) {
		DWORD err = GetLastError();
		if (!(ignoreDirExists && err == ERROR_ALREADY_EXISTS)) {
			throw runtime_error("Failed to create directory " + path);
		}
	}
}

size_t getFileSize(const string &path) {
	HANDLE h = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int err;
	size_t ret;
	if (!h) {
		throw runtime_error("Failed to open file " + path);
	}
	ret = GetFileSize(h, NULL);
	if (ret == INVALID_FILE_SIZE) {
		CloseHandle(h);
		throw runtime_error("Failed to get file size for file " + path);
	}
	CloseHandle(h);
	return ret;
}



bool changeVideoMode(int resW, int resH, int colorBits, int refreshFrequency) {
	DEVMODE devMode;

	for (int i = 0; EnumDisplaySettings(NULL, i, &devMode) ;i++) {
		if (devMode.dmPelsWidth == resW &&
				devMode.dmPelsHeight == resH &&
				devMode.dmBitsPerPel == colorBits) {

			devMode.dmDisplayFrequency = refreshFrequency;

			LONG result = ChangeDisplaySettings(&devMode, 0);
			if (result == DISP_CHANGE_SUCCESSFUL) {
				return true;
			} else {
				return false;
			}
		}
	}

	return false;
}

void restoreVideoMode() {
	int dispChangeErr = ChangeDisplaySettings(NULL, 0);
	assert(dispChangeErr == DISP_CHANGE_SUCCESSFUL);
}

void message(string message) {
	MessageBox(NULL, message.c_str(), "Message", MB_OK);
}

bool ask(string message) {
	return MessageBox(NULL, message.c_str(), "Confirmation", MB_YESNO) == IDYES;
}

void exceptionMessage(const exception &excp) {
	string message, title;
	showCursor(true);

	message += "ERROR(S):\n\n";
	message += excp.what();

	title = "Error: Unhandled Exception";
	MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONSTOP | MB_OK | MB_TASKMODAL);
}


}
}//end namespace
