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

#include "util.h"

#include <ctime>
#include <cassert>
#include <list>
#include <stdexcept>

#include "leak_dumper.h"
#include "platform_util.h"

#if defined(WIN32) || defined(WIN64)
	#include <list>
	#define slist list
#else
	#include <ext/slist>
	#include <glob.h>

	using __gnu_cxx::slist;
#endif

using namespace std;
using namespace Shared::Platform;

namespace Shared{ namespace Util{

// =====================================================
//	class SimpleDataBuffer
// =====================================================

inline void SimpleDataBuffer::compact() {
	if(!size()) {
		start = end = 0;
	} else {
		// rewind buffer
		if(start) {
			memcpy(buf, &buf[start], size());
			end -= start;
			start = 0;
		}
	}
}

void SimpleDataBuffer::expand(size_t minAdded) {
	compact();
	if(room() < minAdded) {
		size_t newSize = minAdded > bufsize * 0.2f ? minAdded + bufsize : bufsize * 1.2f;
		resizeBuffer(newSize);
	}
}

void SimpleDataBuffer::resizeBuffer(size_t newSize) {
	compact();
	if(!(buf = (char *)realloc(buf, bufsize = newSize))) {
		throw runtime_error("out of memory");
	}
	if(newSize < end) {
		end = newSize;
	}
}

// =====================================================
//	Global Functions
// =====================================================

//finds all filenames like path and stores them in results
void findAll(const string &path, vector<string> &results, bool cutExtension){
	slist<string> l;
	DirIterator di;
	char *p = initDirIterator(path, di);

	if(!p) {
		throw runtime_error("No files found: " + path);
	}

	do {
		// exclude current and parent directory as well as any files that start
		// with a dot, but do not exclude files with a leading relative path
		// component, although the path component will be stripped later.
		if(p[0] == '.' && !(p[1] == '/' || p[1] == '.' && p[2] == '/')) {
			continue;
		}
		char* begin = p;
		char* dot = NULL;

		for( ; *p != 0; ++p) {
			// strip the path component
			switch(*p) {
			case '/':
				begin = p + 1;
				break;
			case '.':
				dot = p;
			}
		}
		// this may zero out a dot preceding the base file name, but we
		// don't care
		if(cutExtension && dot) {
			*dot = 0;
		}
		l.push_front(begin);
	} while((p = getNextFile(di)));

	freeDirIterator(di);

	l.sort();
	results.clear();
	for(slist<string>::iterator li = l.begin(); li != l.end(); ++li) {
		results.push_back(*li);
	}
}

string lastDir(const string &s){
	size_t i= s.find_last_of('/');
	size_t j= s.find_last_of('\\');
	size_t pos;

	if(i==string::npos){
		pos= j;
	}
	else if(j==string::npos){
		pos= i;
	}
	else{
		pos= i<j? j: i;
	}

	if (pos==string::npos){
		throw runtime_error(string(__FILE__)+" lastDir - i==string::npos");
	}

	return (s.substr(pos+1, s.length()));
}

string cutLastFile(const string &s){
	size_t i= s.find_last_of('/');
	size_t j= s.find_last_of('\\');
	size_t pos;

	if(i==string::npos){
		pos= j;
	}
	else if(j==string::npos){
		pos= i;
	}
	else{
		pos= i<j? j: i;
	}

	if (pos==string::npos){
		throw runtime_error(string(__FILE__)+"cutLastFile - i==string::npos");
	}

	return (s.substr(0, pos));
}

string cutLastExt(const string &s){
     size_t i= s.find_last_of('.');

	 if (i==string::npos){
          throw runtime_error(string(__FILE__)+"cutLastExt - i==string::npos");
	 }

     return (s.substr(0, i));
}

string ext(const string &s){
     size_t i;

     i=s.find_last_of('.')+1;

	 if (i==string::npos){
          throw runtime_error(string(__FILE__)+"cutLastExt - i==string::npos");
	 }

     return (s.substr(i, s.size()-i));
}

string replaceBy(const string &s, char c1, char c2){
	string rs= s;

	for(size_t i=0; i<s.size(); ++i){
		if (rs[i]==c1){
			rs[i]=c2;
		}
	}

	return rs;
}


}}//end namespace
