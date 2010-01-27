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
#include "network_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkInterface
// =====================================================

const int NetworkInterface::readyWaitTimeout= 60000;	//1 minute


void NetworkInterface::send(const NetworkMessage* networkMessage/*, bool flush*/){
	Socket* socket= getSocket();

	networkMessage->send(socket);

	/*
	size_t startBufSize = txbuf.size();
	msg->writeMsg(txbuf);
	addDataSent(txbuf.size() - startBufSize);
	if(flush) {
		NetworkInterface::flush();
	}
	*/
}

//NETWORK: this is (re)moved
NetworkMessageType NetworkInterface::getNextMessageType(){
	Socket* socket= getSocket();
	int8 messageType= nmtInvalid;

	//peek message type
	if(socket->getDataToRead()>=sizeof(messageType)){
		socket->peek(&messageType, sizeof(messageType));
	}

	//sanity check new message type
	if(messageType<0 || messageType>=nmtCount){
		throw runtime_error("Invalid message type: " + intToStr(messageType));
	}

	return static_cast<NetworkMessageType>(messageType);
}


/** returns false if there is still data to be written *
bool NetworkInterface::flush() {
	if(txbuf.size()) {
		txbuf.pop(getSocket()->send(txbuf.data(), txbuf.size()));
		return !txbuf.size();
	}
	return true;
}
*/

bool NetworkInterface::receiveMessage(NetworkMessage* networkMessage){
	Socket* socket= getSocket();

	return networkMessage->receive(socket);

	/*
	int bytesReceived;
	NetworkMessage *m;
	Socket* socket= getSocket();
	
	rxbuf.ensureRoom(32768);
	while((bytesReceived = socket->receive(rxbuf.data(), rxbuf.room())) > 0) {
		addDataRecieved(bytesReceived);
		rxbuf.resize(rxbuf.size() + bytesReceived);
		while((m = NetworkMessage::readMsg(rxbuf))) {

			// respond immediately to pings
			if(m->getType() == nmtPing) {
#ifdef DEBUG_NETWORK
				pingQ.push_back((NetworkMessagePing*)m);
#else
				processPing((NetworkMessagePing*)m);
#endif
			} else {
				q.push_back(m);
			}
		}
		rxbuf.ensureRoom(32768);
	}
	*/
}

void NetworkInterface::setRemoteNames(const string &hostName, const string &playerName) {
	remoteHostName = hostName;
	remotePlayerName = playerName;

	stringstream str;
	str << remotePlayerName;
	if (!remoteHostName.empty()) {
		str << " (" << remoteHostName << ")";
	}
	description = str.str();
}
/*	
NetworkMessage *NetworkInterface::peek() {
#ifdef DEBUG_NETWORK
	receive();
	
	int now = Chrono::getCurMillis();

	while(!pingQ.empty()) {
		NetworkMessagePing *ping = pingQ.front();
		if(ping->getSimRxTime() < now) {
			processPing(ping);
			pingQ.pop_front();
		} else {
			break;
		} 
	}

	if(!q.empty()) {
		NetworkMessage *msg = q.front();
		return msg->getSimRxTime() < now ? msg : NULL;
	} else {
		return NULL;
	}
#else
	// more processor, more accurate ping times
	receive();
	return q.empty() ? NULL : q.front();
	
	// less processor, less accurate ping times
	/*
	if(!q.empty()) {
		return q.front();
	} else {
		receive();
		return q.empty() ? NULL : q.front();
	}*
#endif
}
*/

// =====================================================
//	class GameNetworkInterface
// =====================================================

GameNetworkInterface::GameNetworkInterface() {
	quit = false;
}

}}//end namespace
