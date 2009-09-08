// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_CHECKSUM_H_
#define _SHARED_UTIL_CHECKSUM_H_

#include <string>
#include <map>
#include <vector>

#include "socket.h"
#include "types.h"

using std::string;
using std::map;
using std::vector;
using Shared::Platform::int32;
using Shared::Platform::uint32;
using Shared::Platform::int8;
using Shared::Platform::NetSerializable;
using Shared::Platform::NetworkDataBuffer;

namespace Shared { namespace Util {

// =====================================================
//	class Checksum
// =====================================================

class Checksum {
private:
	int32 sum;
	int32 r;
	int32 c1;
	int32 c2;

public:
	Checksum();

	int32 getSum() const {return sum;}

	void addByte(int8 value);
	void addString(const string &value);
	void addFile(const string &path, bool text);
};

class Checksums : public NetSerializable {
public:
	enum CompareResult {
		COMPARE_RESULT_NON_MATCHING,
		COMPARE_RESULT_SELF_MISSING,
		COMPARE_RESULT_OTHER_MISSING,

		COMPARE_RESULT_COUNT
	};
	static const char *compareResultDescr[COMPARE_RESULT_COUNT];
	typedef map<string, CompareResult> ComparisonResults;
	/*
	class ComparisonResults : public map<string, CompareResult> {
	public:
		static const char *compareResultDescr[COMPARE_RESULT_COUNT];
		virtual ~ComparisonResults() {}
		string toString();
	};*/

private:
	typedef map<string, int32> FileChecksumMap;
	
	FileChecksumMap ckMap;

public:
	Checksums();
	
	void addFile(const string &path, bool text);
	bool compare(const Checksums &target, ComparisonResults &mismatches) const;

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
};

}} //end namespace

#endif
