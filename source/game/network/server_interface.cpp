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
#include "server_interface.h"

#include <cassert>
#include <stdexcept>
#include <fstream>
#include <zlib.h>

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "client_interface.h"
#include "world.h"
#include "game.h"
#include "protocol_exception.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Game { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(unsigned short port)
		: GameInterface(NR_SERVER, port, 0)
		, updateFactionsFlag(false) {
	getLogger().clear();
}

ServerInterface::~ServerInterface() {
}

/**
 * Accept a new connection.  This function should only be called by the GameInterface::execute()
 * function.
 */
void ServerInterface::accept() {
	ClientSocket *s = getServerSocket().accept();

	if(s) {
		MutexLock lock(mutex);
		if(!getPeers().size() >= GameConstants::maxPlayers) {
			// TODO: We should send a "server full" message.
			s->close();
			delete s;
			return;
		}
		int nextId = getNextPeerId();
		RemoteClientInterface *peer = new RemoteClientInterface(*this, s, nextId);
		peer->setState(STATE_CONNECTED);
		addPeer(peer);
		NetworkMessageHandshake msg(*peer);
		peer->send(msg, true);
		updateListen();
	}
}

/**
 * Find a player who is slotted as a spectator.  This doesn't verify they are truely unslotted by
 * looking at the GameSettings object.
 */
RemoteClientInterface *ServerInterface::findUnslottedClient() {
	foreach(const PeerMap::value_type &p, getPeers()) {
		if(p.second->getPlayer().isSpectator()) {
			return static_cast<RemoteClientInterface *>(p.second);
		}
	}
	return NULL;
}

/**
 * Find the human player who is not the local player that controls the given map slot.
 */
RemoteClientInterface *ServerInterface::findClientForMapSlot(int mapSlot) {
	const GameSettings::Faction *f = getGameSettings()->findFactionForMapSlot(mapSlot);
	if(f) {
		foreach(const Player *p, f->getPlayers()) {
			if(p->getType() == PLAYER_TYPE_HUMAN) {
				return getClient(p->getId());
			}
		}
	}
	return NULL;
}

void ServerInterface::unslotAllClients() {
	foreach(const PeerMap::value_type &pair, getPeers()) {
		RemoteInterface &client = *pair.second;
		getGameSettings()->removePlayer(client.getPlayer());
		client.getProtectedPlayer().setSpectator(true);
	}
	setGameSettingsChanged(true);
	setConnectionsChanged();
}

void ServerInterface::requestCommand(Command *command) {
	MutexLock localLock(mutex);

	if(command->isAuto()) {
		minorUnitUpdate(command->getCommandedUnit());
	}
	copyCommandToNetwork(command);
	futureCommands[getLastFrame() + getGameSettings()->getCommandDelay()].push(command);
}

/*
void ServerInterface::process(NetworkMessageText &msg, int requestor) {
	broadcastMessage(&msg, requestor);
	chatText = msg.getText();
	chatSender = msg.getSender();
	chatTeamIndex = msg.getTeamIndex();
}
*/

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageHandshake &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessagePlayerInfo &msg) {
	MutexLock localLock(mutex);

	setConnectionsChanged();
//	getGameSettings()->addSpectator(source.getPlayer());
	getGameSettings()->updatePlayer(source.getPlayer());
	broadcastGameInfo();

	getLogger().add("Accepted new connection from:", false);
	getLogger().add(static_cast<const Printable &>(source.getPlayer()), false);
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageGameInfo &msg) {
	throw ProtocolException(source, &msg, "ServerInterface doesn't accept NetworkMessageGameInfo.",
			NULL, __FILE__, __LINE__);
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageStatus &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageText &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageFileHeader &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageFileFragment &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageReady &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageCommandList &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageUpdate &msg) {
	throw ProtocolException(source, &msg, "ServerInterface doesn't accept NetworkMessageUpdate.",
			NULL, __FILE__, __LINE__);
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg) {
}

void ServerInterface::_onReceive(RemoteInterface &source, NetworkMessageUpdateRequest &msg) {
	NetworkWriteableXmlDoc &doc = msg.getDoc();
	doc.parse();
	XmlNode &rootNode = doc.getRootNode();

	for(int i = 0; i < rootNode.getChildCount(); ++i) {
		XmlNode *unitNode = rootNode.getChild("unit", i);
		Unit *unit = UnitReference(unitNode).getUnit();

		if(!unit) {
			throw ProtocolException(source, &msg, "Client out of sync",
					NULL, __FILE__, __LINE__);
		}

		if(unitNode->getAttribute("full")->getBoolValue()) {
			unitUpdate(unit);
		} else {
			minorUnitUpdate(unit);
		}
	}
}

void ServerInterface::beginUpdate(int frame, bool isKeyFrame) {
	GameInterface::beginUpdate(frame, isKeyFrame);
}

void ServerInterface::endUpdate() {
	MutexLock lock(mutex);
	for(CommandQueue::iterator i = requestedCommands.begin(); i != requestedCommands.end(); ++i) {
		// this shouldn't actually happen, but if it does, we'll just erase it, invalidating the
		// iterator and recurse
		if(i->second.empty()) {
			requestedCommands.erase(i);
			//i = requestedCommands.begin();
			//--i;
			//continue;
			endUpdate();
			return;
		}
		NetworkMessageCommandList networkMessageCommandList(*this);
		Commands &outgoing = i->second;
		while(!outgoing.empty() && !networkMessageCommandList.isFull()) {
			networkMessageCommandList.addCommand(outgoing.front());
			outgoing.pop();
		}
		broadcastMessage(networkMessageCommandList);
	}
/*
	//build command list, remove commands from requested and add to pending
	for (int i = 0; i < requestedCommands.size(); i++) {
		Command *cmd = requestedCommands[i];
		if (!networkMessageCommandList.isFull()) {
			// Clients are feeding themselves non-auto commands now.
			if (!requestedCommands[i]->isAuto()
					&& !cmd->getCommandedUnit()->getFaction()->isThisFaction()) {
				networkMessageCommandList.addCommand(new Command(*cmd));
			}
			pendingCommands.push_back(cmd);
		} else {
			break;
		}
	}
	requestedCommands.clear();

	//broadcast commands
	broadcastMessage(&networkMessageCommandList);
*/
}

