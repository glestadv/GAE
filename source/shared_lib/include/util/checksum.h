// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_CHECKSUM_H_
#define _SHARED_UTIL_CHECKSUM_H_

#include <string>

#include "types.h"

using std::string;
using Shared::Platform::int32;
using Shared::Platform::int8;

namespace Shared{ namespace Util{

// =====================================================
//	class Checksum
// =====================================================

class Checksum{
private:
	int32	sum;
	int32	r;
    int32	c1;
    int32	c2;

public:
	Checksum();

	int32 getSum() const	{return sum;}

	void addByte(int8 value);

	template <typename T>
	void add(T val) {
		for (unsigned i=0; i < sizeof(T); ++i) {
			int8 *byte = (int8*)&val + i;
			addByte(*byte);
		}
	}

	template <>
	void add<string>(string value) {
		for(int i=0; i < value.size(); ++i) {
			addByte(value[i]);
		}
	}
};

}}//end namespace

#endif
