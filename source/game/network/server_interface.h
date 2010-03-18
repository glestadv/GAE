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

#ifndef _GLEST_GAME_SERVERINTERFACE_H_
#define _GLEST_GAME_SERVERINTERFACE_H_

#include <vector>

#include "game_constants.h"
#include "network_interface.h"
#include "connection_slot.h"
#include "socket.h"

using std::vector;
using Shared::Platform::ServerSocket;

namespace Glest { namespace Game {

// =====================================================
//	class ServerInterface
// =====================================================

class ServerInterface: public GameInterface{
private:
	ConnectionSlot* slots[GameConstants::maxPlayers];
	ServerSocket serverSocket;

public:
	ServerInterface();
	virtual ~ServerInterface();

	virtual Socket* getSocket()				{return &serverSocket;}
	virtual const Socket* getSocket() const	{return &serverSocket;}

#	if _RECORD_GAME_STATE_
		void dumpFrame(int frame);
#	endif

protected:
	//message processing
	virtual void update();
	virtual void updateLobby(){};
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum &checksum);
	virtual void syncAiSeeds(int aiCount, int *seeds);
	virtual void createSkillCycleTable(const TechTree *techTree);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame();

	// unit/projectile updates
	virtual void updateUnitCommand(Unit *unit, int32);
	virtual void unitBorn(Unit *unit, int32);
	virtual void updateProjectile(Unit *unit, int, int32);
	virtual void updateAnim(Unit *unit, int32);

	virtual void updateMove(Unit *unit);
	virtual void updateProjectilePath(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end);

	//misc
	virtual string getStatus() const;

public:
	ServerSocket* getServerSocket()		{return &serverSocket;}

	// ConnectionSlot management
	void addSlot(int playerIndex);
	void removeSlot(int playerIndex);
	ConnectionSlot* getSlot(int playerIndex);
	int getConnectedSlotCount();
	
	void launchGame(const GameSettings* gameSettings);
	void process(NetworkMessageText &msg, int requestor);
	
private:
	void broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot= -1);
	void updateListen();
};

}}//end namespace

#endif