string ServerInterface::getStatus() const {
	stringstream str;
	foreach(const PeerMap::value_type &v, getPeers()) {
		str << endl << v.second->getStatus();
	}
	return str.str();
}

void ServerInterface::print(ObjectPrinter &op) const {
	GameInterface::print(op.beginClass("ServerInterface"));
	op		.endClass();
}

void ServerInterface::update() {
	MutexLock localLock(mutex);

	if(isGameSettingsChanged()) {
		broadcastGameInfo();
		setGameSettingsChanged(false);
	}
}

void ServerInterface::broadcastGameInfo() {
	MutexLock localLock(mutex);

	NetworkMessageGameInfo msg1(*this, *getGameSettings());

	foreach(const PeerMap::value_type &p, getPeers()) {
		if(p.second->getState() >= STATE_INITIALIZED) {
			p.second->send(msg1, true);
		}
	}
}

#if 0
void ServerInterface::waitUntilReady(Checksums &checksums) {

	Chrono chrono;
	bool allReady= false;

	chrono.start();

	//wait until we get a ready message from all clients
	while(!allReady) {

		allReady = true;
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			RemoteClientInterface *client = getClient(i);
			if(client) {
				client->flush();
			}
			if(client && !client->isReady()){
				NetworkMessage *msg = client->peek();
				if(msg && msg->getType() == NMT_READY){
					client->setReady();
					client->pop();
					delete msg;
				} else {
					allReady = false;
				}
			}
		}

		//check for timeout
		if(chrono.getMillis() > READY_WAIT_TIMEOUT){
			throw runtime_error("Timeout waiting for clients");
		}
		sleep(10);
	}

	//send ready message after, so clients start delayed
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		NetworkMessageReady networkMessageReady(checksums);
		RemoteClientInterface *client = getClient(i);

		if(client) {
			client->send(&networkMessageReady);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex);
	broadcastMessage(&networkMessageText);
}

#endif
void ServerInterface::launchGame(/*const GameSettings &gs, const string savedGameFile*/) {

	setState(STATE_LAUNCHING);
	NetworkMessageGameInfo msg1(*this, *getGameSettings());
	broadcastMessage(msg1);
}

/**
 * Record the unit to be updated using the highest update level specified.
 */
void ServerInterface::addUnitUpdate(Unit *unit, UnitUpdateType type) {
	UnitUpdateMap::iterator i = updateMap.find(unit);
	if(i != updateMap.end()) {
		if(i->second > type) {
			i->second = type;
		}
	} else {
		updateMap[unit] = type;
	}
}

/** Send all pending updates to clients. */
void ServerInterface::sendUpdates() {
	World *world = World::getCurrWorld();
	int64 now = Chrono::getCurMicros();
	NetworkMessageUpdate msg;
	if(updateFactionsFlag) {
		for(int i = 0; i < world->getFactionCount(); ++i) {
			msg.updateFaction(world->getFaction(i));
		}
	}

	for(UnitUpdateMap::iterator i = updateMap.begin(); i != updateMap.end(); ++i) {
		switch(i->second) {
		case UUT_NEW:
			msg.newUnit(i->first);
			break;
		case UUT_MORPH:
			msg.unitMorph(i->first);
			break;
		case UUT_FULL_UPDATE:
			msg.unitUpdate(i->first);
			break;
		case UUT_PARTIAL_UPDATE:
			msg.minorUnitUpdate(i->first);
			break;
		}
		if(i->second != UUT_PARTIAL_UPDATE) {
			i->first->setLastUpdated(now);
		}
	}

	if(msg.hasUpdates()) {
		NetworkWriteableXmlDoc &doc = msg.getDoc();
		doc.writeXml();
		doc.compress();
		broadcastMessage(msg);
		updateFactionsFlag = false;
	}
	updateMap.clear();
}

/*
void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i){
		//ConnectionSlot* slot = slots[i];
		RemoteClientInterface *client = getClient(i);

		if(i !=  excludeSlot && client) {
			if(client->isConnected()) {
				client->send(networkMessage);
			} else {
				Lang &lang = Lang::getInstance();
				string errmsg = client->getDescription() + " (" + lang.get("Player") + " "
						+ intToStr(client->getId() + 1) + ") " + lang.get("Disconnected");
				Game::getInstance()->autoSaveAndPrompt(errmsg, slot->getDescription(), i);
				remoteClient(i);
				//throw SocketException(errmsg);
			}
		}
	}
}
*/
//void onReceive() {
	//TODO: rebroadcast text messages and commands
//}


/** Updates the max connections the server socket is willing to accept */
void ServerInterface::updateListen() {
	int openSlotCount = 0;
	//FIXME
/*
	for(int i = 0; i < GameConstants::maxPlayers; ++i){
		if(peers[i] != NULL && !peers[i]->isConnected()) {
			++openSlotCount;
		}
	}

	getServerSocket().listen(openSlotCount);
	*/
}

}} // end namespace
