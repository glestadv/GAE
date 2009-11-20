// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_LOGGER_H_
#define _SHARED_UTIL_LOGGER_H_

#include <string>
#include <deque>
#include <stdexcept>
#include <time.h>
#include <sstream>

#include "timer.h"
#include "patterns.h"
#include "util.h"

using std::string;
using std::deque;
using std::runtime_error;
using std::stringstream;
using Shared::Util::Printable;
using Shared::Util::ObjectPrinter;

namespace Game {

// =====================================================
//	class Logger
//
/// Interface to write log files
// =====================================================

class Logger : Uncopyable {
private:
	static const int logLineCount;

	typedef deque<string> Strings;
	class FileHandler {
	private:
		FILE *f;

	public:
		FileHandler(const string &path) : f(fopen(path.c_str(), "at+")) {
			if(!f) {
				throw runtime_error("Error opening log file" + path);
			}
		}
		~FileHandler()		{fclose(f);}
		FILE *getHandle()	{return f;}
	};

private:
	string fileName;
	//string sectionName;
	string state;
	Strings logLines;
	stringstream ss;
	ObjectPrinter op;
	string subtitle;
	string current;
	static char errorBuf[];

private:
	Logger(const char *fileName);

public:
	static Logger &getInstance() {
		static Logger logger("glestadv.log");
		return logger;
	}

	static Logger &getServerLog() {
		static Logger logger("glestadv-server.log");
		return logger;
	}

	static Logger &getClientLog() {
		static Logger logger("glestadv-client.log");
		return logger;
	}

	static Logger &getErrorLog() {
		static Logger logger("glestadv-error.log");
		return logger;
	}

	void addXmlError(const string &path, const char *error);

	//void setFile(const string &v)			{fileName= v;}
	void setState(const string &state);
	void setSubtitle(const string &v)		{subtitle = v;}

	void add(const string &str);
	void add(const Printable &);
	void add(const string &str, const Printable &p);
	void printf(const char* pattern, ...);
	void clear();
};

// TODO: implement for Windows
#if defined(WIN32) | defined(WIN64)

class Timer {
public:
	Timer(int threshold, const char* msg) {}
	~Timer() {}

	void print(const char* msg) {}

	struct timeval getDiff() {return struct timeval();}
};

#else

class Timer {
	struct timeval start;
	struct timezone tz;
	unsigned int threshold;
	const char* msg;
	FILE *outfile;

public:
	Timer(int threshold, const char* msg, FILE *outfile = stderr)
			: threshold(threshold)
			, msg(msg)
			, outfile(outfile) {
		tz.tz_minuteswest = 0;
		tz.tz_dsttime = 0;
		gettimeofday(&start, &tz);
	}

	~Timer() {
		struct timeval diff = getDiff();
		unsigned int diffusec = diff.tv_sec * 1000000 + diff.tv_usec;
		if (diffusec > threshold) {
			fprintf(outfile, "%s: %d\n", msg, diffusec);
		}
	}

	void print(const char* msg) {
		struct timeval diff = getDiff();
		unsigned int diffusec = diff.tv_sec * 1000000 + diff.tv_usec;
		fprintf(outfile, "%s -> %s: %d\n", this->msg, msg, diffusec);
	}

	struct timeval getDiff() {
		struct timeval cur, diff;
		gettimeofday(&cur, &tz);
		diff.tv_sec = cur.tv_sec - start.tv_sec;
		diff.tv_usec = cur.tv_usec - start.tv_usec;
		return diff;
	}
};
#endif

} // end namespace

#endif
