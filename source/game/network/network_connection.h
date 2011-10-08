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

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "logger.h"

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
	//Socket *m_socket;
	//ENetHost *host;
	//ENetPeer *m_peer;

	// received but not processed messages
	MessageQueue messageQueue;

protected:
	// only child classes should be able to instantiate without socket
	//NetworkConnection() : m_socket(0) {}

	//virtual Socket* getSocket() {return m_socket;}
	//virtual const Socket* getSocket() const {return m_socket;}

public:
	//NetworkConnection(ENetPeer *peer) : m_peer(peer) {}
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
	//ServerSocket serverSocket;
	//ENetHost *m_serverHost;

protected:
	//virtual Socket* getSocket()				{return &serverSocket;}
	//virtual const Socket* getSocket() const	{return &serverSocket;}

public:
	int sendAnnounce(int port) {
		//serverSocket.sendAnnounce(port);
		return -1;
	}

	void bind(int port) {
		//serverSocket.bind(port);
	}

	void listen(int connectionQueueSize= SOMAXCONN) {
		//serverSocket.listen(connectionQueueSize);
	}

	NetworkConnection *accept() {
		/*Socket *socket = serverSocket.accept();
		if (socket) {
			return new NetworkConnection(socket);
		} else {
			return 0;
		}*/
		return 0;
	}
};

// =====================================================
//	class ClientConnection
// =====================================================

class ClientConnection : public NetworkConnection {
private:
	//ClientSocket clientSocket;
	//ENetHost *m_clientHost;

protected:
	//virtual Socket* getSocket()					{return &clientSocket;}
	//virtual const Socket* getSocket() const		{return &clientSocket;}

public:
	void connect(const Ip &ip, int port) {
		//clientSocket.connect(ip, port);
	}

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
