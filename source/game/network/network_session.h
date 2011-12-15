// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//				  2011 Nathan Turner
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef GLEST_NET_NETWORK_SESSION
#define GLEST_NET_NETWORK_SESSION

#include "network_message.h"
#include "enet/enet.h"

#include <stack>

namespace Glest { namespace Net {

// =====================================================
//	class NetworkSession
// =====================================================

WRAPPED_ENUM( DisconnectReason, DEFAULT, IN_GAME, NO_FREE_SLOTS )

class NetworkSession {
protected:
	typedef std::deque<RawMessage> MessageQueue;

private:
	string remoteHostName;
	string remotePlayerName;
	string description;

	ENetPeer *m_peer;

	// received but not processed messages
	MessageQueue messageQueue;

public:
	NetworkSession(ENetPeer *peer);
	~NetworkSession();

	string getRemoteHostName() const	{ return remoteHostName; }
	string getRemotePlayerName() const	{ return remotePlayerName; }
	string getDescription() const		{ return description; }

	// message retrieval
	bool hasMessage()					{ return !messageQueue.empty(); }
	int getMessageCount()               { return messageQueue.size(); }
	RawMessage getNextMessage();
	MessageType peekNextMsg() const		{ return MessageType(messageQueue.front().type); }
	RawMessage peekMessage() const		{ return messageQueue.front(); }
	void pushMessage(RawMessage raw)	{ messageQueue.push_back(raw); }

	bool isConnected() { return m_peer; }

	void setRemoteNames(const string &hostName, const string &playerName);
	void send(const Message* networkMessage);
	void disconnect(DisconnectReason reason);
	void disconnectNow(DisconnectReason reason);
	void reset();
};

}}//end namespace

#endif