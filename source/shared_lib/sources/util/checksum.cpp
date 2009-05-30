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
#include "checksum.h"

#include <cassert>
#include <stdexcept>

#include "util.h"

#include "leak_dumper.h"


using namespace std;

namespace Shared { namespace Util {

// =====================================================
// class Checksum
// =====================================================

Checksum::Checksum() {
	sum = 0;
	r = 55665;
	c1 = 52845;
	c2 = 22719;
}

void Checksum::addByte(int8 value) {
	int32 cipher = (value ^(r >> 8));

	r = (cipher + r) * c1 + c2;
	sum += cipher;
}

void Checksum::addString(const string &value) {
	for (int i = 0; i < value.size(); ++i) {
		addByte(value[i]);
	}
}

void Checksum::addFile(const string &path, bool text) {

	FILE* file = fopen(path.c_str(), "rb");

	if (!file) {
		throw runtime_error("Can not open file: " + path);
	}

	addString(lastFile(path));

	while (!feof(file)) {
		int8 byte = 0;

		fread(&byte, 1, 1, file);
		if (text && !isprint(byte)) {
			continue;
		}
		addByte(byte);
	}

	fclose(file);
}

// =====================================================
// class Checksums
// =====================================================

const char *Checksums::compareResultDescr[COMPARE_RESULT_COUNT] = {
	"NonMatching",
	"MissingLocally",
	"MissingRemotely"
};
/*
string Checksums::ComparisonResults::toString() {
	stringstream str;
	Lang &lang = Lang::getInstance();
	for(const_iterator i = this->begin(); i != this->end(); ++i) {
		assert(i->second >=0 && i->second < COMPARE_RESULT_COUNT);
		str << lang.format(compareResultDescr[i->second], i->first.c_str()) << endl;
	}
}
*/
// =====================================================
// class Checksums
// =====================================================

Checksums::Checksums() {
}

void Checksums::addFile(const string &path, bool text) {
	Checksum c;
	if(path.empty()) {
		throw runtime_error("path is empty");
	}
	c.addFile(path, text);
	ckMap[path] = c.getSum();
}

bool Checksums::compare(const Checksums &target, ComparisonResults &mismatches) const {
	FileChecksumMap::const_iterator i;
	FileChecksumMap::const_iterator found;
	mismatches.clear();

	for(i = ckMap.begin(); i != ckMap.end(); ++i) {
		found = target.ckMap.find(i->first);
		if(found == target.ckMap.end()) {
			mismatches[i->first] = COMPARE_RESULT_OTHER_MISSING;
		} else if(found->second != i->second) {
			mismatches[i->first] = COMPARE_RESULT_NON_MATCHING;
		}
	}

	// find files missing locally (or extraneous remote files)
	for(i = target.ckMap.begin(); i != target.ckMap.end(); ++i) {
		found = ckMap.find(i->first);
		if(found == ckMap.end()) {
			mismatches[i->first] = COMPARE_RESULT_SELF_MISSING;
		}
	}

	return mismatches.size();
}

size_t Checksums::getNetSize() const {
	size_t size = sizeof(uint32);	// tells number of entries
	for(FileChecksumMap::const_iterator i = ckMap.begin(); i != ckMap.end(); ++i) {
		size += sizeof(uint32); 	// actual checksum
		size += i->first.size() + 1;
	}
	return size;
}

size_t Checksums::getMaxNetSize() const {
	return 32768; // an arbitrary number really
}

void Checksums::read(NetworkDataBuffer &buf) {
	uint32 numEntries;
	buf.read(numEntries);
	for(uint32 i = 0; i < numEntries; ++i) {
		int32 checksum;
		int8 chr;
		string path;

		buf.read(checksum);
		
		// read characters until we hit a null
		for(buf.read(chr); chr; buf.read(chr)) {
			assert(chr);
			path.push_back(chr);
		}
		ckMap[path] = checksum;
	}
}

void Checksums::write(NetworkDataBuffer &buf) const {
	uint32 numEntries = ckMap.size();
	buf.write(numEntries);
	for(FileChecksumMap::const_iterator i = ckMap.begin(); i != ckMap.end(); ++i) {
		buf.write(i->second);								// write the 4 byte sum
		buf.write(i->first.c_str(), i->first.size() + 1);	// write the file name plus null term
	}
}

}}//end namespace
