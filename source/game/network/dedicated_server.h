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

#ifndef _GLEST_GAME_DEDICATEDSERVER_H_
#define _GLEST_GAME_DEDICATEDSERVER_H_

#include <vector>

#include "game_constants.h"
#include "network_interface.h"
#include "connection_slot.h"

using std::vector;

namespace Glest { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================
#ifdef false
/** Currently just a message router intended for network debugging.
  * The first connection to the dedicated server is treated as the server.
  * When a client connects to the dedicated server it creates
  * a new connection to the server on behalf of the client.
  * Messages are then routed through the dedicated server.
  */
class DedicatedServer /*: public ServerInterface*/ {
private:
	DedicatedConnectionSlot* m_slots[GameConstants::maxPlayers];
	ServerConnection m_connection;
	NetworkConnection *m_toServer;
	bool m_portBound;

private:
	void bindPort();
	void addSlot(int playerIndex);
	void sendIntroMessage();

public:
	DedicatedServer(/*Program &prog*/);
	virtual ~DedicatedServer();

	bool init();
	void update();
	void loop();
};
#endif

}}//end namespace

#endif
