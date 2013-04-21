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
#include "network_session.h"

namespace Glest { namespace Net {

class ServerInterface;

// =====================================================
//	class ConnectionSlot
// =====================================================

class ConnectionSlot {
private:
	bool				m_ready;
	bool				m_turnDone;
	int					m_playerIndex;
	ServerInterface*	m_serverInterface;
	NetworkSession*		m_connection;

public:
	ConnectionSlot(ServerInterface* serverInterface, int playerIndex);
	virtual ~ConnectionSlot();

	virtual void update();
	
	void send(const Message* networkMessage);
	void reset();
	void logNextMessage();

	bool isReady() const			{return m_ready;}
	bool isConnected() const		{return m_connection && m_connection->isConnected();}

	void setSession(NetworkSession *v)	{m_connection = v;}
	void setReady()						{m_ready = true;}

	int getPlayerIndex() const		{return m_playerIndex;}
	string getName() const			{return m_connection ? m_connection->getRemotePlayerName() : "";}

	const NetworkSession *getSession() const {return m_connection;}

	void startNextTurn() {m_turnDone = false;}
	bool isFinishedTurn() {return m_turnDone;}

protected:
	virtual void processMessages();
};

// =====================================================
//	class DedicatedConnectionSlot
// =====================================================
/*
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
*/
}}//end namespace

#endif
