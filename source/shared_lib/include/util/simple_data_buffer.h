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

#ifndef _SHARED_UTIL_SIMPLEDATABUFFER_H_
#define _SHARED_UTIL_SIMPLEDATABUFFER_H_

#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <string>

#include "patterns.h"

using std::string;
using std::runtime_error;

namespace Shared { namespace Xml {
	class XmlNode;
}}

namespace Shared { namespace Util {

// =====================================================
// class SimpleDataBuffer
// =====================================================

class SimpleDataBuffer : Uncopyable {
	char *buf;
	size_t bufsize;
	size_t start;
	size_t end;
	bool bad;

public:
	SimpleDataBuffer(size_t initial);
	virtual ~SimpleDataBuffer();

	size_t size() const				{return end - start;}
	bool empty() const				{return end == start;}
	bool isBad() const				{return bad;}
	size_t room() const				{assert(end <= bufsize && end >= start); return bufsize - end;}
	void *data()					{return &buf[start];}
	void clear()					{start = end = 0;}
	void expand(size_t minAdded);
	void compact();
	void resizeBuffer(size_t newSize);

	void pop(size_t bytes) {
		assert(size() >= bytes);
		start += bytes;
		if (start == end) {
			start = end = 0;
		}
	}

	void resize(size_t newSize) {
		size_t min = size() + newSize;
		if(room() < min) {
			expand(min);
		}
		//ensureRoom(size() + newSize);
		end = start + newSize;
	}

	void ensureRoom(size_t min) {
		if(room() < min) {
			expand(min);
		}
	}

	void peek(void *dest, size_t count, size_t skip = 0) const {
		if(count + skip > size()) {
			throw runtime_error("buffer underrun");
		}
		memcpy(dest, &buf[start + skip], count);
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

//	void peek(string &dest, size_t skip = 0) const;
	void read(string &dest);
	void write(const string &s)	{
		write(s.c_str(), s.size() + 1);
	}

	void compressUuencodIntoXml(Shared::Xml::XmlNode *dest, size_t chunkSize);
	void uudecodeUncompressFromXml(const Shared::Xml::XmlNode *src);

protected:
//	void pinch()					{if (start == end) start = end = 0;}
	void rewind();
};

}} // end namespace

#endif // _SHARED_UTIL_SIMPLEDATABUFFER_H_
