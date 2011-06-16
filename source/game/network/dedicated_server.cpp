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
#include "dedicated_server.h"

/*
#include "server_interface.h"
#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "world.h"
#include "game.h"
#include "network_types.h"
#include "sim_interface.h"
*/

#include "network_connection.h"

#include "leak_dumper.h"
#include "logger.h"
#include "profiler.h"

//#include "type_factories.h"

using namespace Shared::Platform;
using namespace Shared::Util;

using Glest::Sim::SimulationInterface;

namespace Glest { namespace Net {

// =====================================================
//	class DedicatedServer
// =====================================================

DedicatedServer::DedicatedServer(/*Program &prog*/)
		: /*ServerInterface(prog)*/
		m_portBound(false)
		, m_toServer(0) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		m_slots[i] = 0;
	}
}

DedicatedServer::~DedicatedServer() {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		delete m_slots[i];
	}
}

void DedicatedServer::bindPort() {
	int port = 61358;
	try {
		m_connection.setBlock(false);
		m_connection.bind(/*g_config.getNetServerPort() + 1*/port); ///@todo config doesn't exist for dedicated_server
	} catch (runtime_error &e) {
		LOG_NETWORK(e.what());
		throw e;
	}
	m_portBound = true;
	cout << "Port bound to " << port << endl;
}

bool DedicatedServer::init() {
	bindPort();
	m_connection.listen(GameConstants::maxPlayers);
	return true;
}

void DedicatedServer::loop() {
	while (true) {
		if (m_portBound) {
			update();
		}
		sleep(100);
	}
}

void DedicatedServer::update() {
	if (!m_toServer) {
		m_toServer = m_connection.accept();
		if (m_toServer) {
			cout << "The server connected, sending intro message" << endl;
			sendIntroMessage();
			// get how many slots should be created from server
			addSlot(1);
		}
		return;
	}

	//updateServerConnection(); //get gamesettings message to add/remove slots

	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (m_slots[i]) {
			m_slots[i]->update();
		}
	}
}

void DedicatedServer::sendIntroMessage() {
	assert(m_toServer);
	m_toServer->setBlock(false);
	m_toServer->setNoDelay();
	IntroMessage networkMessageIntro(getNetworkVersionString(), 
		"DedicatedServer", m_connection.getHostName(), 0);
	m_toServer->send(&networkMessageIntro);
}

void DedicatedServer::addSlot(int playerIndex) {
	assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
	if (!m_portBound) {
		bindPort();
	}
	NETWORK_LOG( __FUNCTION__ << " Opening slot " << playerIndex );
	delete m_slots[playerIndex];
	// the first connection is treated as server
	//if (playerIndex == 0) {
	//	m_slots[playerIndex] = new DedicatedConnectionSlot(&m_connection, NULL, playerIndex);
	//} else {
		m_slots[playerIndex] = new DedicatedConnectionSlot(&m_connection, m_toServer, playerIndex);
	//}
	//updateListen();
}

/*
IF_MAD_SYNC_CHECKS(
	void DedicatedServer::dumpFrame(int frame) {
		if (frame > 5) {
			stringstream ss;
			for (int i = 5; i >= 0; --i) {
				worldLog->logFrame(ss, frame - i);
			}
			NETWORK_LOG( ss.str() );
		}
	}
)
*/

}}//end namespace
