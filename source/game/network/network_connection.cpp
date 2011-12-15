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
#include "network_interface.h"
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
//	class NetworkHost
// =====================================================

NetworkHost::NetworkHost(/*int updateTimeout*/) 
	: m_host(0) {
}

NetworkHost::~NetworkHost() {
	foreach (Sessions, it, m_sessions) {
		delete (*it);
	}
	
	destroy();
}

string NetworkHost::getHostName() {
	assert(m_host);
	char hostname[256] = "(error)";
	enet_address_get_host_ip(&m_host->address, hostname, strlen(hostname));
	return string(hostname);
}

string NetworkHost::getIp() {
	assert(m_host);
	Ip ip(m_host->address.host);
	return ip.getString();
}

void NetworkHost::flush() {
	assert(m_host);
	enet_host_flush(m_host);
}

void NetworkHost::update(NetworkInterface *networkInterface) {
	assert(m_host);

	ENetEvent event;
    
	while (true)
    {	
		// Wait up to x milliseconds for an event.
		int status = enet_host_service(m_host, &event, /*m_updateTimeout*/5);
		
		// error
		if (status < 0) {
			NETWORK_LOG( "enet_host_service error" );
			break;
		}

		// no events to process
		if (status == 0) {
			break;
		}

        switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				NetworkSession *session = new NetworkSession(event.peer);
				m_sessions.push_back(session);

				assert(event.peer->data == NULL);
				event.peer->data = session;

				networkInterface->onConnect(session);
				
				break;
			}

			case ENET_EVENT_TYPE_RECEIVE: {
				NetworkSession *session = static_cast<NetworkSession*>(event.peer->data);
				if (session) {
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

						session->pushMessage(rawMsg);
					} else {
						NETWORK_LOG( "ServerConnection::poll(): Invalid message in queue. packet->dataLength < MsgHeader::headerSize + header.messageSize" );
						/*
						enet_packet_destroy(event.packet);
						throw GarbledMessage(MessageType(header.messageType), NetSource::CLIENT);
						*/
					}
				}
				enet_packet_destroy(event.packet);
				break;
			}
	           
			case ENET_EVENT_TYPE_DISCONNECT: {
				NetworkSession *session = static_cast<NetworkSession*>(event.peer->data);
				if (session) {
					NETWORK_LOG( "Peer: " << session->getRemotePlayerName() << " disconected." );

					m_sessions.erase(remove(m_sessions.begin(), m_sessions.end(), session), m_sessions.end());
					session->reset();

					networkInterface->onDisconnect(session, static_cast<DisconnectReason>(event.data));
					
					delete session;
					event.peer->data = NULL;
				}
			}

			case ENET_EVENT_TYPE_NONE:
				break;
        }
    }
}

void NetworkHost::destroy() {
	if (m_host) {
		flush(); // push any final messages
		enet_host_destroy(m_host);
		m_host = NULL;
	}
}

void NetworkHost::disconnect(DisconnectReason reason) {
	foreach (Sessions, it, m_sessions) {
		(*it)->disconnect(reason);
	}

	destroy();
	
	// session is deleted at ENET_EVENT_TYPE_DISCONNECT or destructor if disconnect not received
}
	
void NetworkHost::broadcastMessage(const Message* msg) {
	assert(m_host);

	ENetPacket *packet = enet_packet_create(msg->getData(), msg->getSize(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_host, 0, packet);
}

void NetworkHost::setPeerCount(size_t count) {
	assert(m_host);
	m_host->peerCount = count;
}

void NetworkHost::removeSession(NetworkSession *session) {
	m_sessions.erase(remove(m_sessions.begin(), m_sessions.end(), session), m_sessions.end());
}

// =====================================================
//	class ClientHost
// =====================================================

void ClientHost::connect(const string &address, int port) {
	assert(NetworkHost::getSessionCount() == 0);

	ENetHost *client = enet_host_create(NULL /* create a client host */,
                1 /* only allow 1 outgoing connection */,
                2 /* allow up 2 channels to be used, 0 and 1 */,
                57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
                14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

    if (client == NULL) {
		NETWORK_LOG( "An error occurred while trying to create an ENet client host." );
        throw runtime_error("An error occurred while trying to create an ENet client host.");
    }

	NetworkHost::setHost(client);

	ENetAddress eNetAddress;
	enet_address_set_host(&eNetAddress, address.c_str());
    eNetAddress.port = port;

	connect(client, &eNetAddress);
}

void ClientHost::connect(ENetHost *client, ENetAddress *eNetAddress) {
    ENetEvent event;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    ENetPeer *peer = enet_host_connect(client, eNetAddress, 2, 0);
    
    if (peer == NULL)
    {
	   NETWORK_LOG( "No available peers for initiating an ENet connection." );
       throw runtime_error("No available peers for initiating an ENet connection.");
    }
}

bool ClientHost::isConnected() {
	return NetworkHost::getSessionCount() > 0; //might have issues if disconnects don't work
}

NetworkSession *ClientHost::getServerSession() {
	assert(isConnected());
	return NetworkHost::getSession(0);
}

void ClientHost::send(const Message* message) {
	if (isConnected()) {
		getServerSession()->send(message);
	}
}
/*
void ClientHost::setRemoteNames(const string &hostName, const string &playerName) {
	if (isConnected()) {
		getServerSession()->setRemoteNames(hostName, playerName);
	}
}
*/

// =====================================================
//	class ServerHost
// =====================================================

void ServerHost::bind(int port) {    
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    m_address.host = ENET_HOST_ANY;
    m_address.port = port;
}

void ServerHost::listen(int connectionQueueSize) {
	if (NetworkHost::isHostSet()) {
		//NetworkHost::setPeerCount(connectionQueueSize); //doesn't seem to work
	} else {
		ENetHost *server = enet_host_create(&m_address, connectionQueueSize,
									  2      /* allow up to 2 channels to be used, 0 and 1 */,
									  0      /* assume any amount of incoming bandwidth */,
									  0      /* assume any amount of outgoing bandwidth */);
		if (server == NULL) {
			NETWORK_LOG( "An error occurred while trying to create an ENet server host." );
			throw runtime_error("An error occurred while trying to create an ENet server host.");
		}

		NetworkHost::setHost(server);
	}
}

}} // end namespace