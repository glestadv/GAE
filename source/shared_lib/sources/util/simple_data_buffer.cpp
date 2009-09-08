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

#include "pch.h"
#include "simple_data_buffer.h"

#include <algorithm>
#include <sstream>
#include <zlib.h>

#include "xml_parser.h"
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Platform;
using Shared::Xml::XmlNode;
using std::max;

namespace Shared { namespace Util {

// =====================================================
//	class SimpleDataBuffer
// =====================================================

/** Primary constructor.
 * @param initial the initial size of the internal buffer
 */
SimpleDataBuffer::SimpleDataBuffer(size_t initial)
		: buf((char *)malloc(initial))
		, bufsize(initial)
		, start(0)
		, end(0)
		, bad(false) {
	if(!buf) {
		bad = true;
		throw runtime_error("out of memory");
	}
}

SimpleDataBuffer::~SimpleDataBuffer() {
	free(buf);
}

/**
 * Rewinds the buffer so that any unused space at the beginning of the internal buffer is
 * eliminated, leaving the maxiumal amount of available space for new writes.  If there is no unused
 * space at the start of the internal buffer then this operation does nothing.  If there is no
 * data at all, the start and end pointers are simply reset to zero.  Otherwise, memcpy is called
 * to move existing data to the start of the buffer, thus, this function should not be called
 * liberally.
 */
inline void SimpleDataBuffer::rewind() {
	if(start) {
		if(end == start) {
			start = end = 0;
		} else {
			// rewind buffer
			memcpy(buf, &buf[start], size());
			end -= start;
			start = 0;
		}
	}
}

/**
 * Rewinds the and then resizes the internal buffer to the number of bytes specified by newSize.
 * @param newSize the number of bytes the resulting buffer should have when the function returns.
 * @throws runtime_error if call to realloc fails (indicating an out of memory condition).
 */
void SimpleDataBuffer::resizeBuffer(size_t newSize) {
	rewind();
	buf = static_cast<char *>(realloc(buf, bufsize = newSize));
	if(!buf) {
		bad = true;
		throw runtime_error("out of memory");
	}
	if(newSize < end) {
		end = newSize;
	}
}

/**
 * Expand internal buffer if min minCapacity bytes are not currently available.  In practice, if
 * minCapacity bytes are not available, the buffer is expanded by greater of either minCapacity or
 * 20% of the current buffer size -- this will serve to prevent numerous small increases.  In
 * addition, any empty space at the beginning of the internal buffer is removed (i.e., the data is
 * compacted) prior to reallocating the buffer.
 * @see rewind()
 * @see resizeBuffer()
 */
void SimpleDataBuffer::expand(size_t minCapacity) {
	if(room() < minCapacity) {
		size_t minCapacityNewBytes = minCapacity - room();
		size_t size20 = static_cast<size_t>(bufsize * 0.2f);
		size_t newSize = bufsize + (minCapacityNewBytes > size20
				? minCapacityNewBytes
				: size20);
		resizeBuffer(newSize);
	}
}

/**
 * Compacts the internal buffer to the minimal size needed to store the current data.  Warning:
 * if this object currently holds no data, the internal buffer will be shunken to zero bytes.
 */
void SimpleDataBuffer::compact() {
	resizeBuffer(size());
}

/* not currently used
void SimpleDataBuffer::peek(string &dest, size_t skip) const {
	size_t searchStart = start + skip;
	const char* startAddr = &buf[searchStart];
	if(searchStart >= end || !memchr(startAddr, 0, end - searchStart)) {
		throw runtime_error("buffer underrun");
	}
	dest = startAddr;
}*/

/**
 * Read a null terminated string from the SimpleDataBuffer and store the result in dest.
 * @param the destination for the string being read.
 * @throws runtime_error if this object does not currently contain a null-terminated string, as
 * 						 indicated by the presence, or lack thereof, of a null value.
 */
void SimpleDataBuffer::read(string &dest) {
	const char *pStart = &buf[start];
	const char *pEnd = static_cast<const char *>(memchr(pStart, 0, size()));
	if(!pEnd) {
		throw runtime_error("buffer underrun");
	}
	dest = pStart;
	start += pEnd - pStart + 1;
}

void SimpleDataBuffer::compressUuencodIntoXml(XmlNode *dest, size_t chunkSize) {
	size_t uuencodedSize;
	int zstatus;
	char *linebuf;
	int chunks;
	uLongf compressedSize;
	char *tmpbuf;

	// allocate temp buffer for compressed data
	compressedSize = (uLongf)(size() * 1.001f + 12.f);
	tmpbuf = (char *)malloc(max((size_t)compressedSize, chunkSize + 1));

	if(!tmpbuf) {
		std::stringstream str;
		str << "Out of Memory: failed to allocate " << compressedSize << " bytes of data.";
		throw runtime_error(str.str());
	}

	// compress data
	zstatus = ::compress2((Bytef *)tmpbuf, &compressedSize, (const Bytef *)data(), size(), Z_BEST_COMPRESSION);
	if(zstatus != Z_OK) {
		free(tmpbuf);
		throw runtime_error("error in zstream while compressing map data");
	}

	// shrink temp buffer
	tmpbuf = (char *)realloc(tmpbuf, max((size_t)compressedSize, chunkSize + 1));

	// write header info to xml node
	dest->addAttribute("size", static_cast<int>(size()));
	dest->addAttribute("compressed-size", static_cast<int>(compressedSize));
	dest->addAttribute("compressed", true);
	dest->addAttribute("encoding", "base64");

	// uuencode tmpbuf back into this
	clear();
	ensureRoom(compressedSize * 4 / 3 + 5);
	uuencodedSize = room();
	Shared::Util::uuencode(buf, &uuencodedSize, tmpbuf, compressedSize);
	resize(uuencodedSize);

	// shrink temp buffer again, this time we'll use it as a line buffer
	tmpbuf = (char *)realloc(tmpbuf, chunkSize + 1);

	// finally, break it apart into chunkSize byte chunks and cram it into the xml file
	chunks = (uuencodedSize + chunkSize - 1) / chunkSize;
	for(int i = 0; i < chunks; i++) {
		strncpy(tmpbuf, &((char*)data())[i * chunkSize], chunkSize);
		tmpbuf[chunkSize] = 0;
		dest->addChild("data", (const char*)tmpbuf);		// slightly inefficient, but oh well
	}
}

void SimpleDataBuffer::uudecodeUncompressFromXml(const XmlNode *src) {
	string s;
	char *tmpbuf;
	size_t expectedSize;
	uLongf actualSize;
	size_t compressedSize;
	size_t decodedSize;
	size_t encodedSize;
	char null = 0;

	expectedSize = src->getIntAttribute("size");
	compressedSize = src->getIntAttribute("compressed-size");
	encodedSize = compressedSize / 3 * 4 + (compressedSize % 3 ? compressedSize % 3 + 1 : 0) + 1;

	// retrieve encoded characters from xml document and store in buf
	clear();
	ensureRoom(max(expectedSize, encodedSize + 1));	// reduce buffer expansions
	for(int i = 0; i < src->getChildCount(); ++i) {
		s = src->getChild("data", i)->getStringAttribute("value");
		write(s.c_str(), s.size());
	}

	// add null terminator
	write(&null, 1);

	if(size() != encodedSize) {
		throw runtime_error("Error extracting uuencoded data from xml.  Expected "
				+ Conversion::toStr(encodedSize) + " characters of uuencoded data, but only found "
				+ Conversion::toStr(size()));
	}

	assert(strlen(buf) == size() - 1);

	// allocate tmpbuf to hold compressed (decoded) data
	tmpbuf = (char *)malloc(decodedSize = compressedSize);
	uudecode(tmpbuf, &decodedSize, buf);
	assert(decodedSize == compressedSize);

	clear();
	actualSize = room();
	int zstatus = ::uncompress((Bytef *)buf, &actualSize, (const Bytef *)tmpbuf, decodedSize);
	free(tmpbuf);

	if(zstatus != Z_OK) {
		throw runtime_error("error in zstream while decompressing map data in saved game");
	}

	if(actualSize != expectedSize) {
		throw runtime_error("Error extracting uuencoded data from xml.  Expected "
				+ Conversion::toStr(expectedSize) + " bytes of uncompressed data, but actual size was "
				+ Conversion::toStr(static_cast<uint64>(actualSize)));
	}

	resize(expectedSize);
}


}} // end namespace
