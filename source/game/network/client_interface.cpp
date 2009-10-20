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
#include "protocol_exception.h"

#include "leak_dumper.h"


using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

#define THROW_PROTOCOL_EXCEPTION(description) \
	ProtocolException::coldThrow(source, &msg, (description), NULL, __FILE__, __LINE__)

namespace Game { namespace Net {

// =====================================================
//	class NetworkClientMessenger
// =====================================================

NetworkClientMessenger::NetworkClientMessenger(unsigned short port)
		: NetworkMessenger(NR_CLIENT, port)
		, server(*this)
		, updates()
		, updateRequests()
		, fullUpdateRequests()
		, savedGame(NULL) {
	getLogger().clear();
}

NetworkClientMessenger::~NetworkClientMessenger() {
}

void NetworkClientMessenger::accept() {
	ClientSocket *s = getServerSocket().accept();

	if(s) {
		RemotePeerInterface *peer = new RemotePeerInterface(*this, s, getTemporaryPeerId(), 0);
		addPeer(peer);
		NetworkMessageHandshake msg1(*this);
		peer->send(msg1, true);
	}
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageHandshake &msg) {
	MutexLock lock(getMutex());
	if(&source == &server) {
		assert(getState() < STATE_NEGOTIATED);
		setState(STATE_NEGOTIATED);
		setId(msg.getPlayerId());
		setUid(msg.getUid());
		NetworkMessageHandshake msg1(*this);
		NetworkMessagePlayerInfo msg2(*this);
		source.send(msg1, false);
		msg2.getDoc().writeXml();
		msg2.getDoc().compress();
		source.send(msg2, true);
	} else {
		int id = msg.getPlayerId();
		uint64 uid = msg.getUid();
#ifndef NO_PARINOID_NETWORK_CHECKS
		const Player *p = getGameSettings()->getPlayer(id);
		if(!p) {
			THROW_PROTOCOL_EXCEPTION("Unknown peer (no such id).");
		}

		if(p->getType() != PLAYER_TYPE_HUMAN) {
			THROW_PROTOCOL_EXCEPTION("Peer attempting to connect as non-human player type");
		}

		const HumanPlayer &hp = static_cast<const HumanPlayer &>(*p);
		if(hp.getNetworkInfo().getUid() != uid) {
			THROW_PROTOCOL_EXCEPTION("Peer uid does not match server settings.");
		}
#endif
		if(source.getId() < 0) {
			static_cast<RemotePeerInterface&>(source).init(id, uid);
			NetworkMessageHandshake msg1(*this);
			source.send(msg1, true);
		} else if(source.getId() != uid) {
			THROW_PROTOCOL_EXCEPTION("Peer id mismatch.");
		}
	}
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessagePlayerInfo &msg) {
#ifndef NO_PARINOID_NETWORK_CHECKS
	THROW_PROTOCOL_EXCEPTION("NetworkClientMessenger doesn't accept NetworkMessagePlayerInfo.");
#endif
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageGameInfo &msg) {
#ifndef NO_PARINOID_NETWORK_CHECKS
	if(&source != &server) {
		THROW_PROTOCOL_EXCEPTION("naughty peer tried to send NetworkMessageGameInfo");
	}
#endif
	MutexLock lock(getMutex());
	try {
		setGameSettings(msg.getGameSettings());

		// this is a little screwy, but we want to call getPlayerStatuses() in the try block and
		// then we call it again later to store the reference.  De-uglify this by either
		// enclosing the entire function in this try/catch block or using a pointer instead of a
		// reference.
		msg.getPlayerStatuses();
	} catch (runtime_error &e) {
		THROW_PROTOCOL_EXCEPTION(
				string("Failed to read game-settings from NetworkMessageGameInfo: ") + e.what());
	}
	const NetworkMessageGameInfo::Statuses &statuses = msg.getPlayerStatuses();

	// Update Player information for each RemoteInterface
	foreach(const GameSettings::PlayerMap::value_type &pair, getGameSettings()->getPlayers()) {
		assert(pair.first == pair.second->getId());
		if(pair.second->getType() != PLAYER_TYPE_HUMAN) {
			continue;
		}
		const HumanPlayer &p = static_cast<const HumanPlayer &>(*pair.second);
		int id = p.getId();
		NetworkMessageGameInfo::Statuses::const_iterator statusIterator = statuses.find(id);

		if(statusIterator == statuses.end()) {
			stringstream str;
			str << "Consistency error: did not receive status for human player (id=" << id << ").";
			THROW_PROTOCOL_EXCEPTION(str.str());
		}
		const NetworkPlayerStatus &status = statusIterator->second;

		if(id == Host::getId()) {
			// info for this player

#ifndef NO_PARINOID_NETWORK_CHECKS
			// make sure that the server doesn't try to change things it shouldn't
			// get local player object
			const HumanPlayer &lp = getPlayer();
			if(		   p.getName()								!= lp.getName()
					|| p.getAutoRepairEnabled()					!= lp.getAutoRepairEnabled()
					|| p.getAutoReturnEnabled()					!= lp.getAutoReturnEnabled()
					|| p.getNetworkInfo().getLocalHostName()	!= getLocalHostName()
					|| p.getNetworkInfo().getUid()				!= getUid()
					|| p.getNetworkInfo().getGameVersion()		!= getGameVersion()
					|| p.getNetworkInfo().getProtocolVersion()	!= getProtocolVersion()) {
				stringstream str;
				str << "In NetworkMessageGameInfo, server attempted to change one or more fields "
						"of local player information that it shouldn't." << endl
						<< "Local player: " << getPlayer().toString() << endl
						<< "Server sent: " << p.toString();
				THROW_PROTOCOL_EXCEPTION(str.str());
			}
#endif
			setResolvedHostName(p.getNetworkInfo().getResolvedHostName());
			getProtectedPlayer().setSpectator(p.isSpectator());
		} else {
			// info for a remote player (server or another peer)

			RemoteInterface *peer = getPeer(id);
			if(peer) {
				peer->updatePlayerInfo(p, msg);

				// p2p-TODO: We're skipping clients processing status updates on other clients now,
				// but this needs to be done for peer-to-peer implementation.
				if(peer->getRole() == NR_SERVER) {
					peer->updateStatus(status, msg);

					// If server is launching and we're ready to launch, then change state and let
					// the UI screen query our state and realize we need to change the ProgramState
					// to Game.
					if(peer->getState() == STATE_LAUNCHING && getState() < STATE_LAUNCHING) {
						setState(STATE_LAUNCHING);
					}

					if(getState() < STATE_LAUNCH_READY) {
						setState(STATE_LAUNCH_READY);
					}
				}
			}
			//assert(!peer);
			// If state is STATE_NEGOTIATED then this is the first time we've received this message.
			// Thus, we are the client responsible for connecting with each peer.
			if(getState() == STATE_NEGOTIATED) {
				// p2p-TODO: Store a list of all peers (i.e., other clients to the server) and set a
				// timer for one or two seconds to allow other clients to receive the server's
				// message.  Then attempt to connect to all of the players that I don't have a
				// connection (which should be all of them).  This should be attempted once and
				// perhaps re-attempted every minute or so.  Upon a successful connection to a peer,
				// the NetworkMessageStatus should be broadcast.
				// Note: by storing a list of the peers at this time, we avoid confusion later if
				// another client connects very shortly after we do and another game info message
				// goes out.
			}
		}
	}

	/*
	foreach(NetworkMessageGameInfo::Statuses, msg.getPlayerStatuses()) {
		RemoteInterface *peer = getPeer(id);

	}*/
	if(getState() == STATE_NEGOTIATED) {
		setState(STATE_INITIALIZED);
	}

	if(getState() >= STATE_LAUNCHING) {
		// TODO: manage a late change in game info.  This will happen when somebody drops. A change
		// in game settings after lanuching will require
	}
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageStatus &msg) {
	NetworkPlayerStatus &status = msg.getNetworkPlayerStatus();
	_onReceive(source, status, msg);
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageText &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageFileHeader &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageFileFragment &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageReady &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageCommandList &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageUpdate &msg) {
	THROW_PROTOCOL_EXCEPTION("not yet implemented");
}

void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkMessageUpdateRequest &msg) {
	THROW_PROTOCOL_EXCEPTION("NetworkClientMessenger doesn't accept NetworkMessageUpdateRequest.");
}
void NetworkClientMessenger::_onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg) {
	bool isServer = &source == &server;
	switch(status.getState()) {
		case STATE_UNCONNECTED:
		case STATE_LISTENING:
		case STATE_CONNECTED:
		case STATE_NEGOTIATED:
		case STATE_INITIALIZED:
		case STATE_LAUNCH_READY:
			break;
		case STATE_LAUNCHING:
			if(isServer) {
				setState(STATE_LAUNCH_READY);
			}
			break;
		case STATE_READY:
		case STATE_PLAY:
		case STATE_PAUSED:
			break;
		case STATE_QUIT:
		case STATE_END:
			if(isServer) {
				setState(STATE_QUIT);
			}
			break;
		default:
			break;
	}
	// TODO: check for and handle game parameter change
}

