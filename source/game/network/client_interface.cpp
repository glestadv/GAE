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
#include "timer.h"

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
	delete clientSocket;
	/*
	delete fileReceiver;
	*/
}

void ClientInterface::connect(const Ip &ip, int port) {
	delete clientSocket;
	clientSocket = new ClientSocket();
	//clientSocket->setBlock(false); //NETWORK: why was moved to after connect
	clientSocket->connect(ip, port);
	clientSocket->setBlock(false);
}

void ClientInterface::reset() {
	delete clientSocket;
	clientSocket = NULL;
}

void ClientInterface::update() {
	NetworkMessageCommandList networkMessageCommandList;

	//send as many commands as we can
	while(!requestedCommands.empty()
			&& networkMessageCommandList.addCommand(&requestedCommands.back())){
		requestedCommands.pop_back();
	}
	/* NETWORK: replaces above
	while(!requestedCommands.empty()
			&& networkMessageCommandList.addCommand(requestedCommands.front())) {
		requestedCommands.erase(requestedCommands.begin());
	}
	*/
	if(networkMessageCommandList.getCommandCount() > 0) {
		send(&networkMessageCommandList);
		//flush();
	}

	//clear chat variables
	chatText.clear();
	chatSender.clear();
	chatTeamIndex = -1;

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

	NetworkMessageType networkMessageType= getNextMessageType();
	
	switch(networkMessageType){
		case nmtInvalid:
			break;

		case nmtIntro:{
			NetworkMessageIntro networkMessageIntro;

			if(receiveMessage(&networkMessageIntro)){
				
				//check consistency
				if(Config::getInstance().getNetConsistencyChecks() && networkMessageIntro.getVersionString()!= getNetworkVersionString()){
					throw runtime_error("Server and client versions do not match (" + networkMessageIntro.getVersionString() + "). You have to use the same binaries.");
				}

				//send intro message
				NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(), getHostName(), -1);

				playerIndex= networkMessageIntro.getPlayerIndex();
				serverName= networkMessageIntro.getName();
				setRemoteNames(networkMessageIntro.getName(), networkMessageIntro.getName()); //NETWORK: needs to be done properly
				send(&sendNetworkMessageIntro);
					
				assert(playerIndex>=0 && playerIndex<GameConstants::maxPlayers);
				introDone= true;
			}
		}
		break;

		case nmtLaunch:{
			NetworkMessageLaunch networkMessageLaunch;

			if(receiveMessage(&networkMessageLaunch)){
				networkMessageLaunch.buildGameSettings(&gameSettings);

				//replace server player by network
				for(int i= 0; i<gameSettings.getFactionCount(); ++i){
					
					//replace by network
					if(gameSettings.getFactionControl(i) == ControlType::HUMAN){
						gameSettings.setFactionControl(i, ControlType::NETWORK);
					}

					//set the faction index
					if(gameSettings.getStartLocationIndex(i)==playerIndex){
						gameSettings.setThisFactionIndex(i);
					}
				}
				launchGame= true;
			}
		}
		break;

		default:
			throw runtime_error("Unexpected network message: " + intToStr(networkMessageType));
	}
}

void ClientInterface::updateKeyframe(int frameCount) {
	// NETWORK: this method is very different
	bool done = false;

	while(!done){
		//wait for the next message
		waitForMessage();

		//check we have an expected message
		NetworkMessageType networkMessageType= getNextMessageType();

		switch(networkMessageType){
			case nmtCommandList:{
				//make sure we read the message
				NetworkMessageCommandList networkMessageCommandList;
				while(!receiveMessage(&networkMessageCommandList)){
					sleep(waitSleepTime);
				}

				//check that we are in the right frame
				if(networkMessageCommandList.getFrameCount()!=frameCount){
					throw runtime_error("Network synchronization error, frame counts do not match");
				}

				// give all commands
				for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i){
					pendingCommands.push_back(*networkMessageCommandList.getCommand(i));
				}				

				done= true;
			}
			break;

			case nmtQuit:{
				NetworkMessageQuit networkMessageQuit;
				if(receiveMessage(&networkMessageQuit)){
					quit= true;
				}
				done= true;
			}
			break;

			case nmtText:{
				NetworkMessageText networkMessageText;
				if(receiveMessage(&networkMessageText)){
					chatText= networkMessageText.getText();
					chatSender= networkMessageText.getSender();
					chatTeamIndex= networkMessageText.getTeamIndex();
				}
			}
			break;

			default:
				throw runtime_error("Unexpected message in client interface: " + intToStr(networkMessageType));
		}
	}
}

void ClientInterface::waitUntilReady(Checksum &checksum) {
	// NETWORK: this method is very different
	NetworkMessageReady networkMessageReady;
	Chrono chrono;

	chrono.start();

	//send ready message
	send(&networkMessageReady);

	//wait until we get a ready message from the server
	while(true){
		
		NetworkMessageType networkMessageType= getNextMessageType();

		if(networkMessageType==nmtReady){
			if(receiveMessage(&networkMessageReady)){
				break;
			}
		}
		else if(networkMessageType==nmtInvalid){
			if(chrono.getMillis()>readyWaitTimeout){
				throw runtime_error("Timeout waiting for server");
			}
		}
		else{
			throw runtime_error("Unexpected network message: " + intToStr(networkMessageType) );
		}

		// sleep a bit
		sleep(waitSleepTime);
	}

	//check checksum
	if(Config::getInstance().getNetConsistencyChecks() 
			&& networkMessageReady.getChecksum() != checksum.getSum()){
		throw runtime_error("Checksum error, you don't have the same data as the server");
	}

	//delay the start a bit, so clients have nore room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex){
	NetworkMessageText networkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex);
	send(&networkMessageText);
}

string ClientInterface::getStatus() const{
	return Lang::getInstance().get("Server") + ": " + serverName;
	//	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

void ClientInterface::waitForMessage() {
	Chrono chrono;

	chrono.start();

	while(getNextMessageType()==nmtInvalid){

		if(!isConnected()){
			throw runtime_error("Disconnected");
		}

		if(chrono.getMillis()>messageWaitTimeout){
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

}}//end namespace
