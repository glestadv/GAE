// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2010 James McCulloch
//				  2011 Nathan Turner
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "network_session.h"

#include "logger.h"

#include <cassert>

using std::deque;

namespace Glest { namespace Net {

using Glest::Util::Logger;

NetworkSession::NetworkSession(ENetPeer *peer)
	: m_peer(peer) {
}

NetworkSession::~NetworkSession() {
	if (m_peer)	{
		// Disconnect immediately
		enet_peer_disconnect_now(m_peer, DisconnectReason::DEFAULT);
		m_peer = NULL;
	}
}

void NetworkSession::setRemoteNames(const string &hostName, const string &playerName) {
	remoteHostName = hostName;
	remotePlayerName = playerName;

	stringstream str;
	str << remotePlayerName;
	if (!remoteHostName.empty()) {
		str << " (" << remoteHostName << ")";
	}
	description = str.str();
}

void NetworkSession::send(const Message* msg) {
	if (m_peer) {
		msg->log();

		ENetPacket *packet = enet_packet_create(msg->getData(), msg->getSize(), ENET_PACKET_FLAG_RELIABLE);

		if (enet_peer_send(m_peer, 0, packet) != 0) {
			NETWORK_LOG( "NetworkConnection::send(): Error trying to send " << MessageTypeNames[msg->getType()] << " message, connection severed." );
			throw Disconnect();
		}
	} else {
		NETWORK_LOG( "NetworkConnection::send(): Error trying to send " << MessageTypeNames[msg->getType()] << " message, m_peer is null." );
	}
}

void NetworkSession::disconnect(DisconnectReason reason) {
	if (m_peer) {
		//enet_peer_disconnect_now(m_peer, reason);
		//enet_peer_disconnect_later(m_peer, reason);
		enet_peer_disconnect(m_peer, reason);
		m_peer = NULL;
	}
}

void NetworkSession::disconnectNow(DisconnectReason reason) {
	if (m_peer) {
		enet_peer_disconnect_now(m_peer, reason);
		//enet_peer_disconnect_later(m_peer, reason);
		//enet_peer_disconnect(m_peer, reason);
		m_peer = NULL;
	}
}

void NetworkSession::reset() {
	if (m_peer) {
		enet_peer_reset(m_peer);
		m_peer = NULL;
	}
}

RawMessage NetworkSession::getNextMessage() {
	assert(hasMessage());
	RawMessage res = messageQueue.front();
	messageQueue.pop_front();
	return res;
}

}} // end namespace