/* * True if world updates should proceed, false otherwise.  This is set to false if a key frame is
 * due but not yet recieved.
 *//*
bool NetworkClientMessenger::isReady() {
	MutexLock localLock(getMutex());
	return ready;
}
*/
/**
 * Process the local request for a command.  All commands generated locally are processed locally.
 * However, only non-auto commands generated by the human player are transmitted accross the
 * network.
 */
void NetworkClientMessenger::requestCommand(Command *command) {
	MutexLock localLock(getMutex());
	if(!command->isAuto() && command->getCommandedUnit()->getFaction()->isThisFaction()) {
		copyCommandToNetwork(command);
	}
	futureCommands[getLastFrame() + getGameSettings()->getCommandDelay()].push(command);
}

void NetworkClientMessenger::beginUpdate(int frame, bool isKeyFrame) {
	MutexLock lock(getMutex());
	NetworkMessenger::beginUpdate(frame, isKeyFrame);
}

void NetworkClientMessenger::endUpdate() {
	NetworkMessageCommandList networkMessageCommandList(*this);
	NetworkMessage *msg;
#if 0
	//send as many commands as we can
	while(!requestedCommands.empty()
			&& networkMessageCommandList.addCommand(requestedCommands.front())) {
		requestedCommands.erase(requestedCommands.begin());
	}

	if(networkMessageCommandList.getCommandCount() > 0) {
		server.send(&networkMessageCommandList);
	}

	while((genericMsg = nextMsg())) {
		try {
			if(processMessage(genericMsg)) {
				delete genericMsg;
				genericMsg = NULL;
				continue;
			}
			switch(genericMsg->getType()) {
			case NMT_UPDATE:
				updates.push_back(reinterpret_cast<NetworkMessageUpdate*>(genericMsg));
				// do not delete
				genericMsg = NULL;
				break;

			default:
				throw SocketException("Unexpected message in client interface: " + intToStr(genericMsg->getType()));
			}
			flush();
		} catch(runtime_error &e) {
			if(genericMsg) {
				delete genericMsg;
			}
			throw e;
		}
	}
#endif
}

