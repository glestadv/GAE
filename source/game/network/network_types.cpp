// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "network_types.h"

#include "leak_dumper.h"

namespace Game { namespace Net {
/*
template<int S> 
size_t NetworkString::getNetSize() const {
	//return s.size() + sizeof(uint16);
	return sizeof(size) + size;
}

template<int S> 
size_t NetworkString::getMaxNetSize() const {
	return sizeof(size) + sizeof(buffer);
}

template<int S> 
void NetworkString::write(NetworkDataBuffer &buf) const {
	buf.write(size);
	buf.write(buffer, size);
}

template<int S> 
void NetworkString::read(NetworkDataBuffer &buf) {
	buf.read(size);
	assert(size < S);
	buf.read(buffer, size);
	buffer[size] = 0;
}
*/
}} // end namespace
