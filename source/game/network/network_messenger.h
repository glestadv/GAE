// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_NET_GAMEINTERFACE_H_
#define _GAME_NET_GAMEINTERFACE_H_

//FIXME: dependecy hell
#include "host.h"
/*
#include <string>
#include <vector>
#include <deque>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "network_status.h"
#include "thread.h"
#include "chat_manager.h"
#include "file_transfer.h"

using std::string;
using std::vector;
using std::deque;
//using Shared::Util::Checksums;
//using namespace Shared::Platform;
using Shared::Platform::ClientSocket;
using Shared::Platform::IpAddress;
using Shared::Platform::ServerSocket;
using Shared::Platform::Socket;
using Shared::Platform::Thread;
using Shared::Platform::Mutex;
using Shared::Platform::Condition;
using namespace Game::Net;
*/

#include "logger.h"

using Game::Logger;

namespace Game { namespace Net {

/**
 * A network delgate to handle all network functions for a network game while hiding those
 * details from the users of the NetworkMessenger.  A NetworkMessenger may actually
 * interact with more than one remote hosts whom are in either of the roles of server, client or
 * peer (client to client).
 */
class NetworkMessenger : public Host, private Thread {
protected:
	typedef queue<shared_ptr<Command> > Commands;
	typedef map<int, Commands> CommandQueue;
	typedef map<int, RemoteInterface *> PeerMap;
	typedef SocketTestData<RemoteInterface *> SocketTestRI;
	typedef vector<SocketTestRI> SocketTests;
	static const int messageWaitTimeout = 30;

public:
	typedef map<int, const RemoteInterface *> ConstPeerMap;

private:
	ServerSocket socket;			/**< Socket for listening and accepting new connections from either clients or peers */
protected:
	CommandQueue requestedCommands;	/**< Commands requested locally */
	CommandQueue futureCommands;	/**< Commands (incoming and local) queued up for future execution */
	Commands pendingCommands;		/**< Commands (ready to be executed) */
private:
	ChatMessages chatMessages;		/**< Incoming chat messages */
	PeerMap peers;					/**< Remote peers indexed by their id */
	unsigned int broadcastTargets;	/**< The current target(s) for network operations. */

	// transient object pointers
	FileReceiver *fileReceiver;
	string savedGameFileName;
	shared_ptr<GameSettings> gameSettings;	/**< Settings for this game */
	bool gameSettingsChanged;
	GlestException *exception;

	SocketTests socketTests;
	bool peerMapChanged;			/**< True if peerMap has changed and socketTests needs re-initialization. */
	bool connectionsChanged;		/**< True if new connections have been established or somebody dropped. */
	size_t commandDelay;

public:
	NetworkMessenger(NetworkRole role, unsigned short port, int id = IdNamePair::invalidId);
	virtual ~NetworkMessenger();

	virtual void beginUpdate(int frame, bool isKeyFrame);
	virtual void endUpdate();

	// message related
	void clearTargets()			{broadcastTargets = 0;}
	void setAllTargets()		{broadcastTargets = 0xffffffffu;}
	void setTarget(int i, bool set) {
		assert(i < GameConstants::maxPlayers);
		unsigned int bit = 1 < i;
		broadcastTargets = (broadcastTargets & ~bit) | bit;
	}
	void broadcastMessage(NetworkMessage &msg, bool flush = false);
	void sendTextMessage(const string &text, int teamIndex);
	void sendFile(const string path, const string remoteName, bool compress = true);
	void updateRemoteStatus(const NetworkMessageStatus *msg);

	virtual void requestCommand(Command *command) = 0;
	virtual void end();
	virtual void quit();
	virtual void onError(RemoteInterface &ri, GlestException &e);
	virtual void accept() = 0;
	virtual Logger &getLogger() = 0;
	//virtual void waitUntilReady(Checksums &checksums) = 0;

	//accessors
	virtual string getStatus() const;
protected:
	Socket *getSocket()								{return &socket;}
	ServerSocket &getServerSocket()					{return socket;}
public:
	size_t getConnectionCount() const				{return peers.size();}
 	const string &getSavedGameFileName() const		{return savedGameFileName;}
	const shared_ptr<GameSettings> &getGameSettings() const	{return gameSettings;}
	bool isGameSettingsChanged() const				{return gameSettingsChanged;}

