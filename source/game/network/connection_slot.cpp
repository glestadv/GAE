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

#include "pch.h"
#include "connection_slot.h"

#include <stdexcept>

#include "conversion.h"
#include "game_util.h"
#include "config.h"
#include "server_interface.h"
#include "network_message.h"

#include "leak_dumper.h"
#include "logger.h"
#include "world.h"

using namespace std;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
//	class ClientConnection
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex, bool resumeSaved) {
	this->serverInterface = serverInterface;
	this->playerIndex = playerIndex;
	this->resumeSaved = resumeSaved;
	socket = NULL;
	ready = false;
}

ConnectionSlot::~ConnectionSlot() {
	close();
}

void ConnectionSlot::update() {
	// NETWORK: this method is very different

	if(!socket) {
		socket = serverInterface->getServerSocket()->accept();

		//send intro message when connected
		if(socket) {
			NetworkMessageIntro networkMessageIntro(getNetworkVersionString(), socket->getHostName(), playerIndex);
			send(&networkMessageIntro);
			LOG_NET_SERVER( "Connection established, slot " + intToStr(playerIndex) +  " sending intro message." )
		}
	} else {
		if(socket->isConnected()) {
			NetworkMessageType networkMessageType= getNextMessageType();

			//process incoming commands
			switch(networkMessageType){
				
				case NetworkMessageType::NO_MSG:
				case NetworkMessageType::TEXT:
					break;

				//command list
				case NetworkMessageType::COMMAND_LIST:{
					NetworkMessageCommandList cmdList;
					if(receiveMessage(&cmdList)){
						LOG_NET_SERVER( 
							"Receivied " + intToStr(cmdList.getCommandCount()) + " commands on slot " 
							+ intToStr(playerIndex) + " frame: " + intToStr(theWorld.getFrameCount())
						)
						for (int i=0; i < cmdList.getCommandCount(); ++i) {
							serverInterface->requestCommand(cmdList.getCommand(i));

							const NetworkCommand * const &cmd = cmdList.getCommand(i);
							const Unit * const &unit = theWorld.findUnitById(cmd->getUnitId());
							const UnitType * const &unitType = unit->getType();
							const CommandType * const &cmdType = unitType->findCommandTypeById(cmd->getCommandTypeId());
							LOG_NET_SERVER( 
								"\tUnit: " + intToStr(unit->getId()) + " [" + unitType->getName() + "] " 
								+ cmdType->getName() + "."
							)
						}
					}
				}
				break;

				//process intro messages
				case NetworkMessageType::INTRO: {
					NetworkMessageIntro networkMessageIntro;
					if(receiveMessage(&networkMessageIntro)){
						//name= networkMessageIntro.getName();
						//NETWORK: needs to be done properly
						setRemoteNames(networkMessageIntro.getName(), networkMessageIntro.getName());
					}
					LOG_NET_SERVER( "Received intro message on slot " + intToStr(playerIndex) + ", name = " + getRemotePlayerName() )
				}
				break;

				default:
					LOG_NET_SERVER( "Unexpected message in connection slot: " + intToStr(networkMessageType) )
					throw runtime_error("Unexpected message in connection slot: " + intToStr(networkMessageType));
			}
		}
		else{
			close();
		}
	}
}

void ConnectionSlot::close() {
	delete socket;
	socket = NULL;
}

}}//end namespace
