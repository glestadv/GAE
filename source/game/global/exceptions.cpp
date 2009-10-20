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


#include "pch.h"/*
#include "exceptions.h"
#include "leak_dumper.h"
#include "network_message.h"

using namespace Game::Net;

namespace Game {

// =====================================================
//  class GameException
// =====================================================
GameException::GameException(const string &msg, runtime_error *rootCause)
	: runtime_error(msg)
	, fileName("")
	, rootCause(rootCause) {
}

GameException::GameException(
		const string &fileName,
		const string &msg,
		runtime_error *rootCause)
	: runtime_error(msg)
	, fileName(fileName)
	, rootCause(rootCause) {
}

GameException::~GameException() throw() {}

// =====================================================
//  class MapException
// =====================================================

MapException::MapException(const string &fileName, const string &msg) :
		GameException(fileName, msg) {
}
MapException::~MapException() throw() {
}

// =====================================================
//  class NetworkCommException
// =====================================================


NetworkCommException::NetworkCommException(
			const string &msg,
			NetworkMessenger *gi,
			RemoteInterface *ri,
			NetworkMessage *netmsg,
			const string &fileName,
			runtime_error *rootCause)
	: GameException(fileName, msg, rootCause)
	, gi(gi)
	, ri(ri)
	, netmsg(netmsg) {
}

NetworkCommException::~NetworkCommException() throw() {
	if(netmsg) {
		delete netmsg;
	}
}

} // end namespace
*/