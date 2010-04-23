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

#ifndef _GLEST_GAME_NETWORKMESSAGE_H_
#define _GLEST_GAME_NETWORKMESSAGE_H_

#include "socket.h"
#include "game_constants.h"
#include "network_types.h"

#include <map>

using std::map;
using Shared::Platform::Socket;
using Shared::Platform::int8;
using Shared::Platform::int16;

namespace Glest { 
	
namespace Game {
	class GameSettings;
	class Command;

	class TechTree;
	class FactionType;
	class UnitType;
	class SkillType;
	class Unit;
}

namespace Net {

class NetworkConnection;

// ==============================================================
//	class Message
// ==============================================================
/** Abstract base class for network messages, requires concrete subclasses 
  * to implement receive(Socket*)/send(Socket*), and provides send/receive methods
  * for them to use to accomplish this. */
class Message {
public:
	virtual ~Message(){}
	virtual bool receive(Socket* socket)= 0;
	virtual void send(Socket* socket) const = 0;

protected:
	bool receive(Socket* socket, void* data, int dataSize);
	bool peek(Socket* socket, void *data, int dataSize);
	void send(Socket* socket, const void* data, int dataSize) const;
};

// ==============================================================
//	class IntroMessage
// ==============================================================
/**	Message sent from the server to the client
  *	when the client connects and vice versa */
class IntroMessage : public Message {
private:
	static const int maxVersionStringSize= 64;
	static const int maxNameSize= 16;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxVersionStringSize> versionString; // change to uint32 ?
		NetworkString<maxNameSize> playerName;
		NetworkString<maxNameSize> hostName;
		int16 playerIndex;
	} data;

public:
	IntroMessage();
	IntroMessage(const string &versionString, const string &pName, const string &hName, int playerIndex);

	string getVersionString() const		{return data.versionString.getString();}
	string getPlayerName() const		{return data.playerName.getString();}
	string getHostName() const			{return data.hostName.getString();}
	int getPlayerIndex() const			{return data.playerIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class AiSeedSyncMessage
// ==============================================================
/** Message sent if there are AI players, to seed their Random objects */
class AiSeedSyncMessage : public Message {
private:
	static const int maxAiSeeds = 3;

private:
	struct Data {
		int8 msgType;
		int8 seedCount;
		int32 seeds[maxAiSeeds];
	} data;

public:
	AiSeedSyncMessage();
	AiSeedSyncMessage(int count, int32 *seeds);

	int getSeedCount() const { return data.seedCount; }
	int32 getSeed(int i) const { return data.seeds[i]; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class ReadyMessage
// ==============================================================
/**	Message sent at the beggining of the game */
class ReadyMessage : public Message {
private:
	struct Data{
		int8 messageType;
		int32 checksum;
	} data;

public:
	ReadyMessage();
	ReadyMessage(int32 checksum);

	int32 getChecksum() const	{return data.checksum;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class LaunchMessage
// ==============================================================
/**	Message sent from the server to the client to launch the game */
class LaunchMessage : public Message {
private:
	static const int maxStringSize= 256;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> description;
		NetworkString<maxStringSize> map;
		NetworkString<maxStringSize> tileset;
		NetworkString<maxStringSize> tech;
		NetworkString<maxStringSize> factionTypeNames[GameConstants::maxPlayers]; //faction names

		int8 factionControls[GameConstants::maxPlayers];
		float resourceMultipliers[GameConstants::maxPlayers];

		int8 thisFactionIndex;
		int8 factionCount;
		int8 teams[GameConstants::maxPlayers];
		int8 startLocationIndex[GameConstants::maxPlayers];
		int8 defaultResources;
		int8 defaultUnits;
		int8 defaultVictoryConditions;
		int8 fogOfWar;
	} data;

public:
	LaunchMessage();
	LaunchMessage(const GameSettings *gameSettings);

	void buildGameSettings(GameSettings *gameSettings) const;

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// ==============================================================
//	class CommandList
// ==============================================================
/**	Message to issue commands to several units */
#pragma pack(push, 2)
class CommandListMessage : public Message {
	friend class NetworkConnection;
private:
	static const int maxCommandCount= 16*4;
	
private:
	static const int dataHeaderSize = 6;
	struct Data{
		int8 messageType;
		int8 commandCount;
		int32 frameCount;
		NetworkCommand commands[maxCommandCount];
	} data;

public:
	CommandListMessage(int32 frameCount= -1);

	bool addCommand(const NetworkCommand* networkCommand);
	void clear()									{data.commandCount= 0;}
	int getCommandCount() const						{return data.commandCount;}
	int getFrameCount() const						{return data.frameCount;}
	const NetworkCommand* getCommand(int i) const	{return &data.commands[i];}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// ==============================================================
//	class TextMessage
// ==============================================================
/**	Chat text message */
class TextMessage : public Message {
private:
	static const int maxStringSize= 64;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> text;
		NetworkString<maxStringSize> sender;
		int8 teamIndex;
	} data;

public:
	TextMessage(){}
	TextMessage(const string &text, const string &sender, int teamIndex);

	string getText() const		{return data.text.getString();}
	string getSender() const	{return data.sender.getString();}
	int getTeamIndex() const	{return data.teamIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// =====================================================
//	class QuitMessage
// =====================================================
/** Message sent by clients to quit nicely, or by the server to terminate the game */
class QuitMessage: public Message {
private:
	struct Data{
		int8 messageType;
	} data;

public:
	QuitMessage();

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
	static size_t getSize() { return sizeof(Data); }
};

// =====================================================
//	class KeyFrame
// =====================================================
class KeyFrame : public Message {
private:
	static const int buffer_size = 1024 * 4;
	static const int max_cmds = 512;
	static const int max_checksums = 2048;
	typedef char* byte_ptr;

	int32	frame;

	int32	checksums[max_checksums];
	int32	checksumCount;
	uint32	checksumCounter;
	
	char	 updateBuffer[buffer_size];
	size_t	 updateSize;
	uint32	 projUpdateCount;
	uint32	 moveUpdateCount;
	byte_ptr writePtr;
	byte_ptr readPtr;

	NetworkCommand commands[max_cmds];
	size_t cmdCount;


public:
	KeyFrame()		{ reset(); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	void setFrameCount(int fc) { frame = fc; }
	int getFrameCount() const { return frame; }

	const size_t& getCmdCount() const	{ return cmdCount; }
	const NetworkCommand* getCmd(size_t ndx) const { return &commands[ndx]; }

	int32 getNextChecksum();
	void addChecksum(int32 cs);
	void add(NetworkCommand &nc);
	void reset();
	void addUpdate(MoveSkillUpdate updt);
	void addUpdate(ProjectileUpdate updt);
	MoveSkillUpdate getMoveUpdate();
	ProjectileUpdate getProjUpdate();
};

#if _GAE_DEBUG_EDITION_

class SyncErrorMsg : public Message {
	struct Data{
		int32	messageType	:  8;
		uint32	frameCount	: 24;
	} data;

public:
	SyncErrorMsg(int frame) {
		data.messageType = MessageType::SYNC_ERROR;
		data.frameCount = frame;
	}
	SyncErrorMsg() {}

	int getFrame() const { return data.frameCount; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};

#endif


}}//end namespace

#endif
