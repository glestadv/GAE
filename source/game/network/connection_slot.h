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

#ifndef _GLEST_GAME_CONNECTIONSLOT_H_
#define _GLEST_GAME_CONNECTIONSLOT_H_

#include "network_interface.h"
#include "network_connection.h"

namespace Glest { namespace Net {

class ServerInterface;

// =====================================================
//	class ConnectionSlot
// =====================================================

class ConnectionSlot {
private:
	ServerInterface*	m_serverInterface;
	int					m_playerIndex;
	bool				m_ready;

protected:
	NetworkConnection*	m_connection;

public:
	ConnectionSlot(ServerInterface* serverInterface, int playerIndex);
	virtual ~ConnectionSlot();

	virtual void update();

	void setReady()					{m_ready = true;}
	int getPlayerIndex() const		{return m_playerIndex;}
	bool isReady() const			{return m_ready;}
	bool hasReadyMessage();
	void logLastMessage();
	
	bool isConnected() const		{return m_connection && m_connection->isConnected();}
	string getName() const			{return m_connection ? m_connection->getRemotePlayerName() : "";}
	void send(const Message* networkMessage);
	NetworkConnection *getConnection() {return m_connection;}

protected:
	virtual void processMessages();
	virtual	bool isConnectionReady();

private:
	void sendIntroMessage();
};

// =====================================================
//	class DedicatedConnectionSlot
// =====================================================

class DedicatedConnectionSlot: public ConnectionSlot {
private:
	ClientConnection *m_connectionToServer;
	ServerConnection *m_dedicatedServer;
	NetworkConnection *m_server;

	Message *convertToMessage(RawMessage &raw);

public:
	DedicatedConnectionSlot(ServerConnection *dedicatedServer, NetworkConnection *server, int playerIndex);
	virtual ~DedicatedConnectionSlot();

protected:
	virtual void processMessages() override;
	virtual	bool isConnectionReady() override;
};

}}//end namespace

#endif