void NetworkClientMessenger::connectToServer(const IpAddress &ipAddress, unsigned short port) {
	MutexLock lock(getMutex());
	assert(getState() <= STATE_LISTENING);
	server.connect(ipAddress, port);
	setState(STATE_CONNECTED);
	addPeer(&server);
}

void NetworkClientMessenger::disconnectFromServer() {
	MutexLock lock(getMutex());
	server.quit();
	removePeer(&server);
	setState(STATE_LISTENING);
}

void NetworkClientMessenger::onError(RemoteInterface &ri, GlestException &e) {
	NetworkMessenger::onError(ri, e);
	if(!isConnected()) {
		setState(STATE_UNCONNECTED);
	}
}

void NetworkClientMessenger::sendUpdateRequests() {
	if(isConnected() && updateRequests.size()) {
		NetworkMessageUpdateRequest msg;
		UnitReferences::iterator i;
		for(i = updateRequests.begin(); i != updateRequests.end(); ++i) {
			msg.addUnit(*i, false);
		}
		for(i = fullUpdateRequests.begin(); i != fullUpdateRequests.end(); ++i) {
			msg.addUnit(*i, true);
		}
		msg.getDoc().writeXml();
		msg.getDoc().compress();
		server.send(msg, true);
		updateRequests.clear();
	}
}

