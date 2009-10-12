// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
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

#include "math_util.h"

#if defined(WIN32) || defined(WIN64)
	#include <list>
	#define slist list

	using std::slist;
#else
	#include <ext/slist>

	using __gnu_cxx::slist;
#endif


using std::string;
using std::vector;
using std::runtime_error;

namespace Shared { namespace Xml {
	class XmlNode;
}}

namespace Shared { namespace Util {

const string sharedLibVersionString= "v0.4.1";


#define ENUMERATED_TYPE(Name,...)						\
	struct Name {										\
		enum Enum { __VA_ARGS__, COUNT };				\
		Name() : value((Enum)0) {}						\
		Name(Enum val) : value(val) {}					\
		operator Enum() { return value; }				\
		operator Enum() const { return value; }			\
	private:											\
		Enum value;										\
	};													\
    STRINGY_ENUM_NAMES(Name, Name::COUNT, __VA_ARGS__);
/*
		bool operator== (const Name &that) const {		\
			return value == that.value;					\
		}												\
		bool operator== (const Name &that) {			\
			return value == that.value;					\
		}												\
		bool operator== (Name &that) {					\
			return value == that.value;					\
		}												\
*/


/** A macro for defining C style enums that similtaneously stores their names. */
#define STRINGY_ENUM(name, countValue, ...)								\
	enum name {__VA_ARGS__, countValue};								\
	STRINGY_ENUM_NAMES(enum ## name ## Names, countValue, __VA_ARGS__)

// =====================================================
//	class EnumNames
// =====================================================

/** A utility class to provide a text description for enum values. */
class EnumNames {
#ifdef NO_ENUM_NAMES
public:
    EnumNames(const char *valueList, size_t count, bool copyStrings, bool lazy, const char *enumName = NULL) {}
    const char *operator[](int i) {return "";}
	template<typename EnumType>	EnumType match(string &value) { return EnumType::COUNT; }
#else
private:
	const char *valueList;
	const char **names;
	const char *qualifiedList;
	const char **qualifiedNames;
	size_t count;
	bool copyStrings;

public:
	/** Primary ctor. As it turns out, not specifying copyStrings as true on most modern system will
	 * result in some form of access violation (due to attempting to write to read-only memory. */
	EnumNames(const char *valueList, size_t count, bool copyStrings, bool lazy, const char *enumName = NULL);
    ~EnumNames();
    const char *operator[](unsigned i) const {
		if(!names) {
			const_cast<EnumNames*>(this)->init();
		}
		return i < count ? qualifiedNames ? qualifiedNames[i] : names[i] : "invalid value";
	}
	/** Matches a string value to an enum value  */
	template<typename EnumType>
	EnumType match(const char *value) const {
		if ( !names ) {
			const_cast<EnumNames*>(this)->init();
		}
		for ( unsigned int i=0; i < count; ++i ) {
			const char *ptr1 = names[i];
			const char *ptr2 = value;
			bool same = true;
			if ( !*ptr1 || !*ptr2 ) {
				continue;
			}
			while ( *ptr1 && *ptr2 ) {
				if ( isalpha(*ptr1) && isalpha(*ptr2) ) {
					if ( tolower(*ptr1) != tolower(*ptr2) ) {
						same = false;
						break;
					}
				} else if ( ! ( *ptr1 == '_' && (*ptr2 == ' ' || *ptr2 == '_') ) ) {
					same = false;
					break;
				}
				ptr1++;
				ptr2++;
				if ( ( !*ptr1 && *ptr2 && !isspace(*ptr2) ) || ( *ptr1 && !*ptr2 ) ) {
					same = false;
				}
			}
			if ( same ) {
				return (EnumType::Enum)i;
			}
		}
		return EnumType::COUNT;
	}

private:
	void init();
#endif
};

void findAll(const string &path, vector<string> &results, bool cutExtension = false);

//string fcs
string lastDir(const string &s);

inline string lastFile(const string &s){
	return lastDir(s);
}

string cutLastFile(const string &s);
string cutLastExt(const string &s);
string ext(const string &s);
string replaceBy(const string &s, char c1, char c2);

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
	strncpy(buffer, s.c_str(), bufferSize-1);
	buffer[bufferSize-1]= '\0';
}

extern const char uu_base64[64];
void uuencode(char *dest, size_t *destSize, const void *src, size_t n);
void uudecode(void *dest, size_t *destSize, const char *src);

// ==================== numeric fcs ====================

inline float saturate(float value){
	if (value<0.f){
        return 0.f;
	}
	if (value>1.f){
        return 1.f;
	}
    return value;
}

inline int clamp(int value, int min, int max){
	if (value<min){
        return min;
	}
	if (value>max){
        return max;
	}
    return value;
}

inline float clamp(float value, float min, float max){
	if (value<min){
        return min;
	}
	if (value>max){
        return max;
	}
    return value;
}

inline int round(float f){
     return (int) roundf(f);
}

//misc
bool fileExists(const string &path);

template<typename T>
void deleteValues(T beginIt, T endIt){
	for(T it= beginIt; it!=endIt; ++it){
		delete *it;
	}
}

template<typename T>
void deleteMapValues(T beginIt, T endIt){
	for(T it= beginIt; it!=endIt; ++it){
		delete it->second;
	}
}

class SimpleDataBuffer {
	char *buf;
	size_t bufsize;
	size_t start;
	size_t end;

public:
	SimpleDataBuffer(size_t initial) {
		if(!(buf = (char *)malloc(bufsize = initial))) {
			throw runtime_error("out of memory");
		}
		start = 0;
		end = 0;
	}

	virtual ~SimpleDataBuffer()		{free(buf);}
	size_t size() const				{return end - start;}
	size_t room() const				{return bufsize - end;}
	void *data()					{pinch(); return &buf[start];}
	void resize(size_t newSize)		{
		size_t min = size() + newSize;
		pinch();
		if(room() < min) {
			expand(min);
		}
		//ensureRoom(size() + newSize);
		end = start + newSize;
	}
	void pop(size_t bytes)			{assert(size() >= bytes); start += bytes;}
	void clear()					{start = end = 0;}
	void expand(size_t minAdded);
	void resizeBuffer(size_t newSize);
	void pinch() {
		if(end == start) {
			start = end = 0;
		}
	}

	void ensureRoom(size_t min) {
		pinch();
		if(room() < min) {
			expand(min);
		}
	}

	void peek(void *dest, size_t count) const {
		if(count > size()) {
			throw runtime_error("buffer underrun");
		}
		memcpy(dest, &buf[start], count);
	}

	void read(void *dest, size_t count) {
		peek(dest, count);
		start += count;
	}

	void write(const void *src, size_t count) {
		ensureRoom(count);
		memcpy(&buf[end], src, count);
		end += count;
	}

	void compressUuencodIntoXml(Shared::Xml::XmlNode *dest, size_t chunkSize);
	void uudecodeUncompressFromXml(const Shared::Xml::XmlNode *src);

protected:
	void compact();
};


}}//end namespace

#endif
