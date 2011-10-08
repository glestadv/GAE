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

#include "enet/enet.h"
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
//	class Ip
// =====================================================

Ip::Ip(){
	bytes[0] = 0;
	bytes[1] = 0;
	bytes[2] = 0;
	bytes[3] = 0;
}

Ip::Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3) {
	bytes[0] = byte0;
	bytes[1] = byte1;
	bytes[2] = byte2;
	bytes[3] = byte3;
}


Ip::Ip(const string& ipString) {
	int offset = 0;
	for (int byteIndex = 0; byteIndex < 4; ++byteIndex) {
		int dotPos = ipString.find_first_of('.', offset);
		bytes[byteIndex] = atoi(ipString.substr(offset, dotPos - offset).c_str());
		offset = dotPos + 1;
	}
}

string Ip::getString() const {
	return intToStr(bytes[0]) + "." + intToStr(bytes[1]) + "." + intToStr(bytes[2]) + "." + intToStr(bytes[3]);
}

// =====================================================
//	class Network
// =====================================================
void Network::init() {
	LOG_NETWORK("Initialize network");
	int result = enet_initialize();
	assert(result == 0);
}

void Network::deinit() {
	LOG_NETWORK("Deinitialize network");
	enet_deinitialize();
}

// =====================================================
//	class NetworkConnection
// =====================================================

void NetworkConnection::send(const Message* networkMessage) {
	networkMessage->log();
	networkMessage->send(this);
}

void NetworkConnection::send(const void* data, int dataSize) {
	/*if (getSocket()->send(data, dataSize) != dataSize) {
		LOG_NETWORK( "connection severed, trying to send message.." );
		throw Disconnect();
	}*/
}

bool NetworkConnection::receive(void* data, int dataSize) {
	/*Socket *socket = getSocket();
	int n = socket->getDataToRead();
	NETWORK_LOG( "\tReceiving, data to read: " << n );
	if (n >= dataSize) {
		if (socket->receive(data, dataSize)) {
			return true;
		}
		LOG_NETWORK( "connection severed, trying to read message." );
		throw Disconnect();
	}*/
	return false;
}

bool NetworkConnection::peek(void *data, int dataSize) {
	/*Socket *socket = getSocket();
	if (socket->getDataToRead() >= dataSize) {
		if (socket->peek(data, dataSize)) {
			return true;
		}
		LOG_NETWORK( "connection severed, trying to read message." );
		throw Disconnect();
	}*/
	return false;
}

void NetworkConnection::receiveMessages() {
	/*Socket *socket = getSocket();
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
	}*/


	/*
	ENetEvent event;
    
    // Wait up to 1000 milliseconds for an event.
    while (enet_host_service(host, &event, 1000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            printf("A new client connected from %x:%u.\n", 
                    event.peer->address.host,
                    event.peer->address.port);

            // Store any relevant client information here.
            event.peer->data = "Client information";

			if (peer != NULL) {
				enet_peer_reset(peer);
			}

			peer = event.peer;

			///@todo handle connections

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);

			{
			Data data;
			memcpy(&data, event.packet->data, event.packet->dataLength);

			printf("Number: %i Switch: %i Text: %s\n", data.Number, data.Switch, data.Text.c_str());
			}

            // Clean up the packet now that we're done using it.
            enet_packet_destroy(event.packet);
            
            break;
           
        case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconected.\n", event.peer->data);

            // Reset the peer's client information.
            event.peer->data = NULL;

			///@todo handle disconnects
        }
    }*/
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

// Private
void NetworkConnection::close() {
	/*if (m_socket) {
		m_socket->close();
	}
	delete m_socket;
	m_socket = NULL;
	*/
}

}} // end namespace
