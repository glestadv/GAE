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
#include "enet/enet.h"

using std::string;
using std::vector;
using Shared::Util::Checksum;
using Shared::Math::Vec3f;

namespace Glest { namespace Net {

// =====================================================
//	class IP
// =====================================================

class Ip {
private:
	unsigned char bytes[4];

public:
	Ip();
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
//	class NetworkConnection
// =====================================================

class NetworkConnection {
protected:
	typedef deque<RawMessage> MessageQueue;
	static const int readyWaitTimeout = 60000; // 60 sec

private:
	string remoteHostName;
	string remotePlayerName;
	string description;

	ENetHost *m_host;
	ENetPeer *m_peer;

	// received but not processed messages
	MessageQueue messageQueue;

protected:
	// only child classes should be able to instantiate without socket
	NetworkConnection() : m_host(0), m_peer(0) {}

	ENetHost* getHost() {return m_host;}
	const ENetHost* getSocket() const {return m_host;}

	void setHost(ENetHost *v) {m_host = v;}
	void destroyHost() { enet_host_destroy(m_host); }

	virtual void poll() {};

public:
	NetworkConnection(ENetHost *host, ENetPeer *peer) : m_host(host), m_peer(peer) {}
	virtual ~NetworkConnection() {close();}

	string getIp() const				{return /*getSocket()->getIp();*/ "";}
	string getHostName() const			{return /*getSocket()->getHostName();*/ "";}
	string getRemoteHostName() const	{return remoteHostName;}
	string getRemotePlayerName() const	{return remotePlayerName;}
	string getDescription() const		{return description;}
	int getReadyWaitTimeout()			{return readyWaitTimeout;}

	void setRemoteNames(const string &hostName, const string &playerName);
	void send(const Message* networkMessage);
	void send(const void* data, int dataSize);

	// message retrieval
	bool receive(void* data, int dataSize);
	bool peek(void *data, int dataSize);
	void receiveMessages();
	bool hasMessage()					{ return !messageQueue.empty(); }
	RawMessage getNextMessage();
	MessageType peekNextMsg() const;
	void pushMessage(RawMessage raw)	{ messageQueue.push_back(raw); }

	bool isConnected() const			{ return true;/*getSocket() && getSocket()->isConnected();*/ }
	void setBlock(bool b)				{ /*getSocket()->setBlock(b);*/}
	void setNoDelay()					{ /*getSocket()->setNoDelay();*/}

private:
	void close();
};

// =====================================================
//	class ServerConnection
// =====================================================
class ServerConnection : public NetworkConnection {
private:
	typedef std::map<int, NetworkConnection*> Connections;
	Connections m_connections;
	ENetAddress m_address;

	std::stack<NetworkConnection*> m_looseConnections;

protected:
	//virtual Socket* getSocket()				{return &serverSocket;}
	//virtual const Socket* getSocket() const	{return &serverSocket;}
	virtual void poll() override;

public:
	virtual ~ServerConnection() {
		NetworkConnection::destroyHost();
	}
	int sendAnnounce(int port) {
		//serverSocket.sendAnnounce(port);
		return -1;
	}

	void bind(int port);

	void listen(int connectionQueueSize = SOMAXCONN);

	NetworkConnection *accept() {
		if (!m_looseConnections.empty()) {
			NetworkConnection *connection = m_looseConnections.top();
			m_looseConnections.pop();
			return connection;
		} else {
			return 0;
		}
	}
};

// =====================================================
//	class ClientConnection
// =====================================================

class ClientConnection : public NetworkConnection {
private:
	NetworkConnection *m_server;
protected:
	//virtual Socket* getSocket()					{return &clientSocket;}
	//virtual const Socket* getSocket() const		{return &clientSocket;}

	virtual void poll() override;

public:
	virtual ~ClientConnection() {
		NetworkConnection::destroyHost();
	}
	void connect(const string &address, int port);

	/** @return ip of the sender
	  * @throws SocketException when socket is no longer receiving */
	Ip receiveAnnounce(int port, char *hostname, int dataSize) {
		//clientSocket.receiveAnnounce(port, hostname, dataSize);
	}

	void disconnectUdp() {
		//clientSocket.disconnectUdp();
	}
};

}}//end namespace

#endif
