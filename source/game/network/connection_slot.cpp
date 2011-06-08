// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "connection_slot.h"

#include <stdexcept>

#include "conversion.h"
#include "game_util.h"
#include "config.h"
#include "server_interface.h"
#include "network_message.h"

#include "leak_dumper.h"
#include "logger.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"

using namespace Glest::Sim;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class ConnectionSlot
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex)
		: m_serverInterface(serverInterface)
		, m_playerIndex(playerIndex)
		, m_connection(NULL)
		, m_ready(false) {
}

ConnectionSlot::~ConnectionSlot() {
	delete m_connection;
}

void ConnectionSlot::sendIntroMessage() {
	assert(m_connection);
	NETWORK_LOG( "Connection established, slot " << m_playerIndex << " sending intro message." );
	m_connection->setBlock(false);
	m_connection->setNoDelay();
	IntroMessage networkMessageIntro(getNetworkVersionString(), 
		g_config.getNetPlayerName(), m_connection->getHostName(), m_playerIndex);
	m_connection->send(&networkMessageIntro);
}

bool ConnectionSlot::isConnectionReady() {
	if (!m_connection) {
		m_connection = m_serverInterface->accept();

		// send intro message when connected
		if (m_connection) {
			sendIntroMessage();
		}
		return false;
	}
	if (!m_connection->isConnected()) {
		NETWORK_LOG( "Slot " << m_playerIndex << " disconnected, [" << m_connection->getRemotePlayerName() << "]" );
		throw Disconnect();
	}
	return true;
}

void ConnectionSlot::processMessages() {
	try {
		m_connection->receiveMessages();
	} catch (SocketException &e) {
		NETWORK_LOG( "Slot " << m_playerIndex << " [" << m_connection->getRemotePlayerName() << "]" << " : " << e.what() );
		string msg = m_connection->getRemotePlayerName() + " [" + m_connection->getRemoteHostName() + "] has disconnected.";
		m_serverInterface->sendTextMessage(msg, -1);
		throw Disconnect();
	}
	while (m_connection->hasMessage()) {
		MessageType type = m_connection->peekNextMsg();
		if (type == MessageType::DATA_SYNC) {
			if (!m_serverInterface->syncReady()) {
				return;
			}
		} else if (type == MessageType::READY && !m_serverInterface->syncReady()) {
			return;
		}
		RawMessage raw = m_connection->getNextMessage();
		if (raw.type == MessageType::TEXT) {
			NETWORK_LOG( "Received text message on slot " << m_playerIndex );
			TextMessage textMsg(raw);
			m_serverInterface->process(textMsg, m_playerIndex);
		} else if (raw.type == MessageType::INTRO) {
			IntroMessage msg(raw);
			NETWORK_LOG( "Received intro message on slot " << m_playerIndex << ", host name = "
				<< msg.getHostName() << ", player name = " << msg.getPlayerName() );
			m_connection->setRemoteNames(msg.getHostName(), msg.getPlayerName());
		} else if (raw.type == MessageType::COMMAND_LIST) {
			NETWORK_LOG( "Received command list message on slot " << m_playerIndex );
			CommandListMessage cmdList(raw);
			for (int i=0; i < cmdList.getCommandCount(); ++i) {
				m_serverInterface->requestCommand(cmdList.getCommand(i));
			}
		} else if (raw.type == MessageType::QUIT) {
			QuitMessage quitMsg(raw);
			NETWORK_LOG( "Received quit message on slot " << m_playerIndex );
			string msg = m_connection->getRemotePlayerName() + " [" + m_connection->getRemoteHostName() + "] has quit the game!";
			m_serverInterface->sendTextMessage(msg, -1);
			throw Disconnect();
#		if MAD_SYNC_CHECKING
		} else if (raw.type == MessageType::SYNC_ERROR) {
			SyncErrorMsg e(raw);
			int frame = e.getFrame();
			serverInterface->dumpFrame(frame);
			throw GameSyncError("Client detected sync error");
#		endif
		} else if (raw.type == MessageType::DATA_SYNC) {
			DataSyncMessage msg(raw);
			m_serverInterface->dataSync(m_playerIndex, msg);
		} else {
			NETWORK_LOG( "Unexpected message type: " << raw.type << " on slot: " << m_playerIndex );
			stringstream ss;
			ss << "Player " << m_playerIndex << " [" << getName()
				<< "] was disconnected because they sent the server bad data.";
			m_serverInterface->sendTextMessage(ss.str(), -1);
			throw InvalidMessage((int8)raw.type);
		}
	}
}