	bool hasChatMessage() const						{return !chatMessages.empty();}
	const GlestException *getException() const		{return exception;}
	bool isConnectionsChanged() const				{return connectionsChanged;}
	void resetConnectionsChanged();
	bool isLocalHumanPlayer(const Player &p) const;
	size_t getCommandDelay() const					{return commandDelay;}
	const RemoteInterface &getPeerOrThrow(int id) const throw (range_error);
	const RemoteInterface *getPeer(int id) const {
		const PeerMap::const_iterator i = peers.find(id);
		return i == peers.end() ? NULL : i->second;
	}

	// ugly, but const-correct non-const getter wrappers
	RemoteInterface &getPeerOrThrow(int id) throw (range_error) {
		return const_cast<RemoteInterface &>(
			const_cast<const NetworkMessenger*>(this)->getPeerOrThrow(id));
	}

	RemoteInterface *getPeer(int id) {
		return const_cast<RemoteInterface *>(
			const_cast<const NetworkMessenger*>(this)->getPeer(id));
	}

	const ConstPeerMap &getConstPeers() const	{return reinterpret_cast<const ConstPeerMap &>(peers);}

	void onReceive(RemoteInterface &source, NetworkMessageHandshake &msg)	{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessagePlayerInfo &msg)	{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageGameInfo &msg)	{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageStatus &msg)		{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageText &msg)		{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageFileHeader &msg)	{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageFileFragment &msg){_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageReady &msg)		{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageCommandList &msg)	{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageUpdate &msg)		{_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkMessageUpdateRequest &msg){_onReceive(source, msg);}
	void onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg){_onReceive(source, status, msg);}

	virtual void print(ObjectPrinter &op) const = 0;
	const NetworkMessenger &getNetworkMessenger() const		{return *this;}

private:
	virtual void _onReceive(RemoteInterface &source, NetworkMessageHandshake &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessagePlayerInfo &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageGameInfo &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageStatus &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageText &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageFileHeader &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageFileFragment &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageReady &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageCommandList &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageUpdate &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkMessageUpdateRequest &msg) = 0;
	virtual void _onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg) = 0;

public:
//	void send()									{cond.signal();}
//	void newCommand(Command *cmd)				{MutexLock lock(mutex); pendingCommands.push_back(cmd);}

	ChatMessage *getNextChatMessage() {
		if(chatMessages.empty()) {
			return NULL;
		} else {
			ChatMessage *msg = chatMessages.front();
			chatMessages.pop();
			return msg;
		}
	}

	void updateLocalStatus();
	void setGameSettings(const shared_ptr<GameSettings> &v);
	void addPlayerToFaction(int factionId, int playerId) throw(range_error)		{changeFaction(true, factionId, playerId);}
	void removePlayerFromFaction(int factionId, int playerId) throw(range_error){changeFaction(false, factionId, playerId);}
	virtual void update() = 0;

//	const string &getSavedGameFile() const		{return savedGameFile;}

protected:
	// Shared::Platform::Thread implementation
	virtual void execute();

	// various protected
	void addPeer(RemoteInterface *peer)			{addRemovePeer(true, peer);}
	void removePeer(RemoteInterface *peer)		{addRemovePeer(false, peer);}
	const PeerMap &getPeers() const				{return peers;}
	void setPeerId(RemoteInterface *peer, int newId);
	int getNextPeerId() const;
	int getTemporaryPeerId() const;
	void copyCommandToNetwork(Command *cmd);
	void setSavedGameFileName(const string &v)	{savedGameFileName = v;}
	void setCommandDelay(size_t v)				{commandDelay = v;}
	void setGameSettingsChanged(bool v);

	void setConnectionsChanged();

	// command-related
	CommandQueue &getRequestedCommands()		{return requestedCommands;}
	CommandQueue &getFutureCommands()			{return futureCommands;}
	Commands &getPendingCommands()				{return pendingCommands;}

private:
	void updateSocketTests();
	void changeFaction(bool isAdd, int factionId, int playerId) throw(range_error);
	void addRemovePeer(bool isAdd, RemoteInterface *peer);
};

}} // end namespace

#endif // _GAME_NET_GAMEINTERFACE_H_
