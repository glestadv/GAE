// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "network_info.h"

#include <cstdlib>
#include <ctime>

#include "timer.h"

namespace Game { namespace Net {

// =====================================================
//	class NetworkInfo
// =====================================================

NetworkInfo::NetworkInfo(const uint64 &uid)
		: ipAddress()
		, port(0)
		, localHostName("")
		, resolvedHostName("")
		, gameVersion()
		, protocolVersion()
		, uid(uid) {
}

NetworkInfo::NetworkInfo(const IpAddress &addr, uint16 port, const string &localHostName,
			const string &resolvedHostName, const Version &gameVersion,
			const Version &protocolVersion, const uint64 &uid)
		: ipAddress(ipAddress)
		, port(port)
		, localHostName(localHostName)
		, resolvedHostName(resolvedHostName)
		, gameVersion(gameVersion)
		, protocolVersion(protocolVersion)
		, uid(uid) {
}

NetworkInfo::NetworkInfo(const XmlNode &node)
		: ipAddress			(node.getChildStringValue("ipAddress"))
		, port				(node.getChildUIntValue("port"))
		, localHostName		(node.getChildStringValue("localHostName"))
		, resolvedHostName	(node.getChildStringValue("resolvedHostName"))
		, gameVersion		(*node.getChild("gameVersion"))
		, protocolVersion	(*node.getChild("protocolVersion"))
		, uid				(node.getChildUInt64Value("uid")) {
}

void NetworkInfo::write(XmlNode &node) const {
	node.addChild("ipAddress", ipAddress.toString());
	node.addChild("port", port);
	node.addChild("localHostName", localHostName);
	node.addChild("resolvedHostName", resolvedHostName);
	gameVersion.write(*node.addChild("gameVersion"));
	protocolVersion.write(*node.addChild("protocolVersion"));
	node.addChild("uid", uid);
}

void NetworkInfo::print(ObjectPrinter &op) const {
	op		.beginClass("NetworkInfo")
			.print("ipAddress", ipAddress.toString())
			.print("localHostName", localHostName)
			.print("resolvedHostName", resolvedHostName)
			.print("gameVersion", gameVersion.toString())
			.print("protocolVersion", protocolVersion.toString())
			.print("uid", uid)
			.endClass();
}

uint64 NetworkInfo::getRandomUid() {
	uint64 ret = 0;
	
	// seed with seconds since epoch XORed with whatever resolution the system clock is
	srand(time(NULL) ^ clock());
	
	// rotate the value's bits around while XORing with the result of rand()
	for(int i = 0; i < 16; ++i) {
		ret = ((ret  >> 48) | (ret << 16)) ^ rand();
	}
	
	// top of off by XORing with the address of ret and the current time in microseconds
	return ret ^ reinterpret_cast<uintptr_t>(&ret) ^ Shared::Platform::Chrono::getCurMicros();
}

}} // end namespace Game::Net
