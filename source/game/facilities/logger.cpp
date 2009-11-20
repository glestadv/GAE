// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "logger.h"

#include <stdarg.h>

#include "util.h"
#include "renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "lang.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;

namespace Game {

// =====================================================
// class Logger
// =====================================================

const int Logger::logLineCount = 15;

// ===================== PUBLIC ========================

Logger::Logger(const char *fileName)
		: fileName(fileName)
//		, sectionName()
		, state()
		, logLines()
		, ss()
		, op(ss) {
}

void Logger::setState(const string &state) {
	this->state = state;
	logLines.clear();
}

void Logger::add(const string &str) {
	FileHandler f(fileName);
	fprintf(f.getHandle(), "%s: %s\n", Chrono::getTimestamp().c_str(), str.c_str());

	logLines.push_back(str);
	if (logLines.size() > logLineCount) {
		logLines.pop_front();
	}
}

void Logger::add(const Printable &p) {
	stringstream _ss;
	ObjectPrinter _op(_ss);
	p.print(_op);
	add(_ss.str());
}

void Logger::add(const string &str, const Printable &p) {
	stringstream _ss;
	ObjectPrinter _op(_ss);
	_ss << str;
	p.print(_op);
	add(_ss.str());
}

void Logger::printf(const char* fmt, ...) {
	va_list ap;
	FileHandler f(fileName);
	fprintf(f.getHandle(), "%s: ", Chrono::getTimestamp().c_str());
	va_start(ap, fmt);
	vfprintf(f.getHandle(), fmt, ap);
	va_end(ap);
}

#if 0
void Logger::addLoad(const string &text, bool renderScreen){
	appendText(text, "Loading", renderScreen);
}

void Logger::addInit(const string &text, bool renderScreen) {
	appendText(text, "Initializing", renderScreen);
}

void Logger::addDelete(const string &text, bool renderScreen) {
	appendText(text, "Deleting", renderScreen);
}
#endif

void Logger::clear() {
	string s = "Log file\n";

	FILE *f = fopen(fileName.c_str(), "wt+");
	if (!f) {
		throw runtime_error("Error opening log file" + fileName);
	}

	fprintf(f, "%s", s.c_str());
	fprintf(f, "\n");

	fclose(f);
}

void Logger::addXmlError(const string &path, const char *error) {
	static char buffer[2048];
	sprintf(buffer, "XML Error in %s:\n %s", path.c_str(), error);
	add(buffer);
}

} // end namespace
