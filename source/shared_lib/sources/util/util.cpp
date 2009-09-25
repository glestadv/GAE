// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "util.h"

#include <ctime>
#include <cassert>
#include <list>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include "platform_util.h"
// Big time HACK! Windoze doesn't have strtok_r(), 
// I belive from your use of the 'buffer' parameter that this is 'safe'
// but if you actually need the re-entrant version, this will need revisiting :)
#if defined(WIN32) || defined(WIN64)
#	define strtok_r(a,b,c) strtok(a,b)
#endif

#include "leak_dumper.h"
#include "xml_parser.h"

using std::stringstream;
using Shared::Platform::DirectoryListing;

namespace Shared { namespace Util {

// =====================================================
//	class EnumNames
// =====================================================

#ifndef NO_ENUM_NAMES
EnumNames::EnumNames(const char *valueList, size_t count, bool copyStrings, bool lazy)
		: valueList(valueList)
		, names(NULL)
		, count(count)
		, copyStrings(copyStrings) {
	if(!lazy) {
		init();
	}
}

EnumNames::~EnumNames() {
	if(names) {
		delete[] names;
		if(copyStrings) {
			delete[] valueList;
		}
	}
}

void EnumNames::init() {
	size_t curName = 0;
	bool needStart = true;

	assert(!names);
	names = new const char *[count];
	if(copyStrings) {
		valueList = strcpy(new char[strlen(valueList) + 1], valueList);
	}

	for(char *p = const_cast<char*>(valueList); *p; ++p) {
		if(isspace(*p)) { // I don't want to have any leading or trailing whitespace
			*p = 0;
		} else if(needStart) {
			// do some basic sanity checking, even though the compiler should catch any such errors
			assert(isalpha(*p) || *p == '_');
			assert(curName < count);
			names[curName++] = p;
			needStart = false;
		} else if(*p == ',') {
			assert(!needStart);
			needStart = true;
			*p = 0;
		}
    }
    assert(curName == count);
}
#endif

// =====================================================
// class Version
// =====================================================

Version::Version()
		: _major(0)
		, _minor(0)
		, revision(0)
		, bugFix(0)
		, wip(false) {
}

Version::Version(NetworkDataBuffer &buf) {
	read(buf);
}

Version::Version(uint16 _major, uint16 _minor, uint16 revision, char bugFix, bool wip)
		: _major(_major)
		, _minor(_minor)
		, revision(revision)
		, bugFix(bugFix)
		, wip(wip) {
}

Version::Version(const XmlNode &node)
		: _major(node.getChildUIntValue("major"))
		, _minor(node.getChildUIntValue("minor"))
		, revision(node.getChildUIntValue("revision"))
		, bugFix(node.getChildStringValue("bugFix").c_str()[0])
		, wip(node.getChildBoolValue("wip")) {
}

void Version::write(XmlNode &node) const {
	const char bugFixStr[2] = {bugFix, 0};

	node.addChild("major", _major);
	node.addChild("minor", _minor);
	node.addChild("revision", revision);
	node.addChild("bugFix", bugFixStr);
	node.addChild("wip", wip);
}

string Version::toString() const {
	stringstream str;
	str << _major << "." << _minor << "." << revision;
	if(bugFix) {
		assert(bugFix >= 'a' && bugFix <= 'z');
		str << bugFix;
	}
	if(wip) {
		str << "-wip";
	}

	return str.str();
}

size_t Version::getNetSize() const {
	return getMaxNetSize();
}

size_t Version::getMaxNetSize() const {
	return sizeof(_major)
		 + sizeof(_minor)
		 + sizeof(revision)
		 + sizeof(bugFix) + 1;
}

void Version::read(NetworkDataBuffer &buf) {
	buf.read(_major);
	buf.read(_minor);
	buf.read(revision);
	buf.read(bugFix);
	buf.read(wip);
}

void Version::write(NetworkDataBuffer &buf) const {
	buf.write(_major);
	buf.write(_minor);
	buf.write(revision);
	buf.write(bugFix);
	buf.write(wip);
}
// =====================================================
// class Printable
// =====================================================

string Printable::toString() const {
	stringstream str;
	ObjectPrinter op(str);
	print(op);
	return str.str();
}

// =====================================================
// class ObjectPrinter
// =====================================================

ObjectPrinter::ObjectPrinter(std::ostream &o, const string &si)
		: o(o), i(), si(si), indentNext(true) {
}

ObjectPrinter::~ObjectPrinter() {
}

ObjectPrinter &ObjectPrinter::beginClass(const char *name) {
	if(indentNext) {
		o << i;
	}
	indentNext = true;
	o << name << " {" << std::endl;
	i += si;
	return *this;
}

ObjectPrinter &ObjectPrinter::endClass() {
	assert(i.size() >= si.size());
	i.resize(i.size() - si.size());
	o << i << "}" << std::endl;
	return *this;
}

// =====================================================
// Global Variables & Functions
// =====================================================

const Version glestSharedLibVersion(0, 2, 7, 0, false);
const Version gaeSharedLibVersion(0, 2, 12, 0, true);

const Version &getGlestSharedLibVersion() {
	return glestSharedLibVersion;
}

const Version &getGaeSharedLibVersion() {
	return gaeSharedLibVersion;
}

const char uu_base64[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

void uuencode(char *dest, size_t *destSize, const void *src, size_t n) {
	char *p;
	const unsigned char *s;
	size_t remainder;
	size_t expectedSize;
	const unsigned char *lastTriplet;

	p = dest - 1;
	s = (unsigned char *)src;
	remainder = n % 3;
	expectedSize = n / 3 * 4 + (remainder ? remainder + 1 : 0) + 1; // how many bytes we expect to write
	assert(*destSize >= expectedSize);
	lastTriplet = &s[n - remainder - 1];

	for(; s < lastTriplet; s += 3) {
		*(++p) = uu_base64[ s[0] >> 2];
		*(++p) = uu_base64[(s[0] << 4 & 0x30) | s[1] >> 4];
		*(++p) = uu_base64[(s[1] << 2 & 0x3c) | s[2] >> 6];
		*(++p) = uu_base64[ s[2] & 0x3f];
	}

	if(remainder > 0) {
		*(++p) = uu_base64[s[0] >> 2];

		if(remainder == 1) {
			*(++p) = uu_base64[ s[0] << 4 & 0x30];
		} else {
			*(++p) = uu_base64[(s[0] << 4 & 0x30) | s[1] >> 4];
			*(++p) = uu_base64[ s[1] << 2 & 0x3c];
		}
	}
	*(++p) = 0;
	*destSize = ++p - dest;
	assert(*destSize == expectedSize);
}

inline unsigned char decodeChar(char c) {
	if(c >= 'a') {
		return c - 'a' + 26;
	} else if(c >= 'A') {
		return c - 'A';
	} else if(c >= '0') {
		return c - '0' + 52;
	} else if(c == '+') {
		return 62;
	} else if(c == '/') {
		return 63;
	} else {
		assert(false);
		throw runtime_error("invalid character passed to decodeChar.");
	}
}

void uudecode(void *dest, size_t *destSize, const char *src) {
	size_t n;
	unsigned char *p;
	size_t remainder;
	size_t expectedSize;
	const char *lastQuartet;

	n = strlen(src);
	p = (unsigned char *)dest - 1;
	--src;
	remainder = n % 4;
	assert(remainder != 1);		// a remainder of one is never valid
	expectedSize = (n / 4) * 3 + (remainder ? remainder - 1 : 0);
	lastQuartet = &src[n - remainder];

	// This is some slightly ugly recycling of variables, but it's to help the compiler keep
	// everything in registers: src, p, lastQuartet, a and b.
	while(src < lastQuartet) {
		unsigned char a, b;
		a = decodeChar(*(++src));
		b = decodeChar(*(++src));
		*(++p) = (a << 2 & 0xfc) | b >> 4;

		a = decodeChar(*(++src));
		*(++p) = (b << 4 & 0xf0) | a >> 2;

		b = decodeChar(*(++src));
		*(++p) = (a << 6 & 0xc0) | b;
	}

	if(remainder > 0) {
		unsigned char a, b;
		a = decodeChar(*(++src));
		b = decodeChar(*(++src));
		*(++p) = (a << 2 & 0xfc) | b >> 4;

		if(remainder == 3) {
			a = decodeChar(*(++src));
			*(++p) = (b << 4 & 0xf0) | a >> 2;
		}
	}
	*destSize = ++p - (unsigned char *)dest;
	assert(*destSize == expectedSize);
}


//finds all filenames like path and stores them in results
void findAll(const string &path, vector<string> &results, bool cutExtension){
	slist<string> l;
	DirectoryListing dir(path);

	if(!dir.isExists()) {
		throw runtime_error("No files found: " + path);
	}

	// FIXME: Is it ok to modify the buffers returned by directory iteration functions?  If so,
	// remove this comment, otherwise, we should copy it.
	for(char *p = const_cast<char *>(dir.getNext()); p; p = const_cast<char *>(dir.getNext())) {
		// exclude current and parent directory as well as any files that start
		// with a dot, but do not exclude files with a leading relative path
		// component, although the path component will be stripped later.
		if(p[0] == '.' && !(p[1] == '/' || (p[1] == '.' && p[2] == '/'))) {
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
	};

	l.sort();
	results.clear();
	for(slist<string>::iterator li = l.begin(); li != l.end(); ++li) {
		results.push_back(*li);
	}
}

/**
 * Cleans up a path.
 * - converting any backslashes to slashes
 * - removes duplicate path delimiters
 * - eliminates all entries that specify the current directory
 * - if a "paraent directory" entry is found (two dots) and a path element exists before it, then
 *   both are removed (i.e., factored out)
 * - If the resulting path is the current directory, then an empty string is returned.
 */
string cleanPath(const string &s) {
	string result;
	vector<const char *> elements;
	char *buf = strcpy(new char[s.length() + 1], s.c_str());
	char *data;
	bool isAbsolute;

	// empty input gets empty output
	if(!s.length()) {
		return s;
	}

	isAbsolute = (s.at(0) == '/' || s.at(0) == '\\');
	
	for(char *p =  strtok_r(buf, "\\/", &data); p; p = strtok_r(NULL, "\\/", &data)) {
		// skip duplicate path delimiters
		if(strlen(p) == 0) {
			continue;

		// skip entries that just say "the current directory"
		} else if(!strcmp(p, ".")) {
			continue;

		// If an entry referrs to the parent directory and we have that, then we just drop the
		// parent and shorten the whole path
		} else if(!strcmp(p, "..") && elements.size()) {
			elements.pop_back();
		} else {
			elements.push_back(p);
		}
	}

	for(vector<const char *>::const_iterator i = elements.begin(); i != elements.end(); ++i) {
		if(result.length() || isAbsolute) {
			result.push_back('/');
		}
		result.append(*i);
	}

	delete[] buf;
	return result;
}

string dirname(const string &s) {
	string clean = cleanPath(s);
	int pos = clean.find_last_of('/');

	/* This does the same thing, which one is more readable?
	return pos == string::npos ? string(".") : (pos ? clean.substr(0, pos) : string("/"));
	*/

	if(pos == string::npos) {
		return string(".");
	} else if(pos == 0) {
		return string("/");
	} else {
		return clean.substr(0, pos);
	}
}

string basename(const string &s) {
	string cleaned = cleanPath(s);
	int pos = cleaned.find_last_of('/');

	// cleanPath() promises that the last character wont be the slash, so "pos + 1" should be safe.
	return pos == string::npos ? cleaned : cleaned.substr(pos + 1);
}

string cutLastExt(const string &s) {
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

// ==================== misc ====================

bool fileExists(const string &path){
	FILE* file= fopen(path.c_str(), "rb");
	if(file!=NULL){
		fclose(file);
		return true;
	}
	return false;
}

string toLower(const string &s) {
	size_t size = s.size();
	string rs;
	rs.resize(size);

	for(size_t i = 0; i < size; ++i) {
		rs[i] = tolower(s[i]);
	}

	return rs;
}

}}//end namespace
