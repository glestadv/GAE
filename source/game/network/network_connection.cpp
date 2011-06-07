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

#include "pch.h"
#include "network_connection.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"
#include "world.h"

#include "leak_dumper.h"
#include "logger.h"
#include "network_message.h"
#include "script_manager.h"
#include "command.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class NetworkConnection
// =====================================================

void NetworkConnection::send(const Message* networkMessage) {
	networkMessage->log();
	networkMessage->send(this);
}

void NetworkConnection::send(const void* data, int dataSize) {
	if (getSocket()->send(data, dataSize) != dataSize) {
		LOG_NETWORK( "connection severed, trying to send message.." );
		throw Disconnect();
	}
}

bool NetworkConnection::receive(void* data, int dataSize) {
	Socket *socket = getSocket();
	int n = socket->getDataToRead();
	NETWORK_LOG( "\tReceiving, data to read: " << n );
	if (n >= dataSize) {
		if (socket->receive(data, dataSize)) {
			return true;
		}
		LOG_NETWORK( "connection severed, trying to read message." );
		throw Disconnect();
	}
	return false;
}

bool NetworkConnection::peek(void *data, int dataSize) {
	Socket *socket = getSocket();
	if (socket->getDataToRead() >= dataSize) {
		if (socket->peek(data, dataSize)) {
			return true;
		}
		LOG_NETWORK( "connection severed, trying to read message." );
		throw Disconnect();
	}
	return false;
}

void NetworkConnection::receiveMessages() {
	Socket *socket = getSocket();
	if (!socket->isConnected()) {
		return;
	}
	size_t n = socket->getDataToRead();
	while (n >= MsgHeader::headerSize) {
		MsgHeader header;
		socket->peek(&header, MsgHeader::headerSize);
		if (n >= MsgHeader::headerSize + header.messageSize) {
			RawMessage rawMsg;
			rawMsg.type = header.messageType;
			rawMsg.size = header.messageSize;
			rawMsg.data = new uint8[header.messageSize];
			socket->skip(MsgHeader::headerSize);
			if (header.messageSize) {
				socket->receive(rawMsg.data, header.messageSize);
			} else {
				rawMsg.data = 0;
			}
			messageQueue.push_back(rawMsg);
			n = socket->getDataToRead();
		} else {
			return;
		}
	}
}

RawMessage NetworkConnection::getNextMessage() {
	assert(hasMessage());
	RawMessage res = messageQueue.front();
	messageQueue.pop_front();
	return res;
}

MessageType NetworkConnection::peekNextMsg() const {
	return enum_cast<MessageType>(messageQueue.front().type);
}

void NetworkConnection::setRemoteNames(const string &hostName, const string &playerName) {
	remoteHostName = hostName;
	remotePlayerName = playerName;

	stringstream str;
	str << remotePlayerName;
	if (!remoteHostName.empty()) {
		str << " (" << remoteHostName << ")";
	}
	description = str.str();
}

void NetworkConnection::close() {
	if (m_socket) {
		m_socket->close();
	}
	delete m_socket;
	m_socket = NULL;
}

}} // end namespace
