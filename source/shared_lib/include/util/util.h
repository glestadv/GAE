// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_UTIL_H_
#define _SHARED_UTIL_UTIL_H_

#include <string>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <vector>
#include <ostream>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "math_util.h"
#include "net_util.h"
#include "types.h"

#if (defined(WIN32) || defined(WIN64)) && !_GNU_SOURCE
	#include <list>
	#define slist list

	using std::slist;
#else
	#include <ext/slist>

	using __gnu_cxx::slist;
#endif

namespace Shared { namespace Xml {
	class XmlNode;
}}

using std::string;
using std::vector;
using std::ostream;
using std::ostream;
using std::runtime_error;
using Shared::Platform::NetworkDataBuffer;
using Shared::Platform::NetSerializable;
using Shared::Platform::uint16;
using Shared::Xml::XmlNode;
using boost::shared_ptr;

namespace Shared { namespace Util {

const string sharedLibVersionString= "v0.4.1-gae";
/** A macro for defining C style enums that similtaneously stores their names. */
#define STRINGY_ENUM(name, countValue, ...)								\
	enum name {__VA_ARGS__, countValue};								\
	STRINGY_ENUM_NAMES(enum ## name ## Names, countValue, __VA_ARGS__)

// =====================================================
//	class EnumNames
// =====================================================

/** A utility class to provide a text description for enum values. */
class EnumNames : Uncopyable {
#ifdef NO_ENUM_NAMES
public:
    EnumNames(const char *valueList, size_t count, bool copyStrings, bool lazy) {}
    const char *getName(int i) {return "";}
#else
private:
	const char *valueList;
	const char **names;
	size_t count;
	bool copyStrings;

public:
	/** Primary ctor. As it turns out, not specifying copyStrings as true on most modern system will
	 * result in some form of access violation (due to attempting to write to read-only memory. */
	EnumNames(const char *valueList, size_t count, bool copyStrings, bool lazy);
    ~EnumNames();
    const char *getName(unsigned i) const {
		if(!names) {
			const_cast<EnumNames*>(this)->init();
		}
		return i < count ? names[i] : "invalid value";
	}

private:
	void init();
#endif
};

// =====================================================
// class Version
// =====================================================

class Version : public NetSerializable {
private:
	uint16 _major;
	uint16 _minor;
	uint16 revision;
	char bugFix;
	bool wip;

public:
	Version();
	Version(NetworkDataBuffer &buf);
	Version(uint16 _major, uint16 _minor, uint16 revision, char bugFix, bool wip);
	Version(const XmlNode &node);
//	Version(const string &s);
	// allow default copy ctor and operator=
	virtual ~Version() {}

	uint16 getMajor() const					{return _major;}
	uint16 getMinor() const					{return _minor;}
	uint16 getRevision() const				{return revision;}
	char getBugFix() const					{return bugFix;}
	bool isWip() const						{return wip;}
	bool isInitialized() const				{return _major || _minor || revision || bugFix || wip;}
	string toString() const;
	bool operator==(const Version &v) const	{return !memcmp(this, &v, sizeof(Version));} // FIXME: May break under some ABIs?
	bool operator!=(const Version &v) const	{return !operator==(v);}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
	void write (XmlNode &node) const;
};

class ObjectPrinter;

// =====================================================
// class Printable
// =====================================================

class Printable {
public:
	virtual ~Printable() {}
	virtual void print(ObjectPrinter &op) const = 0;
	string toString() const;
};

// =====================================================
// class ObjectPrinter
// =====================================================

class ObjectPrinter {
private:
	std::ostream &o;	/** Output destination. */
	string i;			/** Current indentation. */
	const string si;	/** Single indentation. */
	bool indentNext;	/** Flag used to omit indentation at the start of printing an object. */

public:
	ObjectPrinter(std::ostream &o, const string &si = string("  "));
	~ObjectPrinter();

	ObjectPrinter &beginClass(const char *name);
	ObjectPrinter &endClass();

	ObjectPrinter &print(const char *name, const Printable &p) {
		o << i << name << " = ";
		indentNext = false;
		p.print(*this);
		return *this;
	}

	ObjectPrinter &print(const char *name, bool value) {
		o << i << name << " = " << (value ? "true" : "false") << std::endl;
		return *this;
	}

	template<typename T>
	ObjectPrinter &print(const char *name, const T &value) {
		o << i << name << " = ";
		o << value;
		o << std::endl;
		return *this;
	}

	/** Prints a Printable object, but forces a non-virtual call. */
	template<class T>
	ObjectPrinter &printNonvirtual(const char *name, const T &p) {
		o << i << name << " = ";
		indentNext = false;
		p.T::print(*this);
		return *this;
	}

	ostream &getOstream() {return o;}
};

// =====================================================
// Miscellaneous functions
// =====================================================

const Version &getGlestSharedLibVersion();
const Version &getGaeSharedLibVersion();

void findAll(const string &path, vector<string> &results, bool cutExtension = false);

//string fcs
string cleanPath(const string &s);
string dirname(const string &s);
string basename(const string &s);
string cutLastExt(const string &s);
string ext(const string &s);
string replaceBy(const string &s, char c1, char c2);
string toLower(const string &s);
#if defined(WIN32) || defined(WIN64)
inline string intToHex ( int addr ) {
	static char hexBuffer[32];
	sprintf ( hexBuffer, "%.8X", addr );
	return string( hexBuffer );
}
#endif

inline string toLower(const string &s){
	size_t size = s.size();
	string rs;
	rs.resize(size);

	for(size_t i = 0; i < size; ++i) {
		rs[i] = tolower(s[i]);
	}

	return rs;
}

inline void copyStringToBuffer(char *buffer, int bufferSize, const string& s) {
	strncpy(buffer, s.c_str(), bufferSize - 1);
	buffer[bufferSize - 1] = '\0';
}

inline char *newStrOrNull(const char *src) {
	return src ? strcpy(new char[strlen(src) + 1], src) : NULL;
}

extern const char uu_base64[64];
void uuencode(char *dest, size_t *destSize, const void *src, size_t n);
void uudecode(void *dest, size_t *destSize, const char *src);

// ==================== numeric fcs ====================

template<typename T>
inline T clamp(T value, T min, T max) {
	if (value < min) {
		return min;
	} else if (value > max) {
		return max;
	} else {
		return value;
	}
}

inline float saturate(float value) {
	return clamp(value, 0.f, 1.f);
}


inline int round(float f) {
	return static_cast<int>(roundf(f));
}

inline int round(double d) {
	return static_cast<int>(::round(d));
}

//misc
bool fileExists(const string &path);

template<typename T>
void deleteValues(T beginIt, T endIt) {
	for (T it = beginIt; it != endIt; ++it) {
		delete *it;
	}
}

template<typename T>
void deleteMapValues(T beginIt, T endIt) {
	for (T it = beginIt; it != endIt; ++it) {
		delete it->second;
	}
}

}}//end namespace

#endif