string NetworkClientMessenger::getStatus() const {
	stringstream str;
	str << server.getStatus();
	foreach(const PeerMap::value_type &v, getPeers()) {
		if(v.second->getId() != 0) {
			str << endl << v.second->getStatus();
		}
	}
	return str.str();
}

void NetworkClientMessenger::print(ObjectPrinter &op) const {
	NetworkMessenger::print(op.beginClass("NetworkClientMessenger"));
	op		.endClass();
}

void NetworkClientMessenger::update() {

}

/*
bool NetworkClientMessenger::process(RemoteInterface &source, NetworkMessageUpdate &msg) {
	updates.push_back(&msg);
	// do not delete
	return false;
}

bool NetworkClientMessenger::process(RemoteInterface &source, NetworkMessageUpdateRequest &msg) {
	throw runtime_error("unexpected message");
}

bool NetworkClientMessenger::vProcess(RemoteInterface &source, NetworkMessageHandshake &msg) {
	// clients don't have clients
	assert(source.getRole() == NR_SERVER || source.getRole() == NR_PEER);

	//reply with handshake and player info
	source.send(NetworkMessageHandshake());
	source.send(NetworkMessagePlayerInfo(*this));

	return true;
}

bool NetworkClientMessenger::process(RemoteInterface &source, NetworkMessagePlayerInfo &msg) {
	throw runtime_error("unexpected message");
}

bool NetworkClientMessenger::process(RemoteInterface &source, NetworkMessageGameInfo &msg) {
	// clients don't have clients
	assert(source.getRole() == NR_SERVER || source.getRole() == NR_PEER);

}
*/
/*
void NetworkClientMessenger::process(RemoteInterface &source, const NetworkMessageIntro &msg, bool versionsMatch) {
}*//*
bool NetworkClientMessenger::process(RemoteInterface &source, NetworkMessageIntro &msg) {
//	const Version protoVersion = getNetworkVersionString();
//	bool versionsMatch = msg.getVersionString() == version;

	if(role == NR_CLIENT) {
		switch(source->getRole()) {
		case NR_SERVER:
			if (state >= STATE_INTRODUCED) {
				throw runtime_error("Network error: already introduced to server, but received new intro message.");
			}
			break;
		case NR_PEER:
			// hi, go take a flea dip
			if (state >= STATE_INTRODUCED) {
				// Jesus! I already know who you are, shut up.
				return true;
			}
			break;
		default:
			throw runtime_error("I don't like you");
	}

	//check consistency
	if(Config::getInstance().getNetConsistencyChecks() && versionsMatch) {
		throw runtime_error("Server and client versions do not match (" + version + ").");
	}

	//send intro message
	NetworkMessageIntro sendNetworkMessageIntro(*this, getNetworkVersionString(), false);
	state = STATE_INTRODUCED;

	Host::id = msg.getPlayerId();
	source->setRemoteNames(msg.getHostName(), msg.getPlayerName());
	source->send(sendNetworkMessageIntro);

	assert(Host::id >= 0 && Host::id < GameConstants::maxPlayers);

	return true;
}*/
#if 0
void NetworkClientMessenger::updateLobby() {
	NetworkMessage *genericMsg = nextMsg();
	if(!genericMsg) {
		return;
	}

	try {
		switch(genericMsg->getType()) {
			case NMT_INTRO: {
				NetworkMessageIntro *msg = (NetworkMessageIntro *)genericMsg;

				//check consistency
				if(Config::getInstance().getNetConsistencyChecks() && msg->getVersionString() != getNetworkVersionString()) {
					throw SocketException("Server and client versions do not match (" + msg->getVersionString() + ").");
				}

				//send intro message
				NetworkMessageIntro sendNetworkMessageIntro(getNetworkVersionString(),
						getHostName(), Config::getInstance().getNetPlayerName(), -1, false);

				playerIndex = msg->getPlayerIndex();
				setRemoteNames(msg->getHostName(), msg->getPlayerName());
				send(&sendNetworkMessageIntro);

				assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
				introDone = true;
			}
			break;

			case NMT_FILE_HEADER: {
				NetworkMessageFileHeader *msg = (NetworkMessageFileHeader *)genericMsg;

				if(fileReceiver) {
					throw runtime_error("Can't receive file from server because I'm already receiving one.");
				}

				if(savedGameFile != "") {
					throw runtime_error("Saved game file already downloaded and server tried to send me another.");
				}

				if(strchr(msg->getName().c_str(), '/') || strchr(msg->getName().c_str(), '\\')) {
					throw SocketException("Server tried to send a file name with a path component, which is not allowed.");
				}

				Shared::Platform::mkdir("incoming", true);
				fileReceiver = new FileReceiver(*msg, "incoming");
			}
			break;

			case NMT_FILE_FRAGMENT: {
				NetworkMessageFileFragment *msg = (NetworkMessageFileFragment*)genericMsg;

				if(!fileReceiver) {
					throw runtime_error("Recieved file fragment, but did not get header.");
				}
				if(fileReceiver->processFragment(*msg)) {
					savedGameFile = fileReceiver->getName();
					delete fileReceiver;
					fileReceiver = NULL;
				}
			}
			break;

			case NMT_LAUNCH: {
				NetworkMessageLaunch *msg = (NetworkMessageLaunch*)genericMsg;

				msg->buildGameSettings(&gameSettings);

				//replace server player by network
				for(int i = 0; i < gameSettings.getFactionCount(); ++i){

					//replace by network
					if(gameSettings.getFactionControl(i) == CT_HUMAN){
						gameSettings.setFactionControl(i, CT_NETWORK);
					}
				}
				gameSettings.setFactionControl(playerIndex, CT_HUMAN);
				gameSettings.setThisFactionIndex(playerIndex);
				launchGame= true;
			}
			break;

			default:
				throw SocketException("Unexpected network message: " + intToStr(genericMsg->getType()));
		}
		flush();
	} catch (runtime_error &e) {
		delete genericMsg;
		throw e;
	}
	delete genericMsg;
}

