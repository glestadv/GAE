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

#ifndef _GAME_NET_HOST_H_
#define _GAME_NET_HOST_H_

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
#include "patterns.h"
#include "network_info.h"

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

namespace Game { namespace Net {

class RemoteInterface;
class ClientInterface;
class ServerInterface;
class NetworkPlayerStatus;


/** Base class to represent a network host in a game, either local or remote. */
class Host : public NetworkInfo, Uncopyable {
public:
	static const int READY_WAIT_TIMEOUT = 60000;	//1 minute

protected:
	Mutex mutex;
	Condition cond;

private:
	HumanPlayer player;
	NetworkRole role;
	State state;				/** Current state of this game host */
	ParamChange paramChange;	/** Current pause or speed changes in progress */
	GameParam gameParam;		/** If paramChange != PARAM_CHANGE_NONE, the game parameter being changed, otherwise irrelevant */
	int targetFrame;			/** If paramChange != PARAM_CHANGE_NONE, the targeted frame for the impending game parameter change, otherwise irrelevant */
	GameSpeed newSpeed;			/** If paramChange != PARAM_CHANGE_NONE and gameParam == GAME_PARAM_SPEED, the new game speed, otherwise irrelevant */
	bool peerConnectionState[GameConstants::maxPlayers];
	string description;
	int lastFrame;
	int lastKeyFrame;
	unsigned short port;

public:
	Host(NetworkRole role, int id, const uint64 &uid);
	~Host();

	// accessors
	const HumanPlayer &getPlayer() const	{return player;}
	int getId() const						{return player.getId();}
	NetworkRole getRole() const				{return role;}
	State getState() const					{return state;}
	ParamChange getParamChange() const		{return paramChange;}
	GameParam getGameParam() const			{return gameParam;}
	int getTargetFrame() const				{return targetFrame;}
	GameSpeed getNewSpeed() const			{return newSpeed;}
	bool isConnected(size_t i) const		{assert(i < GameConstants::maxPlayers); return peerConnectionState[i];}
	const string &getDescription() const	{return description;}
	int getLastFrame() const				{return lastFrame;}
	int getLastKeyFrame() const				{return lastKeyFrame;}
	unsigned short getPort() const			{return port;}

//	virtual bool isConnected()				{return connected;}
	virtual Socket *getSocket() = 0;

//	bool isListening() const				{return state == STATE_LISTENING;}
	bool isReady() const					{return state >= STATE_READY;}
//	bool isLaunching() const				{return state == STATE_LAUNCHING;}
//	bool isConnected() const				{return state >= STATE_CONNECTED && state <= STATE_PAUSED;}
	bool isPlay() const						{return state == STATE_PLAY;}
	bool isPaused() const					{return state == STATE_PAUSED;}
	bool isQuit() const						{return state >= STATE_QUIT;}
	bool isEnd() const						{return state == STATE_END;}

//	virtual void beginUpdate(int frame, bool isKeyFrame) = 0;
//	virtual void endUpdate() = 0;
//	virtual bool isConnected() = 0;
	void setId(int id)						{player.setId(id);}

	virtual void print(ObjectPrinter &op) const = 0;
	virtual const GameInterface &getGameInterface() const = 0;
	virtual void updateStatus(const NetworkPlayerStatus &status);

protected:
	Mutex &getMutex()						{return mutex;}
	Condition &getCond()					{return cond;}
	HumanPlayer &getProtectedPlayer()		{return player;}
	void init(State initialState, unsigned short port, const IpAddress &ipAddress);
	void handshake(const Version &gameVersion, const Version &protocolVersion);

	void setState(State v)					{state = v;}
	void setDescription(const string &v)	{description = v;}
};

}} // end namespace

#endif // _GAME_NET_HOST_H_
