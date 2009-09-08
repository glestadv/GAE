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
#include "network_manager.h"
#include "config.h"

#include "leak_dumper.h"

using Game::Config;

namespace Game { namespace Net {

// =====================================================
// class NetworkManager
// =====================================================
/*
NetworkManager &NetworkManager::getInstance() {
	static NetworkManager networkManager;
	return networkManager;
}
*/

NetworkManager::NetworkManager() : gameInterface(NULL), networkRole(NR_IDLE) {
}

NetworkManager::~NetworkManager() {
	if (gameInterface) {
		delete gameInterface;
	}
}

void NetworkManager::init(NetworkRole networkRole) {
	Config &config = Config::getInstance();
	assert(gameInterface == NULL);

	this->networkRole = networkRole;

	if (networkRole == NR_SERVER) {
		gameInterface = new ServerInterface(config.getNetServerPort());
	} else {
		gameInterface = new ClientInterface(config.getNetClientPort());
	}
}

void NetworkManager::end() {
	if(gameInterface) {
		gameInterface->end();
		delete gameInterface;
		gameInterface = NULL;
		networkRole = NR_IDLE;
	}
}

}} // end namespace
