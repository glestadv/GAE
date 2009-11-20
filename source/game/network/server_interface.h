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
#if 0
#ifndef _GAME_NET_SERVERINTERFACE_H_
#define _GAME_NET_SERVERINTERFACE_H_

#include <vector>
#include <map>

//#include "game_constants.h"
#include "network_messenger.h"
#include "remote_interface.h"
#include "socket.h"

using std::vector;
using std::map;
using Shared::Platform::ServerSocket;
using namespace Game::Net;

namespace Game { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================

class ServerInterface : public NetworkMessenger {
private:
	enum UnitUpdateType {
		UUT_NEW,
		UUT_MORPH,
		UUT_FULL_UPDATE,
		UUT_PARTIAL_UPDATE
	};
	typedef map<Unit *, UnitUpdateType> UnitUpdateMap;

	UnitUpdateMap updateMap;
	bool updateFactionsFlag;
	bool resumingSaved;

public:
	ServerInterface(unsigned short port);
	virtual ~ServerInterface();

	//message processing
//	virtual void update();
//	virtual void updateLobby(){};
//	virtual void updateKeyframe(int frameCount);
//	virtual void waitUntilReady(Checksums &checksums);
	virtual void accept();
	Logger &getLogger() {return Logger::getServerLog();}
//	void updateGameStatus(
//	virtual bool isConnected();

//	void launchGame(const GameSettings &gameSettings, const string savedGameFile = "");
	void launchGame();
//	void setResumingSaved(bool resumingSaved)		{this->resumingSaved = resumingSaved;}
	void setResumeSavedGame(const string &savedGameFile) {
		this->resumingSaved = true;
		setSavedGameFileName(savedGameFile);
	}
	void clearResumeSavedGame() {
		this->resumingSaved = false;
		setSavedGameFileName("");
	}
	void sendSavedGameFile() {
		string fn = getSavedGameFileName();
		if(fn != "") {
			sendFile(fn, "resumed_network_game.sav", true);
		}
	}

	// message sending
//	virtual void sendTextMessage(const string &text, int teamIndex);

	//misc
	virtual string getStatus() const;
	virtual void requestCommand(Command *command);
	virtual void beginUpdate(int frame, bool isKeyFrame);
	virtual void endUpdate();
/*	bool isConnected() {
		for(PeerVector::const_iterator i = peerVector.begin(); i != peerVector.end(); ++i) {
			if((*i)->isConnected()) {
				return true;
			}
		}
		return false;
	}*/

	RemoteClientInterface *getClient(int i) {return static_cast<RemoteClientInterface *>(getPeer(i));}
	RemoteClientInterface *findUnslottedClient();
	RemoteClientInterface *findClientForMapSlot(int mapSlot);
	void unslotAllClients();

//	void launchGame(const GameSettings* gameSettings, const string savedGameFile = "");
	//void sendFile(const string path, const string remoteName, bool compress);

	// unit update requests
	void updateFactions()				{updateFactionsFlag = true;}
	void newUnit(Unit *unit)			{addUnitUpdate(unit, UUT_NEW);}
	void unitMorph(Unit *unit)			{addUnitUpdate(unit, UUT_MORPH);}
	void unitUpdate(Unit *unit)			{addUnitUpdate(unit, UUT_FULL_UPDATE);}
	void minorUnitUpdate(Unit *unit)	{addUnitUpdate(unit, UUT_PARTIAL_UPDATE);}
	void sendUpdates();

	virtual void print(ObjectPrinter &op) const;

protected:
	virtual void ping() {}
	void update();

private:
	void _onReceive(RemoteInterface &source, NetworkMessageHandshake &msg);
	void _onReceive(RemoteInterface &source, NetworkMessagePlayerInfo &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageGameInfo &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageStatus &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageText &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageFileHeader &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageFileFragment &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageReady &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageCommandList &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageUpdate &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageUpdateRequest &msg);
	void _onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg);

	void addUnitUpdate(Unit *unit, UnitUpdateType type);
	//void broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot = -1);
	void updateListen();
	void broadcastGameInfo();
};

}} // end namespace

#endif
#endif