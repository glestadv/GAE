// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "host.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"
#include "config.h"
#include "exceptions.h"
#include "client_interface.h"
#include "server_interface.h"
#include "game_util.h"

#include "leak_dumper.h"

//using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;
using Game::Config;

namespace Game { namespace Net {

// =====================================================
//	class Host
// =====================================================

Host::Host(NetworkRole role, int id, const uint64 &uid)
		: NetworkInfo(uid)
		, mutex()
		, cond()
		, player(id, "", false, false, true, this)
		, role(role)
		, state(STATE_UNCONNECTED)
		, paramChange(PARAM_CHANGE_NONE)
		, gameParam(GAME_PARAM_NONE)
		, targetFrame(0)
		, newSpeed(GAME_SPEED_NORMAL)/*
		, hostName()
		, playerName()
		, description()*/
		, lastFrame(0)
		, lastKeyFrame(0)/*
		, ipAddress()
		, port(0)*/ {
	memset(peerConnectionState, 0, sizeof(peerConnectionState));
}

Host::~Host() {
}

void Host::init(State initialState, unsigned short port, const IpAddress &ipAddress) {
	this->state = initialState;
	setIpAndPort(ipAddress, port);
}

void Host::handshake(const Version &gameVersion, const Version &protocolVersion) {
	assert(state < STATE_NEGOTIATED);
	state = STATE_NEGOTIATED;
	setVersionInfo(gameVersion, protocolVersion);
}

void Host::print(ObjectPrinter &op) const {
	stringstream connectionStatesStr;
	connectionStatesStr << "{";
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(i) {
			connectionStatesStr << ", ";
		}
		connectionStatesStr << Conversion::toStr(peerConnectionState[i]);
	}
	connectionStatesStr << "}";
	NetworkInfo::print(op.beginClass("Host"));
	op		.print("player", (const Printable &)player)
			.print("role", enumNetworkRoleNames.getName(role))
			.print("state", enumStateNames.getName(state))
			.print("paramChange", enumParamChangeNames.getName(paramChange))
			.print("gameParam", enumGameParamNames.getName(gameParam))
			.print("targetFrame", targetFrame)
			.print("newSpeed", enumGameSpeedNames.getName(newSpeed))
			.print("peerConnectionState", connectionStatesStr.str())
			.print("description", description)
			.print("lastFrame", lastFrame)
			.print("lastKeyFrame", lastKeyFrame)
			.print("port", port)
			.endClass();
}

}} // end namespace