void NetworkClientMessenger::waitUntilReady(Checksums &checksums) {
	NetworkMessage *msg = NULL;
	NetworkMessageReady networkMessageReady(&checksums);
	Chrono chrono;

	chrono.start();

	//send ready message
	send(&networkMessageReady);

	try {
		//wait until we get a ready message from the server
		while(!(msg = nextMsg())) {
			if(chrono.getMillis() > READY_WAIT_TIMEOUT){
				throw SocketException("Timeout waiting for server");
			}

			// sleep a bit
			sleep(1);
		}

		if(msg->getType() != NMT_READY) {
			SocketException("Unexpected network message: " + intToStr(msg->getType()));
		}

		//check checksum
		if(Config::getInstance().getNetConsistencyChecks()
				&& ((NetworkMessageReady*)msg)->getChecksum() != checksum.getSum()) {
			throw SocketException("Checksum error, you don't have the same data as the server");
		}
		flush();
	} catch (runtime_error &e) {
		if(msg) {
			delete msg;
		}
		throw e;
	}

	//delay the start a bit, so clients have more room to get messages
	sleep(GameConstants::networkExtraLatency);
	delete msg;
}
#endif
/*
NetworkMessage *NetworkClientMessenger::waitForMessage() {
	NetworkMessage *msg = NULL;
	Chrono chrono;

	chrono.start();

	while (!(msg = nextMsg())) {
		if (!isConnected()) {
			throw SocketException("Disconnected");
		}

		if (chrono.getMillis() > messageWaitTimeout) {
			throw SocketException("Timeout waiting for message");
		}

		sleep(waitSleepTime);
	}
	return msg;
}
*/
}} // end namespace

