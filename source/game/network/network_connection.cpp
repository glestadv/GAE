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

Ip::Ip() {
	asUInt = 0;
}

Ip::Ip(uint32 val) {
	asUInt = val;
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

void NetworkConnection::update() {
	if (m_needFlush) {
		enet_host_flush(m_host);
		m_needFlush = false;
	}
	poll();
}

void NetworkConnection::send(const Message* msg) {
	if (m_peer) {
		msg->log();

		ENetPacket *packet = enet_packet_create(msg->getData(), msg->getSize(), ENET_PACKET_FLAG_RELIABLE);

		if (enet_peer_send(m_peer, 0, packet) != 0) {
			NETWORK_LOG( "NetworkConnection::send(): Error trying to send " << MessageTypeNames[msg->getType()] << " message, connection severed." );
			throw Disconnect();
		}
		//NETWORK_LOG( "NetworkConnection::send(): Sent " << MessageTypeNames[msg->getType()] << " message, size = " << msg->getSize() << "." );
		m_needFlush = true;
		//enet_host_flush(m_host);

	} else {
		NETWORK_LOG( "NetworkConnection::send(): Error trying to send " << MessageTypeNames[msg->getType()] << " message, m_peer is null." );
	}
}
#ifdef false
void NetworkConnection::send(const void* data, int dataSize) {
	ENetPacket *packet = enet_packet_create(data, dataSize, ENET_PACKET_FLAG_RELIABLE);
    
    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    if (enet_peer_send(m_peer, 0, packet) != 0) {
		LOG_NETWORK("connection severed, trying to send message..");
		throw Disconnect();
	}

	enet_host_flush(m_host);
}
#endif

//void NetworkConnection::receiveMessages() {
//	poll();
//}

RawMessage NetworkConnection::getNextMessage() {
	assert(hasMessage());
	RawMessage res = messageQueue.front();
	messageQueue.pop_front();
	return res;
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
				m_peer = 0;
				return;
			}
		}
	    
		/* We've arrived here, so the disconnect attempt didn't */
		/* succeed yet.  Force the connection down.             */
		enet_peer_reset(m_peer);
		
		m_peer = 0;
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
	while (true)
    {	
		int status = enet_host_service(host, &event, 5);
		if (status < 0) {
			NETWORK_LOG( "Enet Server errored." );
			break;
		}

		if (status == 0) {
			break; // no events to process
		}

        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
		{
			Ip ip(event.peer->address.host);
			NETWORK_LOG( "A new client connected from " << ip.getString() << " on port " << event.peer->address.port );

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
			//NETWORK_LOG( "A packet of length " << event.packet->dataLength << " was received from client " << int(event.peer->data)
			//	<< " on channel " << int(event.channelID) );
			
			MsgHeader header;
			memcpy(&header, event.packet->data, MsgHeader::headerSize);
			if (event.packet->dataLength >= MsgHeader::headerSize + header.messageSize) {		
				RawMessage rawMsg;
				rawMsg.type = header.messageType;
				rawMsg.size = header.messageSize;				
				if (header.messageSize) {
					rawMsg.data = new uint8[header.messageSize];
					char *buffer = ((char*)event.packet->data) + MsgHeader::headerSize;
					memcpy((char*)rawMsg.data, buffer, header.messageSize);
				} else {
					rawMsg.data = 0;
				}
				//NETWORK_LOG( "ServerConnection::poll(): received message [type=" << MessageTypeNames[rawMsg.type] << 
				//	", size=" << rawMsg.size );
				m_connections[(int)event.peer->data]->pushMessage(rawMsg);
			} else {
				NETWORK_LOG( "ServerConnection::poll(): Invalid message in queue. packet->dataLength < MsgHeader::headerSize + header.messageSize" );
				enet_packet_destroy(event.packet);
				throw GarbledMessage(MessageType(header.messageType), NetSource::CLIENT);
			}
            enet_packet_destroy(event.packet);
            break;
		}
           
        case ENET_EVENT_TYPE_DISCONNECT:
            NETWORK_LOG( "client " << int(event.peer->data) << "disconected." );

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

	if (!host) {
		return;
	}

	while (/*enet_host_service(host, &event, 5) > 0*/ true) {
		int status = enet_host_service(host, &event, 5);
		if (status < 0) {
			NETWORK_LOG( "Enet client errored." );
			break;
		}

		if (status == 0) {
			break; // no events to process
		}

		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
		{
			char hostname[256] = "(error)";
			enet_address_get_host_ip(&event.peer->address, hostname, strlen(hostname));

			// ???
			//m_server = new NetworkConnection(host, event.peer);
			setHost(host); // ?
			
			setPeer(event.peer);

			break;
		}

		case ENET_EVENT_TYPE_DISCONNECT:
		{
			setHost(0);
			setPeer(0);	
			throw Disconnect();
			//delete m_server;
			//m_server = NULL;
			break;
		}

		case ENET_EVENT_TYPE_RECEIVE:
		{
			//NETWORK_LOG( "A packet of length " << event.packet->dataLength << " was received from the server"
			//	<< " on channel " << int(event.channelID) );
			MsgHeader header;
			memcpy(&header, event.packet->data, MsgHeader::headerSize);

			if (event.packet->dataLength >= MsgHeader::headerSize + header.messageSize) {
				RawMessage rawMsg;
				rawMsg.type = header.messageType;
				rawMsg.size = header.messageSize;
				if (header.messageSize) {
					rawMsg.data = new uint8[header.messageSize];
					char *buffer = ((char*)event.packet->data) + MsgHeader::headerSize;
					memcpy((char*)rawMsg.data, buffer, header.messageSize);//event.packet->dataLength - MsgHeader::headerSize);
				} else {
					rawMsg.data = 0;
				}
				pushMessage(rawMsg);
				//NETWORK_LOG( "ClientConnection::poll(): received message [type=" << MessageTypeNames[rawMsg.type] << 
				//	", size=" << rawMsg.size );
			} else {
				NETWORK_LOG( "ClientConnection::poll(): Invalid message in queue. packet->dataLength < MsgHeader::headerSize + header.messageSize" );
				throw GarbledMessage(MessageType(header.messageType), NetSource::SERVER);
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