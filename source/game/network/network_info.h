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

#ifndef _GAME_NET_NETWORKINFO_H_
#define _GAME_NET_NETWORKINFO_H_

#include <string>

#include "util.h"
#include "socket.h"
#include "xml_parser.h"

using std::string;
using Shared::Platform::IpAddress;
using Shared::Util::Printable;
using Shared::Xml::XmlNode;

namespace Game { namespace Net {

// =====================================================
//	class NetworkInfo
// =====================================================

/**
 * Encapsulates basic network information for a participant in a network game.
 */
class NetworkInfo : public Printable {
private:
	IpAddress ipAddress;
	uint16 port;
	string localHostName;
	string resolvedHostName;
	Version gameVersion;
	Version protocolVersion;
	uint64 uid;				/** A big fat ugly number that the server generates to uniquely identify
							 *  each client in the game, similiar to a sesson id. */

public:
	NetworkInfo(const uint64 &uid);
	NetworkInfo(const IpAddress &addr, uint16 port, const string &localHostName,
			const string &resolvedHostName, const Version &gameVersion,
			const Version &protocolVersion, const uint64 &uid);
	NetworkInfo(const XmlNode &node);
	// allow default copy constructor and assignment operator

	void write(XmlNode &node) const;

	const IpAddress &getIpAddress() const		{return ipAddress;}
	uint16 getPort() const						{return port;}
	const string &getLocalHostName() const		{return localHostName;}
	const string &getResolvedHostName() const	{return resolvedHostName;}
	const Version &getGameVersion() const		{return gameVersion;}
	const Version &getProtocolVersion() const	{return protocolVersion;}
	const uint64 &getUid() const				{return uid;}
	
	bool hasIpAndPort() const					{return !ipAddress.isZero();}
	bool hasLocalHostName() const				{return !localHostName.empty();}
	bool hasResolvedHostName() const			{return !resolvedHostName.empty();}
	bool hasVersionInfo() const					{return gameVersion.isInitialized();}
	bool hasUid() const							{return uid;}

	virtual void print(ObjectPrinter &op) const;
	static uint64 getRandomUid();

protected:
	void setIpAndPort(const IpAddress &ipAddress, uint16 port) {
		assert(!ipAddress.isZero() && port);
		this->ipAddress = ipAddress;
		this->port = port;
	}
	void setLocalHostName(const string &v)		{localHostName = v;}
	void setResolvedHostName(const string &v)	{resolvedHostName = v;}
	void setVersionInfo(const Version &gameVersion, const Version &protocolVersion) {
		assert(gameVersion.isInitialized() && protocolVersion.isInitialized());
		this->gameVersion = gameVersion;
		this->protocolVersion = protocolVersion;
	}
	void setUid(const uint64 &v)				{uid = v;}
};

}} // end namespace

#endif // _GAME_NET_NETWORKINFO_H_
