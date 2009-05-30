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

#ifndef _GAME_NET_NETWORKMANAGER_H_
#define _GAME_NET_NETWORKMANAGER_H_

#include <cassert>

//#include "socket.h"
//#include "checksum.h"
#include "server_interface.h"
#include "client_interface.h"
#include "logger.h"

//using Shared::Util::Checksum;
using namespace Game::Net;

namespace Game { namespace Net {

// =====================================================
//	class NetworkManager
// =====================================================

class NetworkManager {
private:
	GameInterface* gameInterface;
	NetworkRole networkRole;

public:
	static NetworkManager &getInstance() {
		static NetworkManager networkManager;
		return networkManager;
	}

	NetworkManager();
	~NetworkManager();
	void init(NetworkRole networkRole);
	void end();

	void beginUpdate(int frame, bool isKeyFrame) {
		if(gameInterface) {
			gameInterface->beginUpdate(frame, isKeyFrame);
		}
	}

	void endUpdate() {
		if(gameInterface) {
			gameInterface->endUpdate();
		}
	}

	bool isServer()			{return networkRole == NR_SERVER;}
	bool isNetworkServer()	{return networkRole == NR_SERVER && getServerInterface()->getConnectionCount() > 0;}
	bool isNetworkClient()	{return networkRole == NR_CLIENT;}
	bool isNetworkGame()	{return isNetworkClient() || isNetworkServer();}	
	bool isLocal()			{return !isNetworkGame();}

	Logger &getLogger() {
		assert(isNetworkGame());
		return networkRole == NR_CLIENT ? Logger::getClientLog() : Logger::getServerLog();
	}

	GameInterface* getGameInterface() {
		assert(gameInterface);
		return gameInterface;
	}

	ServerInterface* getServerInterface() {
		assert(gameInterface);
		assert(networkRole == NR_SERVER);
		return static_cast<ServerInterface*>(gameInterface);
	}

	ClientInterface* getClientInterface() {
		assert(gameInterface);
		assert(networkRole == NR_CLIENT);
		return static_cast<ClientInterface*>(gameInterface);
	}
};

}} // end namespace

#endif
