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
#include "client_interface.h"

#include <stdexcept>
#include <cassert>

#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "leak_dumper.h"
#include "logger.h"
#include "timer.h"

#include "world.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;
using Shared::Platform::Chrono;

namespace Glest { namespace Game {
/*
ClientInterface::FileReceiver::FileReceiver(const NetworkMessageFileHeader &msg, const string &outdir) :
		name(outdir + "/" + msg.getName()),
		out(name.c_str(), ios::binary | ios::out | ios::trunc) {
	if(out.bad()) {
		throw runtime_error("Failed to open new file for output: " + msg.getName());
	}
	compressed = msg.isCompressed();
	finished = false;
	nextseq = 0;
}

ClientInterface::FileReceiver::~FileReceiver() {
}

bool ClientInterface::FileReceiver::processFragment(const NetworkMessageFileFragment &msg) {
    int zstatus;

	assert(!finished);
	if(finished) {
		throw runtime_error(string("Received file fragment after download of file ")
				+ name + " was already completed.");
	}

	if(!compressed) {
		out.write(msg.getData(), msg.getDataSize());
		if(out.bad()) {
			throw runtime_error("Error while writing file " + name);
		}
	}

	if(nextseq == 0){
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;
		z.avail_in = 0;
		z.next_in = Z_NULL;

		if(inflateInit(&z) != Z_OK) {
			throw runtime_error(string("Failed to initialize zstream: ") + z.msg);
		}
	}

	if(nextseq++ != msg.getSeq()) {
		throw runtime_error("File fragments arrived out of sequence, which isn't "
				"supposed to happen with stream sockets.  Did somebody change "
				"the socket implementation to datagrams?");
	}

	z.avail_in = msg.getDataSize();
	z.next_in = (Bytef*)msg.getData();
	do {
		z.avail_out = sizeof(buf);
		z.next_out = (Bytef*)buf;
		zstatus = inflate(&z, Z_NO_FLUSH);
		assert(zstatus != Z_STREAM_ERROR);	// state not clobbered
		switch (zstatus) {
		case Z_NEED_DICT:
			zstatus = Z_DATA_ERROR;
			// intentional fall-through
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			throw runtime_error(string("error in zstream: ") + z.msg);
		}
		out.write(buf, sizeof(buf) - z.avail_out);
		if(out.bad()) {
			throw runtime_error("Error while writing file " + name);
		}
	} while (z.avail_out == 0);

	if(msg.isLast() && zstatus != Z_STREAM_END) {
		throw runtime_error("Unexpected end of zstream data.");
	}

	if(msg.isLast() || zstatus == Z_STREAM_END) {
		finished = true;
		inflateEnd(&z);
	}

	return msg.isLast();
}
*/

// =====================================================
//	class ClientInterface
// =====================================================

const int ClientInterface::messageWaitTimeout = 10000;	//10 seconds
const int ClientInterface::waitSleepTime = 5; // changed from 50, no obvious effect

ClientInterface::ClientInterface(){
	clientSocket = NULL;
	launchGame = false;
	introDone = false;
	playerIndex = -1;
	/*
	fileReceiver = NULL;
	savedGameFile = "";
	*/
}

ClientInterface::~ClientInterface() {
	if(clientSocket && clientSocket->isConnected()) {
		string text = getHostName() + " has left the game!";
		sendTextMessage(text,-1);
	}
	delete clientSocket;
	clientSocket = NULL;
	/*
	delete fileReceiver;
	*/
}

void ClientInterface::connect(const Ip &ip, int port) {
	delete clientSocket;
	clientSocket = new ClientSocket();
	clientSocket->connect(ip, port);
	clientSocket->setBlock(false);
	LOG_NETWORK( "connecting to " + ip.getString() + ":" + intToStr(port) );
}

void ClientInterface::reset() {
	if(clientSocket) {
		string text = getHostName() + " has left the game!";
		sendTextMessage(text,-1);
	}
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::update() {
	NetworkMessageCommandList cmdList;

	//send as many commands as we can
	while (!requestedCommands.empty() && cmdList.addCommand(&requestedCommands.back())) {
		requestedCommands.pop_back();
	}
	if (cmdList.getCommandCount() > 0) {
		/*LOG_NETWORK(
			"Sending command(s) to server, current frame = " + intToStr(theWorld.getFrameCount())
		);
		for (int i=0; i < cmdList.getCommandCount(); ++i) {
			const NetworkCommand * const &cmd = cmdList.getCommand(i);
			const Unit * const &unit = theWorld.findUnitById(cmd->getUnitId());
			LOG_NETWORK( 
				"\tUnit: " + intToStr(cmd->getUnitId()) + " [" + unit->getType()->getName() + "] " 
				+ unit->getType()->findCommandTypeById(cmd->getCommandTypeId())->getName() + "."
			);
		}*/
		send(&cmdList);
		//flush();
	}

	/*
	if(isConnected()) {
		receive();
		MsgQueue newQ;

		// pull out updates for immediate processing
		for(MsgQueue::iterator i = q.begin(); i != q.end(); ++i) {
			if((*i)->getType() == nmtUpdate) {
				updates.push_back((NetworkMessageUpdate*)*i);
			} else {
				newQ.push_back(*i);
			}
		}
		q.swap(newQ);
		NetworkStatus::update();
	}
	flush();
	*/
}

/*
void ClientInterface::sendUpdateRequests() {
	if(isConnected() && updateRequests.size()) {
		NetworkMessageUpdateRequest msg;
		UnitReferences::iterator i;
		for(i = updateRequests.begin(); i != updateRequests.end(); ++i) {
			msg.addUnit(*i, false);
		}
		for(i = fullUpdateRequests.begin(); i != fullUpdateRequests.end(); ++i) {
			msg.addUnit(*i, true);
		}
		msg.writeXml();
		msg.compress();
		send(&msg, true);
		updateRequests.clear();
	}
}
*/

void ClientInterface::updateLobby() {
	// NETWORK: this method is very different
	NetworkMessageType msgType = getNextMessageType();
	
	if (msgType == NetworkMessageType::INTRO) {
		NetworkMessageIntro introMsg;
		if (receiveMessage(&introMsg)) {
			//check consistency
			if(theConfig.getNetConsistencyChecks() && introMsg.getVersionString() != getNetworkVersionString()) {
				throw runtime_error("Server and client versions do not match (" 
							+ introMsg.getVersionString() + "). You have to use the same binaries.");
			}
			LOG_NETWORK( "Received intro message, sending intro message reply." );
			playerIndex = introMsg.getPlayerIndex();
			serverName = introMsg.getName();
			setRemoteNames(introMsg.getName(), introMsg.getName()); //NETWORK: needs to be done properly

			if (playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
				throw runtime_error("Intro message from server contains bad data, are you using the same version?");
			}
			
			//send reply
			NetworkMessageIntro replyMsg(getNetworkVersionString(), getHostName(), -1);
			send(&replyMsg);
			introDone= true;
		}
	} else if (msgType == NetworkMessageType::LAUNCH) {
		NetworkMessageLaunch launchMsg;
		if (receiveMessage(&launchMsg)) {
			launchMsg.buildGameSettings(&gameSettings);
			//replace server player by network
			for (int i= 0; i < gameSettings.getFactionCount(); ++i) {
				//replace by network
				if (gameSettings.getFactionControl(i) == ControlType::HUMAN) {
					gameSettings.setFactionControl(i, ControlType::NETWORK);
				}
				//set the faction index
				if (gameSettings.getStartLocationIndex(i)==playerIndex) {
					gameSettings.setThisFactionIndex(i);
				}
			}
			launchGame= true;
			LOG_NETWORK( "Received launch message." );
		}
	} else if (msgType != NetworkMessageType::NO_MSG) {
		throw runtime_error("Unexpected network message: " + intToStr(msgType));
	}
}

void ClientInterface::updateKeyframe(int frameCount) {
	// NETWORK: this method is very different
	while (true) {
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType msgType = getNextMessageType();

		if (msgType == NetworkMessageType::COMMAND_LIST) {
			NetworkMessageCommandList cmdList;
			//make sure we read the message
			while (!receiveMessage(&cmdList)) {
				sleep(waitSleepTime);
			}
			//check that we are in the right frame
			if (cmdList.getFrameCount() != frameCount) {
				throw runtime_error("Network synchronization error, frame counts do not match");
			}
			// give all commands
			if (cmdList.getCommandCount()) {
				/*LOG_NETWORK( 
					"Keyframe update: " + intToStr(frameCount) + " received "
					+ intToStr(cmdList.getCommandCount()) + " commands. " 
					+ intToStr(dataAvailable()) + " bytes waiting to be read."
				);*/
				for (int i= 0; i < cmdList.getCommandCount(); ++i) {
					pendingCommands.push_back(*cmdList.getCommand(i));
					/*const NetworkCommand * const &cmd = cmdList.getCommand(i);
					const Unit * const &unit = theWorld.findUnitById(cmd->getUnitId());
					LOG_NETWORK( 
						"\tUnit: " + intToStr(unit->getId()) + " [" + unit->getType()->getName() + "] " 
						+ unit->getType()->findCommandTypeById(cmd->getCommandTypeId())->getName() + "."
					);*/
				}
			}
			return;
		} else if (msgType == NetworkMessageType::QUIT) {
			NetworkMessageQuit quitMsg;
			if (receiveMessage(&quitMsg)) {
				quit = true;
			}
			return;
		} else if (msgType == NetworkMessageType::TEXT) {
			NetworkMessageText textMsg;
			if (receiveMessage(&textMsg)) {
				GameNetworkInterface::processTextMessage(textMsg);
			}
		} else {
			throw runtime_error("Unexpected message in client interface: " + intToStr(msgType));
		}
	}
}

void ClientInterface::syncAiSeeds(int aiCount, int *seeds) {
	NetworkMessageAiSeedSync seedSyncMsg;
	Chrono chrono;
	chrono.start();
	LOG_NETWORK( "Ready, waiting for server to send Ai random number seeds." );
	//wait until we get a ai seed sync message from the server
	while (true) {
		NetworkMessageType msgType = getNextMessageType();
		if (msgType == NetworkMessageType::AI_SYNC) {
			if (receiveMessage(&seedSyncMsg)) {
				LOG_NETWORK( "Received AI random number seed message." );
				assert(seedSyncMsg.getSeedCount());
				for (int i=0; i < seedSyncMsg.getSeedCount(); ++i) {
					seeds[i] = seedSyncMsg.getSeed(i);
				}
				break;
			}
		} else if (msgType == NetworkMessageType::NO_MSG) {
			if (chrono.getMillis() > readyWaitTimeout) {
				throw runtime_error("Timeout waiting for server");
			}
		} else {
			throw runtime_error("Unexpected network message: " + intToStr(msgType) );
		}
		sleep(2);
	}
}

void ClientInterface::waitUntilReady(Checksum &checksum) {
	// NETWORK: this method is very different
	NetworkMessageReady readyMsg;
	Chrono chrono;
	chrono.start();

	//send ready message
	send(&readyMsg);
	LOG_NETWORK( "Ready, waiting for server ready message." );

	//wait until we get a ready message from the server
	while (true) {
		NetworkMessageType msgType = getNextMessageType();
		if (msgType == NetworkMessageType::READY) {
			if (receiveMessage(&readyMsg)) {
				LOG_NETWORK( "Received ready message." );
				break;
			}
		} else if (msgType == NetworkMessageType::NO_MSG) {
			if (chrono.getMillis() > readyWaitTimeout) {
				throw runtime_error("Timeout waiting for server");
			}
		} else {
			throw runtime_error("Unexpected network message: " + intToStr(msgType) );
		}

		// sleep a bit
		sleep(waitSleepTime);
	}

	//check checksum
	if (theConfig.getNetConsistencyChecks() && readyMsg.getChecksum() != checksum.getSum()) {
		throw runtime_error("Checksum error, you don't have the same data as the server");
	}

	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText textMsg(text, theConfig.getNetPlayerName(), teamIndex);
	send(&textMsg);
}

string ClientInterface::getStatus() const{
	return theLang.get("Server") + ": " + serverName;
	//	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

void ClientInterface::waitForMessage() {
	Chrono chrono;
	chrono.start();
	while (getNextMessageType() == NetworkMessageType::NO_MSG) {
		if (!isConnected()) {
			throw runtime_error("Disconnected");
		}
		if (chrono.getMillis() > messageWaitTimeout) {
			throw runtime_error("Timeout waiting for message");
		}
		sleep(waitSleepTime);
	}
}

/*
void ClientInterface::requestCommand(Command *command) {
	if(!command->isAuto()) {
		requestedCommands.push_back(new Command(*command));
	}
	pendingCommands.push_back(command);
}
*/

void ClientInterface::quitGame() {
	if(clientSocket && clientSocket->isConnected()) {
		string sQuitText = getHostName() + " has chosen to leave the game!";
		sendTextMessage(sQuitText,-1);
	}
	delete clientSocket;
	clientSocket = NULL;
}

}}//end namespace
