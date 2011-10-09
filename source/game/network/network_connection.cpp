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
	// could do, to avoid round trip
	/*
	ENetPacket *packet = enet_packet_create(networkMessage->getData(), networkMessage->getSize(), ENET_PACKET_FLAG_RELIABLE);
    
    if (enet_peer_send(peer, 0, packet) != 0) {
		LOG_NETWORK("connection severed, trying to send message..");
		throw Disconnect();
	}*/
}

void NetworkConnection::send(const void* data, int dataSize) {
	ENetPacket *packet = enet_packet_create(data, dataSize, ENET_PACKET_FLAG_RELIABLE);
    
    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    if (enet_peer_send(m_peer, 0, packet) != 0) {
		LOG_NETWORK("connection severed, trying to send message..");
		throw Disconnect();
	}
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
	poll();
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
	if (m_peer)
	{
		ENetEvent event;
    
		enet_peer_disconnect(m_peer, 0);

		/* Allow up to 3 seconds for the disconnect to succeed
			and drop any packets received packets.
		 */
		while (enet_host_service(m_host, &event, 3000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				puts("Disconnection succeeded.");
				return;
			}
		}
	    
		/* We've arrived here, so the disconnect attempt didn't */
		/* succeed yet.  Force the connection down.             */
		enet_peer_reset(m_peer);
		
		m_peer = NULL;
	}
}

// Protected
void ServerConnection::bind(int port) {    
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */

    m_address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    m_address.port = port;
}

void ServerConnection::listen(int connectionQueueSize) {

    ENetHost *server = enet_host_create(&m_address, connectionQueueSize,
                                  2      /* allow up to 2 channels to be used, 0 and 1 */,
                                  0      /* assume any amount of incoming bandwidth */,
                                  0      /* assume any amount of outgoing bandwidth */);
    if (server == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }

	NetworkConnection::setHost(server);
}

void ServerConnection::poll() {
	ENetEvent event;
	ENetHost *host = NetworkConnection::getHost();
    
    // Wait up to x milliseconds for an event.

	while (enet_host_service(host, &event, 5) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
		{
            printf("A new client connected from %x:%u.\n", 
                    event.peer->address.host,
                    event.peer->address.port);

            // Store any relevant client information here.
			int id = m_connections.size();
            event.peer->data = (char*)id;

			NetworkConnection *connection = new NetworkConnection(NetworkConnection::getHost(), event.peer);
			m_connections.insert(pair<int, NetworkConnection*>(id, connection));
			m_looseConnections.push(connection);
			
            break;
		}

        case ENET_EVENT_TYPE_RECEIVE:
		{
            printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);
			
			MsgHeader header;
			memcpy(&header, event.packet->data, MsgHeader::headerSize);
			if (event.packet->dataLength >= MsgHeader::headerSize + header.messageSize) {
				RawMessage rawMsg;
				rawMsg.type = header.messageType;
				rawMsg.size = header.messageSize;
				rawMsg.data = new uint8[header.messageSize];
				//socket->skip(MsgHeader::headerSize); //???
				if (header.messageSize) {
					//socket->receive(rawMsg.data, header.messageSize);
				} else {
					rawMsg.data = 0;
				}
	
				m_connections[(int)event.peer->data]->pushMessage(rawMsg);
			} else {
				return; //does this make sense here??
			}

            enet_packet_destroy(event.packet);
			
            
            break;
		}
           
        case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconected.\n", event.peer->data);

            // Reset the peer's client information.
            event.peer->data = NULL;

			//delete networkConnection;

			///@todo handle disconnects
		case ENET_EVENT_TYPE_NONE:
			break;
        }
    }
}

void ClientConnection::poll() {
	ENetEvent event;
	ENetHost *host = NetworkConnection::getHost();

	while (enet_host_service(host, &event, 50) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
		{
			char hostname[256] = "(error)";
			enet_address_get_host_ip(&event.peer->address, hostname, strlen(hostname));

			m_server = new NetworkConnection(host, event.peer);

			break;
		}

		case ENET_EVENT_TYPE_DISCONNECT:
		{
			delete m_server;
			m_server = NULL;
			break;
		}

		case ENET_EVENT_TYPE_RECEIVE:
		{
			MsgHeader header;
			memcpy(&header, event.packet->data, MsgHeader::headerSize);
			if (event.packet->dataLength >= MsgHeader::headerSize + header.messageSize) {
				RawMessage rawMsg;
				rawMsg.type = header.messageType;
				rawMsg.size = header.messageSize;
				rawMsg.data = new uint8[header.messageSize];
				//socket->skip(MsgHeader::headerSize); //???
				if (header.messageSize) {
					//socket->receive(rawMsg.data, header.messageSize);
				} else {
					rawMsg.data = 0;
				}
	
				pushMessage(rawMsg);
			} else {
				return; //does this make sense here??
			}

            enet_packet_destroy(event.packet);

			break;
		}

		case ENET_EVENT_TYPE_NONE:
			break;
		}
	}
}

// Public
void ClientConnection::connect(const string &address, int port) {
	ENetHost *client = enet_host_create(NULL /* create a client host */,
                1 /* only allow 1 outgoing connection */,
                2 /* allow up 2 channels to be used, 0 and 1 */,
                57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
                14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

    if (client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
        exit(EXIT_FAILURE);
    }

	NetworkConnection::setHost(client);

	///// connect

	ENetAddress eNetAddress;
    ENetEvent event;

	enet_address_set_host(&eNetAddress, address.c_str());
    eNetAddress.port = port;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    ENetPeer *peer = enet_host_connect(client, &eNetAddress, 2, 0);
    
    if (peer == NULL)
    {
       fprintf(stderr, "No available peers for initiating an ENet connection.\n");
       exit(EXIT_FAILURE);
    }
    
#ifdef false
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (client, &event, 10000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
		LOG_NETWORK("Connection to localhost succeeded.");
		m_server = new NetworkConnection(client, peer);
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);

        LOG_NETWORK("Connection to localhost failed.");
    }
#endif
}

}} // end namespace
