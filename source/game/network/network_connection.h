// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_NETWORKCONNECTION_H_
#define _GLEST_GAME_NETWORKCONNECTION_H_

#include <string>
#include <vector>
#include <stack>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "logger.h"
#include "network_session.h"
#include "enet/enet.h"

using std::string;
using std::vector;
using Shared::Util::Checksum;
using Shared::Math::Vec3f;

namespace Glest { namespace Net {

class NetworkInterface;

// =====================================================
//	class IP
// =====================================================

class Ip {
private:
	union {
		uint8  bytes[4];
		uint32 asUInt;
	};

public:
	Ip();
	Ip(uint32 val);
	Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3);
	Ip(const string& ipString);

	unsigned char getByte(int byteIndex)	{return bytes[byteIndex];}
	string getString() const;
};

// =====================================================
//	class Network
// =====================================================
class Network {
public:
	static void init();
	static void deinit();
};

// =====================================================
//	class NetworkHost
// =====================================================

class NetworkHost {
private:
	typedef vector<NetworkSession*> Sessions;
	Sessions m_sessions;
	ENetHost *m_host;
	string description;

	static const int readyWaitTimeout = 60000; // 60 sec

public:
	NetworkHost(/*int updateTimeout*/);
	virtual ~NetworkHost();

	void setHost(ENetHost *v) {assert(!m_host); m_host = v;}
	
	string getHostName();
	string getIp();
	int getSessionCount()					{return m_sessions.size();}
	NetworkSession *getSession(int index)	{assert(m_sessions.size() > index); return m_sessions[index];}
	int getReadyWaitTimeout()				{return readyWaitTimeout;}
	string getDescription() const			{return description;}

	void flush();
	void update(NetworkInterface *networkInterface);
	void disconnect(DisconnectReason reason);
	
	void broadcastMessage(const Message* networkMessage);
protected:
	bool isHostSet() {return m_host;}
	void setPeerCount(size_t count);
};

// =====================================================
//	class ClientHost
// =====================================================

/// A NetworkHost that contains a single session for the server peer.
class ClientHost : public NetworkHost {
public:
	void connect(const string &address, int port);
	bool isConnected();
	void send(const Message* networkMessage);
	NetworkSession *getServerSession();

private:
	void connect(ENetHost *client, ENetAddress *eNetAddress);
};

// =====================================================
//	class ServerHost
// =====================================================

/// A NetworkHost that allows clients to connect.
class ServerHost : public NetworkHost {
private:
	ENetAddress m_address;

public:
	void bind(int port);
	void listen(int connectionQueueSize = SOMAXCONN);
};

}}//end namespace

#endif
