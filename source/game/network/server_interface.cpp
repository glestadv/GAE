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
//#include <fstream>
//#include <zlib.h>

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "timer.h" //NETWORK:removed
/*
#include "client_interface.h"
*/
#include "world.h"
#include "game.h"

#include "network_types.h"

#include "leak_dumper.h"
#include "logger.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;
using Shared::Platform::Chrono; //NETWORK:removed, probably shouldn't

namespace Glest { namespace Game {

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface() {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		slots[i] = NULL;
	}
	try {
		serverSocket.setBlock(false);
		serverSocket.bind(GameConstants::serverPort);
	} catch (runtime_error e) {
		LOG_NETWORK(e.what());
		throw e;
	}
	//updateFactionsFlag = false;
}

ServerInterface::~ServerInterface(){
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		delete slots[i];
	}
}

void ServerInterface::addSlot(int playerIndex) {
	assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
	LOG_NETWORK( "Opening slot " + intToStr(playerIndex) );
	delete slots[playerIndex];
	slots[playerIndex] = new ConnectionSlot(this, playerIndex, false);
	updateListen();
}

void ServerInterface::removeSlot(int playerIndex) {
	LOG_NETWORK( "Closing slot " + intToStr(playerIndex) );
	delete slots[playerIndex];
	slots[playerIndex] = NULL;
	updateListen();
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex) {
	return slots[playerIndex];
}

int ServerInterface::getConnectedSlotCount() {
	int connectedSlotCount = 0;
	
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && slots[i]->isConnected()) {
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

void ServerInterface::update() {
	//update all slots
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i]) {
			slots[i]->update();
		}
	}
}

void ServerInterface::process(NetworkMessageText &msg, int requestor) {
	broadcastMessage(&msg, requestor);
	GameNetworkInterface::processTextMessage(msg);
}
/*
void ServerInterface::process(NetworkMessageUpdateRequest &msg) {
	msg.parse();
	XmlNode *rootNode = msg.getRootNode();

	for(int i = 0; i < rootNode->getChildCount(); ++i) {
		XmlNode *unitNode = rootNode->getChild("unit", i);
		Unit *unit = UnitReference(unitNode).getUnit();

		if(!unit) {
			throw SocketException("Client out of sync");
		}

		if(unitNode->getAttribute("full")->getBoolValue()) {
			unitUpdate(unit);
		} else {
			minorUnitUpdate(unit);
		}
	}
}
*/

void ServerInterface::updateKeyframe(int frameCount){

	NetworkMessageCommandList cmdList(frameCount);
	
	//build command list, remove commands from requested and add to pending
	while(!requestedCommands.empty()){
		if(cmdList.addCommand(&requestedCommands.back())){
			pendingCommands.push_back(requestedCommands.back());
			requestedCommands.pop_back();
		}
		else{
			break;
		}
	}

	// log it ?
	/*if (cmdList.getCommandCount()) {
		LOG_NETWORK( 
			"KeyFrame update, sending " + intToStr(cmdList.getCommandCount()) + " commands" +
			(requestedCommands.size() ? intToStr(requestedCommands.size()) + " still pending." : "." )
		);
		for (int i=0; i < cmdList.getCommandCount(); ++i) {
			const NetworkCommand * const &cmd = cmdList.getCommand(i);
			const Unit * const &unit = theWorld.findUnitById(cmd->getUnitId());
			LOG_NETWORK( 
				"\tUnit: " + intToStr(unit->getId()) + " [" + unit->getType()->getName() + "] " 
				+ unit->getType()->findCommandTypeById(cmd->getCommandTypeId())->getName() + "."
			);
		}
	}*/

	//broadcast commands
	broadcastMessage(&cmdList);
}

void ServerInterface::waitUntilReady(Checksum &checksum) {
	Chrono chrono;
	bool allReady = false;

	chrono.start();

	LOG_NETWORK( "Waiting for ready messages from all clients" );
	//wait until we get a ready message from all clients
	while(!allReady) {
		allReady = true;
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			ConnectionSlot* slot = slots[i];
			if (slot && !slot->isReady()) {
				NetworkMessageType msgType = slot->getNextMessageType();
				NetworkMessageReady readyMsg;
				if (msgType == NetworkMessageType::READY && slot->receiveMessage(&readyMsg)) {
					LOG_NETWORK( "Received ready message, slot " + intToStr(i) );
					slot->setReady();
				} else if (msgType != NetworkMessageType::NO_MSG) {
					throw runtime_error("Unexpected network message: " + intToStr(msgType));
				} else {
					allReady = false;
				}
			}
		}

		//check for timeout
		if (chrono.getMillis() > readyWaitTimeout) {
			throw runtime_error("Timeout waiting for clients");
		}
		sleep(2);
	}

	LOG_NETWORK( "Received all ready messages, sending ready message(s)." );
	
	//send ready message after, so clients start delayed
	for (int i= 0; i < GameConstants::maxPlayers; ++i) {
		NetworkMessageReady readyMsg(checksum.getSum());
		if (slots[i]) {
			slots[i]->send(&readyMsg);
		}
	}
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex);
	broadcastMessage(&networkMessageText);
}

void ServerInterface::quitGame() {
	string text = getHostName() + " has chosen to leave the game!";
	NetworkMessageText networkMessageText(text,getHostName(),-1);
	broadcastMessage(&networkMessageText, -1);
	NetworkMessageQuit networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
}

string ServerInterface::getStatus() const{
	string str;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		str += intToStr(i) + ": ";
		if (slots[i] && slots[i]->isConnected()) {
				str += slots[i]->getName(); //str += connectionSlot->getRemotePlayerName() + ": " + connectionSlot->getStatus();
		} else {
			str += theLang.get("NotConnected");
		}
		str += '\n';
	}
	return str;
}

