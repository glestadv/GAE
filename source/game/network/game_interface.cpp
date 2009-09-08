// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008-2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "game_interface.h"

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
#include "protocol_exception.h"

#include "leak_dumper.h"

//using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;
using Game::Config;

namespace Game { namespace Net {


// =====================================================
//	class GameInterface
// =====================================================

GameInterface::GameInterface(NetworkRole role, unsigned short port, int id)
		: Host(role, id, 0)
		, Thread()
		, socket()
		, requestedCommands()
		, futureCommands()
		, pendingCommands()
		, chatMessages()
		, peers()
		, broadcastTargets(0)
		, fileReceiver(NULL)
		, gameSettings(new GameSettings())
		, gameSettingsChanged(false)
//		, ownGameSettings(false)
		, exception(NULL)
		, peerMapChanged(true)
		, connectionsChanged(false)
		, commandDelay(10) {
	Config &config = Config::getInstance();

	setVersionInfo(getGaeVersion(), getNetProtocolVersion());
	socket.setBlock(false);
	socket.bind(port);
	socket.listen(1);
	init(STATE_LISTENING, port, socket.getIpAddress());
	getProtectedPlayer().setName(config.getNetPlayerName());
	setLocalHostName(socket.getLocalHostName());
	start();
}

GameInterface::~GameInterface() {
	MutexLock lock(mutex);
	if(getState() < STATE_QUIT) {
		setState(STATE_QUIT);
	}

	cond.signal();
	mutex.v();
	join();	// nothing else happens until the thread terminates
	mutex.p();

	if(fileReceiver) {
		delete fileReceiver;
	}

	if(socket.isOpen()) {
		socket.close();
	}

	foreach(const PeerMap::value_type &pair, const_cast<const PeerMap&>(peers)) {
		// slightly hacky, but we don't want to delete the RemoteServerInterface object that
		// ClientInterface objects own
		if(pair.second->getId() != 0) {
			delete pair.second;
		}
	}
}

string GameInterface::getStatus() const {
	Lang &lang = Lang::getInstance();
	stringstream str;

	for (PeerMap::const_iterator i = peers.begin(); i != peers.end(); ++i) {
		str << i->second->getPlayer().getName() + ": " + i->second->getStatus() << endl;
	}
	return str.str();
}


/**
 * Examines the status of all remote hosts and updates local status (including impending game
 * parameter changes) if needed.
 */
void GameInterface::updateLocalStatus() {
	for (PeerMap::const_iterator i = peers.begin(); i != peers.end(); ++i) {
		// TODO
	}
}

/**
 * Network thread main loop.  This function is called when the ClientInterface or ServerInterface
 * is created and is executed until before it's destroyed.  The loop continues to execute until
 * Host::state is set to STATE_QUIT or a fatal exception occurs.  Here is a summary of this loop:
 * -# Update the socketTests vector to represent every RemoteInterface currently connected.
 * -  Iterate through connected RemoteInterfaces and retrieve their next execution time and
 *    calculate which needs to be updated the soonest.
 * -# Perform a multi-object select (or in windows, WaitForMultipleObjects) on each socket in use to
 *    either sleep, completely yielding CPU for the amount of time until the next update is due, or
 *    to be interrupted if data becomes available for reading.
 * -# Iterate through each socket (including the GameInterfaces's server socket):
 *    - For the server socket, if data is available, it indicates a new remote host is attempting to
 *      connect, so we accept that connection and instantiate a new RemoteInterface derived object
 *      as needed.
 *    - For RemoteInterface objects, we check if the socket test indicated data was available and
 *      also if it's due for an update.  Reading data from the network and performing other update
 *      functionality is all implemented in the same call, RemoteInterface::update(int64).  So we
 *      check for either condition and call it's update(int64) function if needed.
 * Exception Handling:
 * If an exception is thrown while attempting to accept a new connection or calling
 * select/WaitForMultipleObjects, then an exception is thrown and not caught by anything, thereby
 * crashing the goddam program (crap, that's a FIXME).  If an exception is thrown while calling any
 * RemoteInterface's update() function, then RemoteInterface will call our onError() function.
 */
void GameInterface::execute() {
	MutexLock lock(mutex);

	int64 waitTime;
	int64 now;

	// main network thread loop.
	while(getState() != STATE_QUIT) {
		assert(getState() < STATE_QUIT);
		int64 now = Chrono::getCurMillis();
		waitTime = 100000;	// max wait time of 100ms

		// only update sockets test vector when an actual change in the peer list occurs.
		if(peerMapChanged) {
			updateSocketTests();
		}
#if 0
		// if we wont be waiting on any sockets, then we wait on the condition instead so we can be
		// interrupted at any time (specifically to be told to quit)
		if(socketTests.size() < 2) {
			mutex.v();
			cond.wait(mutex);
			mutex.p();
			continue;
		}
#endif

		// updates to transmit pings & change status string
		now = Chrono::getCurMicros();
 		foreach (PeerMap::value_type &pair, peers) {
			RemoteInterface &peer = *pair.second;
			if(!peer.getNextExecution() || peer.getState() < STATE_INITIALIZED) {
				continue;
			}
			int64 delay = peer.getNextExecution() - now;
			if(waitTime > delay) {
				waitTime = delay;
			}
		}

		// Sleep and/or wait for I/O
		if(waitTime > 0) {
			mutex.v();
			// Force a one millisecond wait time to allow I/O operations to queue up a bit.  This
			// should slightly reduce CPU usage and make this thread a more viable candidate for
			// elevated priority.
			sleep(10);
			if(waitTime > 10000) {
				waitTime -= 10000;
			} else {
				waitTime = 0;
			}
			// FIXME: NASTINESS!!!  call to wait should be with mutex unlocked, but then if the
			// client clicks disconnect from the lobby (and probably in other conditions) we can
			// end up with the socket object deleted, bad ouch!  So this is crappy for thread
			// friendliess, but will prevent it for now.
			mutex.p();

			// do this again since it could have changed when the mutex was unlocked (above)
			if(peerMapChanged) {
				updateSocketTests();
			}

			Socket::wait<RemoteInterface *>(socketTests, waitTime / 1000);
			if(getState() == STATE_QUIT) {
				break;
			}
		}

		// Update GameInterface's server socket & each RemoteInterface object as needed.
		now = Chrono::getCurMicros();
		foreach (SocketTestRI &st, socketTests) {
			if(st.socket->isServer()) {
				// Accept any new connections
				if(st.result & SOCKET_TEST_READ) {
					accept();
				}
			} else if(st.result & SOCKET_TEST_READ || st.value->getNextExecution() <= now) {
				st.value->update(now);
			}
			if(st.result & SOCKET_TEST_ERROR) {
				// FIXME: do something here
			}
		}

		update();
	}
	setState(STATE_END);
}

/**
 * Updates the socketTests vector with the data needed for a multi-select/WaitOnMultipleObjects
 * call.
 */
void GameInterface::updateSocketTests() {
	socketTests.clear();
	socketTests.push_back(SocketTestRI(&getServerSocket(), SOCKET_TEST_READ | SOCKET_TEST_ERROR, NULL));
	for (PeerMap::const_iterator i = peers.begin(); i != peers.end(); ++i) {
		if(!i->second->getSocket()) {
			continue;
		}
		socketTests.push_back(SocketTestRI(i->second->getSocket(), SOCKET_TEST_READ | SOCKET_TEST_ERROR, i->second));
	}
	peerMapChanged = false;
}

/**
 * Handles an exception that occured in the network thread while processing a connection.
 */
void GameInterface::onError(RemoteInterface &ri, GlestException &e) {
	cerr << e.what();
	getLogger().add(e.what(), false);
	if(!exception) {
		MutexLock localLock(mutex);
		exception = e.clone();
	}
	getLogger().add("Network error processing connection for:", false);
	getLogger().add(static_cast<const Printable &>(ri.getPlayer()), false);
	getLogger().add(e.what(), false);

	removePeer(&ri);
}

void GameInterface::broadcastMessage(NetworkMessage &msg, bool flush) {
	MutexLock lock(mutex);
	for (PeerMap::const_iterator i = peers.begin(); i != peers.end(); ++i) {
		RemoteInterface &client = *i->second;
		client.send(msg, flush);
		/* TODO: Revisit this code before the 0.2.12 release and make sure we're properly handling
		 * disconnects.
		 */
		/*
		if(client.isConnected()) {
			client.send(msg, flush);
		} else {
			Lang &lang = Lang::getInstance();

			stringstream errmsg;
			errmsg << client.getDescription() << " (" << lang.get("Player") << " "
					<< (client.getId() + 1) << ") " << lang.get("Disconnected2");
			// FIXME: improper handling?  Either way, we need to manage this better
			//Game::getInstance()->autoSaveAndPrompt(errmsg, slot->getDescription(), i);
			removePeer(&client);
			delete &client;
			throw runtime_error(errmsg.str());
		}
		*/
	}
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
void GameInterface::end() {
	MutexLock lock(mutex);
	setState(STATE_QUIT);
	// FIXME make sure thread wakes up so we can join on it
	cond.signal();
}

void GameInterface::quit() {
	MutexLock lock(mutex);
	setState(STATE_QUIT);
	NetworkMessageStatus quitMsg(*this);
	broadcastMessage(quitMsg, true);
	end();
}

void GameInterface::copyCommandToNetwork(Command *command) {
	// always call with mutex locked
	MutexLock lock(mutex);
	requestedCommands[getLastFrame() + getGameSettings()->getCommandDelay()].push(new Command(*command));
}

void GameInterface::beginUpdate(int frame, bool isKeyFrame) {/*
	MutexLock lock(mutex);
	lastFrame = frame;

	if(isKeyFrame) {
		lastKeyFrame = frame;
	}

	// move any future commands for this frame to the pendingCommands queue and get rid of all those
	// nasty objects
	CommandQueue::iterator i = futureCommands.find(frame);
	if(i != futureCommands.end()) {
		Commands &commands = i->second;
		while(!commands.empty()) {
			pendingCommands.push(commands.front());
			commands.pop();
		}
		futureCommands.erase(i);
	}*/
}

void GameInterface::endUpdate() {
}

const RemoteInterface &GameInterface::getPeerOrThrow(int id) const throw (range_error) {
	const RemoteInterface *peer = getPeer(id);
	if(!peer) {
		stringstream str;
		str << "no such peer (id = " << id << ")";
		throw range_error(str.str());
	}
	return *peer;
}

void GameInterface::setGameSettings(const shared_ptr<GameSettings> &v) {
	MutexLock localLock(mutex);
	gameSettings = v;
	gameSettingsChanged = true;
}

void GameInterface::changeFaction(bool isAdd, int factionId, int playerId) throw (range_error) {
	MutexLock localLock(mutex);
	const GameSettings::Faction &f = gameSettings->getFactionOrThrow(factionId);
	HumanPlayer &humanPlayer = getPeerOrThrow(playerId).getProtectedPlayer();
	if(isAdd) {
		gameSettings->addPlayerToFaction(f, humanPlayer);
	} else {
		gameSettings->removePlayerFromFaction(f, humanPlayer);
	}
	const Player &gsPlayer = gameSettings->getPlayerOrThrow(playerId);
	assert(gsPlayer.getType() == PLAYER_TYPE_HUMAN);
	const HumanPlayer &gsHumanPlayer = static_cast<const HumanPlayer &>(gsPlayer);
	humanPlayer.setSpectator(gsHumanPlayer.isSpectator());
	gameSettingsChanged = true;
}

void GameInterface::setGameSettingsChanged(bool v) {
	MutexLock localLock(mutex);
	gameSettingsChanged = v;
}

void GameInterface::resetConnectionsChanged() {
	MutexLock localLock(mutex);
	connectionsChanged = false;
}

void GameInterface::setConnectionsChanged() {
	MutexLock localLock(mutex);
	connectionsChanged = true;
}

/*
void onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg) {
	_onReceive(source, status, msg);
}
*/

/**
 * Process a message.
 * @return true if the should be deleted, false otherwise
 *//*
bool GameInterface::process(RemoteInterface &source, NetworkMessage *msg) {
	MutexLock lock(mutex);

	switch(msg->getType()) {

	case NMT_STATUS:
		return process(source, *dynamic_cast<NetworkMessageStatus*>(msg));
	case NMT_HANDSHAKE:
		return process(source, *dynamic_cast<NetworkMessageHandshake*>(msg));
	case NMT_PLAYER_INFO:
		return process(source, *dynamic_cast<NetworkMessagePlayerInfo*>(msg));
	case NMT_GAME_INFO:
		return process(source, *dynamic_cast<NetworkMessageGameInfo*>(msg));
	case NMT_TEXT:
		return process(source, *dynamic_cast<NetworkMessageText*>(msg));
	case NMT_FILE_HEADER:
		return process(source, *dynamic_cast<NetworkMessageFileHeader*>(msg));
	case NMT_FILE_FRAGMENT:
		return process(source, *dynamic_cast<NetworkMessageFileFragment*>(msg));
	case NMT_READY:
		return process(source, *dynamic_cast<NetworkMessageReady*>(msg));
	case NMT_COMMAND_LIST:
		return process(source, *dynamic_cast<NetworkMessageCommandList*>(msg));
	case NMT_UPDATE:
		return process(source, *dynamic_cast<NetworkMessageUpdate*>(msg));
	case NMT_UPDATE_REQUEST:
		return process(source, *dynamic_cast<NetworkMessageUpdateRequest*>(msg));
	default:
		assert(0);
		throw runtime_error("unknown message type " + msg->getType());
	}
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageStatus &msg) {
	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageHandshake &msg) {
	bool ret;

	if(getState() >= STATE_NEGOTIATED) {
		throw runtime_error("unexpected message");
	}

	// some day, we may want more complex support of older protocol verions
	if(getNetProtocolVersion() != msg.getProtocolVersion()) {
		// FIXME: language neutral message
		throw runtime_error("Remote peer and client network versions do not match.  (You have "
				+ getNetProtocolVersion().toString() + " and peer has "
				+ msg.getProtocolVersion().toString() + ".)");
	}

	source.setGameVersion(msg.getGameVersion());
	source.setProtocolVersion(msg.getProtocolVersion());
	source.setState(STATE_INTRODUCED);

	return vProcess(source, msg);
}

bool GameInterface::process(RemoteInterface &source, NetworkMessagePlayerInfo &msg) {
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageGameInfo &msg) {
	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageText &msg) {
	chatMessages.push(new ChatMessage(msg.getText(), msg.getSender(), msg.getTeamIndex()));
	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageFileHeader &msg) {
	if(fileReceiver) {
		throw runtime_error("Can't receive file from server because I'm already receiving one.");
	}

	if(savedGameFile != "") {
		throw runtime_error("Saved game file already downloaded and server tried to send me another.");
	}

	if(strchr(msg.getName().c_str(), '/') || strchr(msg.getName().c_str(), '\\')) {
		throw SocketException("Server tried to send a file name with a path component, which is not allowed.");
	}

	Shared::Platform::mkdir("incoming", true);
	fileReceiver = new FileReceiver(msg, "incoming");

	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageFileFragment &msg) {
	if(!fileReceiver) {
		throw runtime_error("Recieved file fragment, but did not get header.");
	}
	if(fileReceiver->processFragment(msg)) {
		savedGameFile = fileReceiver->getName();
		delete fileReceiver;
		fileReceiver = NULL;
	}

	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageLaunch &msg) {
	if(state != STATE_READY) {
		throw runtime_error("recieved NMT_LANCH at unexpected time");
	}

	if(role != NR_CLIENT || source.getRole() != NR_SERVER) {
		throw runtime_error("stfu");
	}
	if(gameSettings && ownGameSettings) {
		delete gameSettings;
		gameSettings = NULL;
	}

	gameSettings = msg.createGameSettings();
	ownGameSettings = true;

	//replace server player by network
	const GameSettings::Factions &factions = gameSettings->getFactions();
	for(GameSettings::Factions::const_iterator i = factions.begin(); i != factions.end(); ++i) {
		if((*i)->getControlType() == CT_HUMAN) {
			(*i)->setControlType(CT_NETWORK);
		}
	}
	factions[Host::id]->setControlType(CT_HUMAN);
	gameSettings->setThisFactionId(Host::id);
	state = STATE_LAUNCHING;

	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageReady &msg) {
	return true;
}

bool GameInterface::process(RemoteInterface &source, NetworkMessageCommandList &msg) {
	if(!msg.hasFrame()) {
		throw runtime_error("Recieved NMT_COMMAND_LIST without a frame and that's not "
				"supposed to happen (I can't do anything with it).");
	}
	// if this is for the current or recent frame, add them to pending, otherwise, they go
	// in futureCommands.
	if(msg.getFrame() <= lastFrame) {
		for(int i= 0; i < msg.getCommandCount(); ++i) {
			pendingCommands.push(msg.getCommand(i));
		}
	} else {
		Commands &commands = futureCommands[msg.getFrame()];
		for(int i= 0; i < msg.getCommandCount(); ++i) {
			commands.push(msg.getCommand(i));
		}
	}
	msg.clear();

	return true;
}

*/
void GameInterface::sendTextMessage(const string &text, int teamIndex) {
	MutexLock lock(mutex);
//	outQ.push(new NetworkMessageText(text, Config::getInstance().getNetPlayerName(), teamIndex));
}

void GameInterface::addRemovePeer(bool isAdd, RemoteInterface *peer) {
	MutexLock lock(mutex);
	shared_ptr<MutexLock> gsLock = gameSettings->getLock();
	int id = peer->getId();
	const Player *gsPlayer = gameSettings->getPlayer(id);

	// a few sanity checks
	assert(id >= 0);
	assert(id > 0 || peer->getRole() == NR_SERVER);
	// if isAdd, we want to make sure they don't exist, if removing, make sure they do
	assert(isAdd == (peers.find(id) == peers.end()));

	if(gsPlayer) {
#ifndef NO_PARINOID_NETWORK_CHECKS
		// If the GameSettings already has a Player by this ID, verify that it's the same one
		if(!peer->getPlayer().isSame(*gsPlayer)) {
			assert(0);
			throw GlestException(
					"Internal Consistiency Error: An attempt to add or remove a peer was invoked "
					"when an existing player in the GameSettings was found with the same ID, which "
					"does not match the specified peer",
	 				"GameInterface::addRemovePeer()",
	  				NULL, __FILE__, __LINE__);
		}
#endif
	} else {
		// otherwise, we add the new peer's player info as a spectator.
		const HumanPlayer &p = peer->getPlayer();
		if(isAdd) {
			gameSettings->addSpectator(p);
			peer->getProtectedPlayer().setSpectator(true);
		} else {
			gameSettings->removePlayer(p);
		}
		connectionsChanged = true;
	}

	if(isAdd) {
		peers[id] = peer;
	} else {
		peers.erase(id);
		// This is a wee-bit hacky, but we don't delete RemotetServerInterface objects,
		// ClientInterfaces own them.  We could circumvent this problem by changed PeerMap to
		// type map<int, shared_ptr<RemoteInterface> > if we wanted to.
		if(id != 0) {
			delete peer;
		}
	}

	peerMapChanged = true;
}

bool GameInterface::isLocalHumanPlayer(const Player &p) const {
	return p.getType() == PLAYER_TYPE_HUMAN
			&& static_cast<const HumanPlayer&>(p).getNetworkInfo().getUid() == getUid();
	//getPlayer().getName() == p.getName();
}

/*
void GameInterface::setPeerId(RemoteInterface *peer, int newId) {
	MutexLock lock(mutex);
	// may only set a peer ID if it's -1 (i.e., un-initialized)
	assert(peer->getId() == -1);
	assert(newId >= 0 || newId < GameConstants::maxPlayers);
	assert(peers[newId] == NULL);
	peers[newId] = peer;
	peer->setId(newId);
}
*/
/**
 * Retreives the next usable id.
 */
int GameInterface::getNextPeerId() const {
	int highestId = 0;
	foreach(const PeerMap::value_type &p, peers) {
		if(p.first > highestId) {
			highestId = p.first;
		}
	}
	return highestId + 1;
}

/**
 * Retreives a temporary id.
 */
int GameInterface::getTemporaryPeerId() const {
	int lowestId = 0;
	foreach(const PeerMap::value_type &p, peers) {
		if(p.first < lowestId) {
			lowestId = p.first;
		}
	}
	return lowestId - 1;
}

/** Send a file to all clients. */
void GameInterface::sendFile(const string path, const string remoteName, bool compress) {
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
		outbuflen = static_cast<uLongf>(fileSize * 1.001f + 12.f);
		outbuf = new char[outbuflen];
		zstatus = ::compress((Bytef *)outbuf, &outbuflen, (const Bytef *)inbuf, fileSize);
		if(zstatus != Z_OK) {
			throw runtime_error("Call to compress failed for some unknown fucking reason.");
		}
	} else {
		outbuf = inbuf;
		outbuflen = fileSize;
	}

	NetworkMessageFileHeader headerMsg(remoteName, fileSize, compress);
	broadcastMessage(headerMsg, false);
	size_t i;
	size_t off;
	for(i = 0, off = 0; off < outbuflen; ++i, off += NetworkMessageFileFragment::bufSize) {
		if(outbuflen - off > NetworkMessageFileFragment::bufSize) {
			NetworkMessageFileFragment msg(&outbuf[off], NetworkMessageFileFragment::bufSize, i, false);
			broadcastMessage(msg);
		} else {
			NetworkMessageFileFragment msg(&outbuf[off], outbuflen - off, i, true);
			broadcastMessage(msg);
		}
	}
	delete[] inbuf;
	if(compress) {
		delete[] outbuf;
	}

/*	FUCK FUCK FUCK FUCK FUUUUCK!
	Please leave this code here, even though it's commented out.  This is the send file
	implementation that compresses in a stream (i.e., incremental vs allocating a huge buffer and
	compressing a file all at once) and it's really needed instead of the above implementation.
	However, it's also broken :( Maybe you can fix it!

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
	broadcastMessage(&headerMsg, false);

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

void GameInterface::print(ObjectPrinter &op) const {
	Host::print(op.beginClass("GameInterface"));
	op		.print("broadcastTargets", broadcastTargets)
			.print("fileReceiver", (void *)fileReceiver)
			.print("gameSettings", (void *)gameSettings.get())
//			.print("ownGameSettings", ownGameSettings)
//			.print("savedGameFile", savedGameFile)
			.print("exception", (void *)exception)
			.endClass();
}

}} // end namespace
