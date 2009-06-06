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

#ifndef _GAME_NET_NETWORKINTERFACE_H_
#define _GAME_NET_NETWORKINTERFACE_H_

#include <queue>

#include "game_interface.h"

using std::queue;

namespace Game { namespace Net {

//class NetworkStatistics;

// =====================================================
//	class RemoteInterface
// =====================================================

/**
 * Base class for the actual network connections used in mulitplayer a game.  Each instance runs
 * its own thread to receive data, so you never need to explicitly request data to be received from
 * the underlying network implementation.  However, the network thread never transmits data other
 * than pings.  Call to send are thread-safe.
 */
class RemoteInterface : public Host, public Scheduleable {
	friend class GameInterface;
	friend class ServerInterface;
	friend class ClientInterface;

public:
	typedef queue<GlestException *> Exceptions;

private:
	GameInterface &owner;
	ClientSocket *socket;
	NetworkStatistics stats;
	int64 pingInterval;
	int64 lastPing;
	Checksums *checksums;
	Exceptions pendingExceptions;
	NetworkDataBuffer txbuf;
	NetworkDataBuffer rxbuf;
#ifdef DEBUG_NETWORK_DELAY
	typedef queue<NetworkMessage *> MessageQueue;
	MessageQueue debugDelayQ;			/** Queue for messages used only when debugging network. */
#endif

public:
	RemoteInterface(GameInterface &owner, NetworkRole role, int id, const uint64 &uid = 0);
	virtual ~RemoteInterface();

	// accessors
	GameInterface &getOwner()					{return owner;}
	ClientSocket *getSocket()					{return socket;}
	const NetworkStatistics &getStats()			{return stats;}
	int64 getPingInterval() const				{return pingInterval;}
	int64 getLastPing() const					{return lastPing;}
	const Checksums *getChecksums() const		{return checksums;}
	GlestException *getPendingException()		{return pendingExceptions.empty() ? NULL : pendingExceptions.front();}
	const string &getStatus() const				{return stats.getStatus();}
	int64 getAvgLatency() const					{return stats.getAvgLatency();}

	// setters
	void setChecksums(Checksums *v)				{assert(!checksums); checksums = v;}
	void setSlot(int v)							{setSlot(v);}

	// misc
	void popPendingException()					{pendingExceptions.pop();}
	void send(NetworkMessage &msg, bool flush = true);
	bool flush();
	NetworkMessage *getNextMsg();
	void update(const int64 &now);
	bool isConnected();
	void updateStatus(const NetworkPlayerStatus &status, NetworkMessage &msg);
	void updatePlayerInfo(const HumanPlayer &player, NetworkMessage &msg);
	string getStatusStr() const				{return getPlayer().getName() + ": " + stats.getStatus();}

#ifdef DEBUG_NETWORK_DELAY
	int64 getNextSimRxTime(const int64 &now) {
		if(!debugDelayQ.empty()) {
			int64 simRxTime = debugDelayQ.front()->getSimRxTime();
			return simRxTime <= now ? 0 : simRxTime;
		}
		return 0;
	}
#endif

	void connect(const IpAddress &ipAddress, unsigned short port);
	void connect(ClientSocket *socket);
	void quit();

	virtual void beginUpdate(int frame, bool isKeyFrame);
	virtual void endUpdate();
	virtual void ping();

	virtual void print(ObjectPrinter &op) const;
	const GameInterface &getGameInterface() const	{return owner;}

private:
	void dispatch(NetworkMessage *msg);
	bool process(NetworkMessageHandshake &msg);
	bool process(NetworkMessagePing &ping);
	bool process(NetworkMessagePlayerInfo &msg);
	bool process(NetworkMessageGameInfo &msg);
	bool process(NetworkMessageStatus &msg);
	bool process(NetworkMessageText &msg);
	bool process(NetworkMessageFileHeader &msg);
	bool process(NetworkMessageFileFragment &msg);
	bool process(NetworkMessageReady &msg);
	bool process(NetworkMessageCommandList &msg);
	bool process(NetworkMessageUpdate &msg);
	bool process(NetworkMessageUpdateRequest &msg);
};

class RemotePeerInterface : public RemoteInterface {
public:
	RemotePeerInterface(ClientInterface &owner, ClientSocket *s, int id, const uint64 &uid);

	void init(int newId, const uint64 &uid) {
		setId(newId);
		setUid(uid);
	}
};

class RemoteServerInterface : public RemoteInterface {
public:
	RemoteServerInterface(ClientInterface &owner);
};

class RemoteClientInterface : public RemoteInterface {
public:
	RemoteClientInterface(ServerInterface &owner, ClientSocket *s, int id);
};

}} // end namespace

#endif
