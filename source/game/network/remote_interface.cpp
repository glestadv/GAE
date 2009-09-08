// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "remote_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"
#include "config.h"
#include "exceptions.h"
#include "client_interface.h"
#include "server_interface.h"
#include "game_util.h"
#include "network_manager.h"
#include "protocol_exception.h"

#include "leak_dumper.h"

//using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;
using Game::Config;
using Game::Logger;

#define THROW_PROTOCOL_EXCEPTION(description) \
	throw ProtocolException(*this, &msg, description, NULL, __FILE__, __LINE__)

namespace Game { namespace Net {

// =====================================================
//	class RemoteInterface
// =====================================================

//const int NetworkInterface::readyWaitTimeout = 60000;	//1 minute

RemoteInterface::RemoteInterface(GameInterface &owner, NetworkRole role, int id, const uint64 &uid)
		: Host(role, id, uid)
		, Scheduleable(0, 0)
		, owner(owner)
		, socket(NULL)
		, stats(*this)
		, pingInterval(1000000)
		, lastPing(0)
		, checksums(NULL)
		, pendingExceptions()
		, txbuf(16384)
		, rxbuf(16384)
#ifdef DEBUG_NETWORK_DELAY
		, debugDelayQ()
#endif
		{
}

RemoteInterface::~RemoteInterface() {
}

/**
 * Main driving function for RemoteInterface class.  Being that this is a rather important function,
 * I'll clarify its functionality more than I normally would. This function should be called when
 * either:
 * -# this->getNextExecution() returns a time value <= now, or
 * -# this->socket has data to be read (determined by Socket::wait<RemoteInterface *>)
 * The precise behavior of this function varies depending upon rather or not it's compiled with
 * DEBUG_NETWORK_DELAY.  Luckily, the differences are transparent to the caller and the bulk of the
 * network lag simulation functionality is encapsulated in this function.  The following is the
 * normal flow (i.e., without DEBUG_NETWORK_DELAY):
 * -# Reads any available data from socket.
 * -# Assembles a NetworkMessage from said data if sufficient data is available.
 * -# If a complete message is available, then the message is processed via the RemoteInterface's
 *    message dispatcher
 *    - otherwise, the new message is queued using its simulated rx time
 * -# Checks the NetworkStatistics object to see if it needs to be updated by calling
 *    stats.getNextExecution().  If it returns a time value <= now, then it's update() function is
 *    called (also it's main driving function).
 * -# The next scheduled execution time is set to the NetworkStatistics object's next execution time
 *    (which is recalculated each time it's update() function is called).
 * Differences when compiled with DEBUG_NETWORK_DELAY:
 * - Messages in step 3 are queued in (the data member) q instead of being sent to the message
 *   dispatcher.
 * - Prior to step 4, the q is checked to see if there are any messages that have been recieved
 *   (either this time or previously) that are ready to be processed.  If so, they are finally sent
 *   through the message dispatcher.
 * - In set 6, instead of directly setting the next execution time ({@see Scheduleable}) using that
 *   of the stats object, the q is first checked to see if it's non-empty and if non-empty, then
 *   the lessor (sooner) of times between the stats object's nextExecutionTime() and the next
 *   message's simulated rx time is used to determine the next execution time of the RemoteInterface
 *   object.  This ensures that messages queued to simulate network lag are retrieved in the most
 *   timely fashion (should be less than 1mS from the scheduled time).
 * In short, I have attempted to contain the "uglies" of this here, making this function a bit
 * uglier, but keeping the rest of the code cleaner.
 */
void RemoteInterface::update(const int64 &now) {
	try {
		MutexLock lock(mutex);
		if(!socket) {
			return;
		}
		int bytesReceived;
		NetworkMessage *msg;

		rxbuf.ensureRoom(32768);
		size_t room = rxbuf.room();
		bytesReceived = socket->receive(rxbuf.data(), room);
		assert(bytesReceived < room);
		if(bytesReceived > 0) {
			// NOTE: DEBUG_NETWORK_DELAY will not cause throughput statistics to be delayed.  That would
			// be more pain that it's worth IMO because we would have to break raw bytes recieved apart
			// into their prospective messages.
			stats.addDataRecieved(bytesReceived);
			rxbuf.resize(rxbuf.size() + bytesReceived);
		}
		while((msg = NetworkMessage::readMsg(rxbuf))) {
			if(Config::getInstance().getMiscDebugMode()) {
				stringstream str;
				str << "received from peer[" << getId() << "]: ";
				owner.getLogger().add(str.str(), *msg);
			}

#ifdef DEBUG_NETWORK_DELAY
			// Simulated network lag
			debugDelayQ.push(msg);
		}

		while(!debugDelayQ.empty() && debugDelayQ.front()->getSimRxTime() <= now) {
			msg = debugDelayQ.front();
			debugDelayQ.pop();
#endif // DEBUG_NETWORK_DELAY
			dispatch(msg);
		}

		// Update NetworkStatistics object if it's time.
		if(stats.getNextExecution() <= now) {
			stats.update(now);
		}

		setLastExecution(now);

		// Set the next scheduled call to update this object.
#ifdef DEBUG_NETWORK_DELAY
		int64 nextSimRxTime = debugDelayQ.empty() ? 0 : debugDelayQ.front()->getSimRxTime();
		if(nextSimRxTime && nextSimRxTime < stats.getNextExecution()) {
			setNextExecution(nextSimRxTime);
		} else {
			setNextExecution(stats.getNextExecution());
		}
#else
		setNextExecution(stats.getNextExecution());
#endif
	} catch(GlestException &e) {
		owner.onError(*this, e);
	}
}

void RemoteInterface::dispatch(NetworkMessage *msg) {
	bool ret = false;
	try {
		switch(msg->getType()) {

		case NMT_HANDSHAKE:
			ret = process(static_cast<NetworkMessageHandshake &>(*msg));
			break;
		case NMT_PING:
			ret = process(static_cast<NetworkMessagePing &>(*msg));
			break;
		case NMT_PLAYER_INFO:
			ret = process(static_cast<NetworkMessagePlayerInfo &>(*msg));
			break;
		case NMT_GAME_INFO:
			ret = process(static_cast<NetworkMessageGameInfo &>(*msg));
			break;
		case NMT_STATUS:
			ret = process(static_cast<NetworkMessageStatus &>(*msg));
			break;
		case NMT_TEXT:
			ret = process(static_cast<NetworkMessageText &>(*msg));
			break;
		case NMT_FILE_HEADER:
			ret = process(static_cast<NetworkMessageFileHeader &>(*msg));
			break;
		case NMT_FILE_FRAGMENT:
			ret = process(static_cast<NetworkMessageFileFragment &>(*msg));
			break;
		case NMT_READY:
			ret = process(static_cast<NetworkMessageReady &>(*msg));
			break;
		case NMT_COMMAND_LIST:
			ret = process(static_cast<NetworkMessageCommandList &>(*msg));
			break;
		case NMT_UPDATE:
			ret = process(static_cast<NetworkMessageUpdate &>(*msg));
			break;
		case NMT_UPDATE_REQUEST:
			ret = process(static_cast<NetworkMessageUpdateRequest &>(*msg));
			break;
		default: {
				stringstream str;
				str << "unknown message type " << msg->getType();
				throw ProtocolException(*this, NULL, str.str().c_str(), NULL, __FILE__, __LINE__);
			}
		}
	} catch(ProtocolException &e) {
		// if ProtocolException is thrown, the exception takes ownership of the message.
		//owner.onError(*this, e);
		throw e;
	} catch(GlestException &e) {
		if(msg) {
			delete msg;
		}
		//owner.onError(*this, e);
		//pendingExceptions.push_back(e.clone());
		throw e;
	} catch(...) {
		// shouldn't have any other exceptions being thrown
		assert(0);
		throw;
	}
	if(ret) {
		delete msg;
	}
}

bool RemoteInterface::process(NetworkMessageHandshake &msg) {
	bool ret;

	if(getState() >= STATE_NEGOTIATED) {
		throw ProtocolException(*this, &msg,
				"NetworkMessageHandshake unexpected at this time.",
				NULL, __FILE__, __LINE__);
	}

	// some day, we may want more complex support of older protocol verions
	if(getNetProtocolVersion() != msg.getProtocolVersion()) {
		throw ProtocolException(*this, &msg,
				Lang::getInstance().format("ErrorVersionMismatch",
						getNetProtocolVersion().toString().c_str(),
						msg.getProtocolVersion().toString().c_str()).c_str(),
				NULL, __FILE__, __LINE__);
	}
	handshake(msg.getGameVersion(), msg.getProtocolVersion());
	setState(STATE_NEGOTIATED);

	owner.onReceive(*this, msg);

	return true;
}

bool RemoteInterface::process(NetworkMessagePing &msg) {
	if(msg.isPong()) {
		stats.pong(msg.getTime(), msg.getTimeRcvd());
	} else {
		msg.setPong();
		send(msg);
		flush();
	}
	return true;
}

bool RemoteInterface::process(NetworkMessagePlayerInfo &msg) {
	MutexLock localLock(mutex);

	// only valid after handshake
	if(getState() != STATE_NEGOTIATED) {
		throw ProtocolException(*this, &msg,
				"NetworkMessagePlayerInfo unexpected at this time.",
				NULL, __FILE__, __LINE__);
	}

	// only RemoteClientInterface receives this message type
	if(getRole() != NR_CLIENT) {
		throw ProtocolException(*this, &msg,
				"NetworkMessagePlayerInfo should only be sent by clients to the server.",
				NULL, __FILE__, __LINE__);
	}

	updatePlayerInfo(msg.getPlayer(), msg);

	owner.onReceive(*this, msg);

	return true;
}

bool RemoteInterface::process(NetworkMessageGameInfo &msg) {
	// only RemoteServerInterface receives this message type
	if(getRole() != NR_SERVER) {
		throw ProtocolException(*this, &msg,
				"NetworkMessageGameInfo should only be sent by the server.",
				NULL, __FILE__, __LINE__);
	}

	owner.onReceive(*this, msg);

	return true;
}

bool RemoteInterface::process(NetworkMessageStatus &msg) {
	owner.onReceive(*this, msg);
	updateStatus(msg.getNetworkPlayerStatus(), msg);

	return true;
}

bool RemoteInterface::process(NetworkMessageText &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

bool RemoteInterface::process(NetworkMessageFileHeader &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

bool RemoteInterface::process(NetworkMessageFileFragment &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

bool RemoteInterface::process(NetworkMessageReady &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

bool RemoteInterface::process(NetworkMessageCommandList &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

bool RemoteInterface::process(NetworkMessageUpdate &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

bool RemoteInterface::process(NetworkMessageUpdateRequest &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}


void RemoteInterface::beginUpdate(int frame, bool isKeyFrame) {
	MutexLock lock(mutex);
}

void RemoteInterface::endUpdate() {
}

void RemoteInterface::send(NetworkMessage &msg, bool flush) {
	MutexLock lock(mutex);
	size_t startBufSize = txbuf.size();
	msg.writeMsg(txbuf);
	stats.addDataSent(txbuf.size() - startBufSize);
	if(Config::getInstance().getMiscDebugMode()) {
		stringstream str;
		str << "sent to peer[" << getId() << "]: ";
		owner.getLogger().add(str.str(), static_cast<Printable &>(msg), false);
	}
	if(flush) {
		this->flush();
	}
}

/**
 * Writes as much pending data to the network as possible.
 * @return true if all data was written, false if another call to flush is needed later to write
 *		   all pending data.  When blocking is enabled, this function should always return true,
 *		   but may block the current thread.
 */
bool RemoteInterface::flush() {
	MutexLock lock(mutex);
	if(txbuf.size()) {
		txbuf.pop(socket->send(txbuf.data(), txbuf.size()));
		return !txbuf.size();
	}
	return true;
}

//bool RemoteInterface::process(NetworkMessageStatus &msg) {
	/*
	int msgSourceId = msg->getSource();
	if(msgSourceId != id) {
		RemoteInterface *peer = owner.getPeer(msgSourceId);
		if(!peer || peer == this || peer->getSlot() != msgSourceId) {
			throw GlestException("Invalid source id in message and/or inconsistent game state.");
		}
		peer->updateStatus(msg);
		return;
	}

	setState(msg->getState());
	setParamChange(msg->getParamChange());
	setGameParam(msg->getGameParam());
	if(msg->hasFrame()) {
		setLastFrame(msg->getFrame());
	}
	if(getParamChange() != PARAM_CHANGE_NONE) {
		if(!msg->hasTargetFrame()) {
			throw runtime_error("Message contains game parameter change, but a target frame was not specified.");
		}
		setTargetFrame(msg->getTargetFrame());
		if(getGameParam() == GAME_PARAM_SPEED) {
			setNewSpeed(msg->getGameSpeed());
		}
	}

	owner.updateLocalStatus();*/
//	return true;
//}
/*
bool RemoteInterface::receive() {
	MutexLock lock(mutex);
	int bytesReceived;
	NetworkMessage *m;

	rxbuf.ensureRoom(32768);
	if((bytesReceived = socket->receive(rxbuf.data(), rxbuf.room())) > 0) {
		addDataRecieved(bytesReceived);
		rxbuf.resize(rxbuf.size() + bytesReceived);
		while((m = NetworkMessage::readMsg(rxbuf))) {
			if(owner.process(*this, *m)) {
				delete m;
			}
			rxbuf.ensureRoom(32768);
		}
		return true;
	}
	return false;
}*/


/*
void RemoteInterface::setRemoteNames(const string &hostName, const string &playerName) {
	setHostName(hostName);
	setPlayerName(playerName);

	stringstream str;
	str << playerName;
	if (!hostName.empty()) {
		str << " (" << hostName << ")";
	}
	setDescription(str.str());
}*/

void RemoteInterface::ping() {
	if(isConnected()) {
		NetworkMessagePing msg;
		send(msg);
	}
}
/*
void RemoteInterface::execute() {
	while(state != STATE_QUIT) {
		if(receiveSingle()) {
			onReceive();
		}
		NetworkStatus::update();
	}
}*/

void RemoteInterface::connect(const IpAddress &ipAddress, unsigned short port) {
	assert(getState() == STATE_UNCONNECTED);
	assert(!socket);

	ClientSocket *socket = NULL;
	try {
		ClientSocket *socket = new ClientSocket(ipAddress, port);
		socket->setBlock(false);
		this->socket = socket;
		init(STATE_CONNECTED, port, ipAddress);
	} catch(...) {
		if(socket) {
			delete socket;
			socket = NULL;
		}
		throw;
	}
}

void RemoteInterface::connect(ClientSocket *socket) {
	assert(!this->socket);
	this->socket = socket;
	setIpAndPort(socket->getRemoteAddr(), socket->getRemotePort());
	setResolvedHostName(socket->getRemoteHostName());
	socket->setBlock(false);
}

void RemoteInterface::quit() {
	if(getSocket()) {
		if(getState() >= STATE_INITIALIZED) {
			setState(STATE_QUIT);
			NetworkMessageStatus msg1(*this);
			send(msg1, true);
		}
		delete socket;
		socket = NULL;
	}
	setState(STATE_UNCONNECTED);
}

bool RemoteInterface::isConnected() {
	return socket && socket->isConnected();
}

void RemoteInterface::updateStatus(const NetworkPlayerStatus &status, NetworkMessage &msg) {
	switch(status.getState()) {
		case STATE_UNCONNECTED:
		case STATE_LISTENING:
		case STATE_CONNECTED:
		case STATE_NEGOTIATED:
		case STATE_INITIALIZED:
		case STATE_LAUNCH_READY:
		case STATE_LAUNCHING:
		case STATE_READY:
		case STATE_PLAY:
		case STATE_PAUSED:
		case STATE_QUIT:
		case STATE_END:
			THROW_PROTOCOL_EXCEPTION((string("handling of state ") + enumStateNames.getName(status.getState())
					+ " not yet implemented.").c_str());
		default:
			assert(0);
	}
}

void RemoteInterface::updatePlayerInfo(const HumanPlayer &player, NetworkMessage &msg) {
	// validate the client is properly identified.
	if(player.getId() != getId()
			|| player.getNetworkInfo().getUid() != getUid()
	  		|| player.getType() != PLAYER_TYPE_HUMAN) {
		throw ProtocolException(*this, &msg,
				"NetworkMessagePlayerInfo received with incorrect/invalid id, uid and/or type.",
				NULL, __FILE__, __LINE__);
	}

	// everything else is OK, so capture their info.
	getProtectedPlayer().copyVitals(player);
	setLocalHostName(player.getNetworkInfo().getLocalHostName());
	if(getState() < STATE_INITIALIZED) {
		setState(STATE_INITIALIZED);
	}
}

void RemoteInterface::print(ObjectPrinter &op) const {
	Host::print(op.beginClass("RemoteInterface"));
	op		.print("owner.id", owner.Host::getId())
			.print("socket", (void *)socket)
			.print("stats.getStatus()", stats.getStatus())
			.print("pingInterval", pingInterval)
			.print("lastPing", lastPing)
			.print("checksums", (void *)checksums)
			.print("txbuf.size()", txbuf.size())
			.print("rxbuf.size()", rxbuf.size())
			.endClass();
}

RemotePeerInterface::RemotePeerInterface(ClientInterface &owner, ClientSocket *s, int id, const uint64 &uid)
		: RemoteInterface(owner, NR_PEER, id, uid) {
	connect(s);
}

RemoteServerInterface::RemoteServerInterface(ClientInterface &owner)
		: RemoteInterface(owner, NR_SERVER, 0) {
	setId(0);
}

RemoteClientInterface::RemoteClientInterface(ServerInterface &owner, ClientSocket *s, int id)
		: RemoteInterface(owner, NR_CLIENT, id, NetworkInfo::getRandomUid()) {
	connect(s);
}


/*
void RemoteInterface::reset() {
	delete socket;
	socket = new ClientSocket();
	socket->setBlock(false);
}
*/

// =====================================================
//	class GameNetworkInterface
// =====================================================
/*
GameNetworkInterface::GameNetworkInterface() : quit(false) {
}

GameNetworkInterface::~GameNetworkInterface(){
	assert(quit);
	quit = true;
	join();
}

void GameNetworkInterface::sendTextMessage(const string &text, int teamIndex) {
	MutexLock lock(mutex);
	NetworkMessageText networkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex);
	send(&networkMessageText);
}

string GameNetworkInterface::getStatus() const {
	return getRemotePlayerName() + ": " + NetworkStatus::getStatus();
}

void GameNetworkInterface::execute() {
	while(!quit) {
		update();
		//cond.wait(mutex, 20);
	}
}
*/
}} // end namespace