void ServerInterface::syncAiSeeds(int aiCount, int *seeds) {
	assert(aiCount && seeds);
	LOG_NETWORK("sending " + intToStr(aiCount) + " Ai random number seeds...");
	NetworkMessageAiSeedSync seedSyncMsg(aiCount, seeds);
	broadcastMessage(&seedSyncMsg);
}

void ServerInterface::launchGame(const GameSettings* gameSettings){
	NetworkMessageLaunch networkMessageLaunch(gameSettings);
	/*
	if(savedGameFile != "") {
		sendFile(savedGameFile, "resumed_network_game.sav", true);
	}
	*/
	LOG_NETWORK( "Launching game, sending launch message(s)" );
	broadcastMessage(&networkMessageLaunch);	
}


// NETWORK: I'm thinking the file stuff should go somewhere else.
#if 0
void ServerInterface::sendFile(const string path, const string remoteName, bool compress) {
	size_t fileSize;
	ifstream in;
	int zstatus;

	fileSize = getFileSize(path);

	in.open(path.c_str(), ios_base::in | ios_base::binary);
	if(in.fail()) {
		throw runtime_error("Failed to open file " + path);
	}

	char *inbuf = new char[fileSize];
	in.read(inbuf, fileSize);
	assert(in.gcount() == fileSize && in.read((char*)&zstatus, 1).eof());
	char *outbuf;
	uLongf outbuflen;
	if(compress) {
		outbuflen = (float)fileSize * 1.001f + 12;
		outbuf = new char[outbuflen];
		zstatus = ::compress((Bytef *)outbuf, &outbuflen, (const Bytef *)inbuf, fileSize);
		if(zstatus != Z_OK) {
			throw runtime_error("Call to compress failed for some unknown fucking reason.");
		}
	} else {
		outbuf = inbuf;
		outbuflen = fileSize;
	}

	NetworkMessageFileHeader headerMsg(remoteName, compress);
	broadcastMessage(&headerMsg);
	size_t i;
	size_t off;
	for(i = 0, off = 0; off < outbuflen; ++i, off += NetworkMessageFileFragment::bufSize) {
		if(outbuflen - off > NetworkMessageFileFragment::bufSize) {
			NetworkMessageFileFragment msg(&outbuf[off], NetworkMessageFileFragment::bufSize, i, false);
			broadcastMessage(&msg);
		} else {
			NetworkMessageFileFragment msg(&outbuf[off], outbuflen - off, i, true);
			broadcastMessage(&msg);
		}
	}
	delete[] inbuf;
	if(compress) {
		delete[] outbuf;
	}

/*	FUCK FUCK FUCK FUCK FUUUUCK!
	int outready = 0;
	int flush;
	z_stream z;
//	unsigned char inbuf[32768];
//	char outbuf[NetworkMessageFileFragment::bufSize];

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;
	z.next_in = Z_NULL;
	z.avail_in = 0;

	NetworkMessageFileHeader headerMsg(remoteName);
	broadcastMessage(&headerMsg);

	if(deflateInit(&z, Z_BEST_COMPRESSION) != Z_OK) {
		throw runtime_error(string("Error initializing zstream: ") + z.msg);
	}

	do {
		in.read((char*)inbuf, sizeof(inbuf));
		z.next_in = (Bytef*)inbuf;
		z.avail_in = in.gcount();
		if(in.bad()) {
			deflateEnd(&z);
			throw runtime_error("Error while reading file " + path);
		}
		flush = in.eof() ? Z_FINISH : Z_NO_FLUSH;

		do {
			z.next_out = (Bytef*)outbuf;
			z.avail_out = sizeof(outbuf);

			zstatus = deflate(&z, flush);
			assert(zstatus != Z_STREAM_ERROR);

			outready = sizeof(outbuf) - z.avail_out;
			if(outready) {
				NetworkMessageFileFragment msg(outbuf, outready, false);
				broadcastMessage(&msg);
			}
		} while (!z.avail_out);
		assert(z.avail_in == 0);

	} while(flush != Z_FINISH);
	assert(zstatus == Z_STREAM_END);

	{
		NetworkMessageFileFragment msg(outbuf, 0, true);
		broadcastMessage(&msg);
	}

	deflateEnd(&z);*/
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
		case uutNew:
			msg.newUnit(i->first);
			break;
		case uutMorph:
			msg.unitMorph(i->first);
			break;
		case uutFullUpdate:
			msg.unitUpdate(i->first);
			break;
		case uutPartialUpdate:
			msg.minorUnitUpdate(i->first);
			break;
		}
		if(i->second != uutPartialUpdate) {
			i->first->setLastUpdated(now);
		}
	}

	if(msg.hasUpdates()) {
		msg.writeXml();
		msg.compress();
		broadcastMessage(&msg);
		updateFactionsFlag = false;
	}
	updateMap.clear();
}
#endif

void ServerInterface::broadcastMessage(const NetworkMessage* networkMessage, int excludeSlot) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(i != excludeSlot && slots[i]) {
			if(slots[i]->isConnected()) {
				slots[i]->send(networkMessage);
			} else {
				Lang &lang = Lang::getInstance();
				string errmsg = slots[i]->getDescription() + " (" + lang.get("Player") + " "
						+ intToStr(slots[i]->getPlayerIndex() + 1) + ") " + lang.get("Disconnected");
				removeSlot(i);
				LOG_NETWORK(errmsg);
				Game::getInstance()->getConsole()->addLine(errmsg);
				//throw SocketException(errmsg);
			}
		}
	}
}

void ServerInterface::updateListen() {
	int openSlotCount = 0;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && !slots[i]->isConnected()) {
			++openSlotCount;
		}
	}
	serverSocket.listen(openSlotCount);
}

}}//end namespace
