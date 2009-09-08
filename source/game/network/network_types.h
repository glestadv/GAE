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

#ifndef _GAME_NET_NETWORKTYPES_H_
#define _GAME_NET_NETWORKTYPES_H_

#include <string>

#include "net_util.h"

using std::string;
using Shared::Platform::NetSerializable;
using Shared::Platform::NetworkDataBuffer;

namespace Game { namespace Net {
	
// =====================================================
//	class NetworkString
// =====================================================

template<size_t S> class NetworkString : public NetSerializable {
private:
	string s;

public:
	NetworkString() : s()						{}
	NetworkString(const string &s) : s(s)		{clamp();}
	NetworkString &operator=(const string &s)	{this->s = s; clamp(); return *this;}
//	const string &getString() const				{return s;}
	operator const string &() const				{return s;}
	size_t getNetSize() const					{return s.size() + 1;}
	size_t getMaxNetSize() const				{return S + 1;}	
	void write(NetworkDataBuffer &buf) const	{buf.SimpleDataBuffer::write(s);}
	void read(NetworkDataBuffer &buf)			{buf.SimpleDataBuffer::read(s); clamp();}

private:
	void clamp()								{if(s.size() > S) s.resize(S);}
};

}} // end namespace

#endif
