// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_NETWORKMANAGER_H_
#define _GLEST_GAME_NETWORKMANAGER_H_

#include <cassert>

#include "socket.h"
#include "checksum.h"
#include "server_interface.h"
#include "client_interface.h"

using Shared::Util::Checksum;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkManager
// =====================================================

class NetworkManager{
private:
	GameNetworkInterface* gameNetworkInterface;
	NetworkRole networkRole;

public:
	static NetworkManager &getInstance();

	NetworkManager();
	~NetworkManager();
	void init(NetworkRole networkRole);
	void end();

	void update() {
		if(gameNetworkInterface) {
			gameNetworkInterface->update();
		}
	}

	bool isLocal() {
		return !isNetworkGame();
	}

	bool isServer() {
		return networkRole == NR_SERVER;
	}

	bool isNetworkServer() {
		return networkRole == NR_SERVER && getServerInterface()->getConnectedSlotCount() > 0;
	}

	bool isNetworkClient() {
		return networkRole == NR_CLIENT;
	}

	bool isNetworkGame() {
		return networkRole == NR_CLIENT || getServerInterface()->getConnectedSlotCount() > 0;
	}

	GameNetworkInterface* getGameNetworkInterface() {
		assert(gameNetworkInterface != NULL);
		return gameNetworkInterface;
	}

	ServerInterface* getServerInterface() {
		assert(gameNetworkInterface != NULL);
		assert(networkRole == NR_SERVER);
		return static_cast<ServerInterface*>(gameNetworkInterface);
	}

	ClientInterface* getClientInterface() {
		assert(gameNetworkInterface != NULL);
		assert(networkRole == NR_CLIENT);
		return static_cast<ClientInterface*>(gameNetworkInterface);
	}
};

}}//end namespace

#endif