void ConnectionSlot::update() {
	if (isConnectionReady()) {
		processMessages();
	}
}

bool ConnectionSlot::hasReadyMessage() {
	// only check for a ready message once
	if (!isReady()) {
		m_connection->receiveMessages();
		if (!m_connection->hasMessage()) {
			return false;
		}
		RawMessage raw = m_connection->getNextMessage();
		if (raw.type != MessageType::READY) {
			throw InvalidMessage(MessageType::READY, raw.type);
		}
		NETWORK_LOG( __FUNCTION__ << " Received ready message, slot " << m_playerIndex );
		setReady();
	}

	return true;
}

void ConnectionSlot::logLastMessage() {
	NETWORK_LOG( "\tSlot[" << m_playerIndex << "]");
	MsgHeader hdr;
	if (m_connection->receive(&hdr, hdr.headerSize)) {
		NETWORK_LOG( "\tMessage type: " << MessageTypeNames[MessageType(hdr.messageType)]
			<< ", message size: " << hdr.messageSize );
	}
}

void ConnectionSlot::send(const Message* networkMessage) {
	///@todo this might be too slow to check each time, so have another version that doesn't check.
	if (m_connection->isConnected()) {
		m_connection->send(networkMessage);
	} else {
		Lang &lang = Lang::getInstance();
		string errmsg = m_connection->getDescription() + " (" + lang.get("Player") + " "
			+ intToStr(m_playerIndex + 1) + ") " + lang.get("Disconnected");
		LOG_NETWORK(errmsg);

		throw Disconnect(errmsg);
	}
}

// =====================================================
//	class DedicatedConnectionSlot
// =====================================================

DedicatedConnectionSlot::DedicatedConnectionSlot(ServerConnection *dedicatedServer, NetworkConnection *server, int playerIndex)
		: ConnectionSlot(NULL, playerIndex) // not using ServerInterface yet
		, m_dedicatedServer(dedicatedServer)
		, m_server(server) {
}

void DedicatedConnectionSlot::processMessages() {
	// get messages from server and send to client
	try {
		m_connectionToServer->receiveMessages();
	} catch (SocketException &e) {
		//NETWORK_LOG( "Slot " << m_playerIndex << " [" << getRemotePlayerName() << "]" << " : " << e.what() );
		//string msg = getRemotePlayerName() + " [" + getRemoteHostName() + "] has disconnected.";
		//m_serverInterface->sendTextMessage(msg, -1);
		throw Disconnect();
	}
	cout << "===== From Server to Client =====" << endl;
	while (m_connectionToServer->hasMessage()) {
		RawMessage raw = m_connectionToServer->getNextMessage();
		cout << "Player: " << getPlayerIndex() << " Message Type: " << raw.type << " Size: " << raw.size << endl;
		m_connection->send(raw.data, raw.size);
	}

	// get messages from client and send to server
	try {
		m_connection->receiveMessages();
	} catch (SocketException &e) {
		NETWORK_LOG( "Slot " << getPlayerIndex() << " [" << m_connection->getRemotePlayerName() << "]" << " : " << e.what() );
		string msg = m_connection->getRemotePlayerName() + " [" + m_connection->getRemoteHostName() + "] has disconnected.";
		//m_serverInterface->sendTextMessage(msg, -1);
		throw Disconnect();
	}
	while (m_connection->hasMessage()) {
		RawMessage raw = m_connection->getNextMessage();
		m_connectionToServer->send(raw.data, raw.size);
	}
}

bool DedicatedConnectionSlot::isConnectionReady() {
	if (!m_connection) {
		m_connection = m_dedicatedServer->accept();
		if (!m_connection) {
			return false;
		}
		cout << "Connection received" << endl;
		if (m_server) {
			///@todo work out how to get a port in there without config
			m_connectionToServer->connect(m_server->getIp(), 61357);
		} else {
			//m_connectionToServer->connect(m_connection->getIp(), 61357);
		}
		// note: no need to send intro message since it will come from server
	}
	if (!m_connection->isConnected() /*|| !m_connectionToServer->isConnected()*/) {
		NETWORK_LOG( "Slot " << getPlayerIndex() << " disconnected, [" << m_connection->getRemotePlayerName() << "]" );
		throw Disconnect();
	}
	return true;
}

}}//end namespace
