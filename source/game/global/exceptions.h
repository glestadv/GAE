// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_EXCEPTIONS_H_
#define _GAME_EXCEPTIONS_H_

#include "platform_exception.h"
/*
#include "patterns.h"
using std::string;
using std::runtime_error;
using namespace Shared::Platform;
using Shared::Util::Cloneable;

namespace Game {

namespace Net {
	class GameInterface;
	class RemoteInterface;
	class NetworkMessage;
} // end namespace Net
using namespace Net;

// =====================================================
//  class GameException
// =====================================================
/ ** Base exception class for all in-game exceptions. * /
class GameException : public GlestException {
	string fileName;
	runtime_error *rootCause;

public:
	GameException(
			const string &msg,
			runtime_error *rootCause = NULL);
	GameException(
			const string &fileName,
			const string &msg,
			runtime_error *rootCause = NULL);
	virtual ~GameException() throw();
	virtual GameException *clone() const throw () {return new GameException(*this);}
};

class MapException : public GameException {
public:
	MapException(const string &fileName, const string &msg);
	virtual ~MapException() throw();
	virtual MapException *clone() const throw () {return new MapException(*this);}
};

// =====================================================
//  class NetworkCommException
// =====================================================

class NetworkCommException : public GameException {
private:
	GameInterface *gi;
	RemoteInterface *ri;
	NetworkMessage *netmsg;
public:
	NetworkCommException(
			const string &msg,
			GameInterface *gi,
			RemoteInterface *ri,
			NetworkMessage *netmsg,
			const string &fileName = "",
			runtime_error *rootCause = NULL);
	virtual ~NetworkCommException() throw();
	virtual NetworkCommException *clone() const throw () {return new NetworkCommException(*this);}
};

} // end namespace
*/
#endif
