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
#include "checksum.h"

#include <map>

using std::map;
using Shared::Platform::Socket;
using Shared::Platform::int8;
using Shared::Platform::int16;
using Shared::Util::Checksum;
using Glest::Sim::GameSpeed;

namespace Glest { namespace Net {

class NetworkConnection;

struct RawMessage {
	uint32 type;
	uint32 size;
	uint8* data;

	RawMessage() : type(0), size(0), data(0) { }
	RawMessage(const RawMessage &m) : type(m.type), size(m.size), data(m.data) { }
};

struct MsgHeader {
	static const size_t headerSize = 4;
	uint32 messageType :  8;
	uint32 messageSize : 24;
};

// ==============================================================
//	class Message
// ==============================================================
/** Abstract base class for network messages, requires concrete subclasses 
  * to implement receive(Socket*)/send(Socket*), and provides send/receive methods
  * for them to use to accomplish this. */
class Message {
public:
	virtual ~Message(){}
	virtual bool receive(NetworkConnection* connection) = 0;
	virtual void send(NetworkConnection* connection) const = 0;

protected:
	bool receive(NetworkConnection* connection, void* data, int dataSize);
	bool peek(NetworkConnection* connection, void *data, int dataSize);
	void send(NetworkConnection* connection, const void* data, int dataSize) const;
	void send(Socket* socket, const void* data, int dataSize) const;
};

// ==============================================================
//	class IntroMessage
// ==============================================================
/**	Message sent from the server to the client
  *	when the client connects and vice versa */
class IntroMessage : public Message {
private:
	static const int maxVersionStringSize = 64;
	static const int maxNameSize = 16;

private:
	struct Data {
		uint32 messageType :  8;
		uint32 messageSize : 24;
		NetworkString<maxVersionStringSize> versionString;
		NetworkString<maxNameSize> playerName;
		NetworkString<maxNameSize> hostName;
		int16 playerIndex;
	} data; // 4 + 64 + 32 + 2 == 102

	// 0x 01 00 00 66

public:
	IntroMessage();
	IntroMessage(const string &versionString, const string &pName, const string &hName, int playerIndex);
	IntroMessage(RawMessage raw);

	string getVersionString() const		{return data.versionString.getString();}
	string getPlayerName() const		{return data.playerName.getString();}
	string getHostName() const			{return data.hostName.getString();}
	int getPlayerIndex() const			{return data.playerIndex;}

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
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
		uint32 messageType :  8;
		uint32 messageSize : 24;
		int8 seedCount;
		int32 seeds[maxAiSeeds];
	} data;

public:
	AiSeedSyncMessage();
	AiSeedSyncMessage(int count, int32 *seeds);
	AiSeedSyncMessage(RawMessage raw);

	int getSeedCount() const { return data.seedCount; }
	int32 getSeed(int i) const { return data.seeds[i]; }

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

// ==============================================================
//	class ReadyMessage
// ==============================================================
/**	Message sent at the beggining of the game */
class ReadyMessage : public Message {
private:
	MsgHeader data;

public:
	ReadyMessage();
	ReadyMessage(RawMessage raw);

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
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
		uint32 messageType :  8;
		uint32 messageSize : 24;
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
		int8 colourIndices[GameConstants::maxPlayers];
		int8 defaultResources;
		int8 defaultUnits;
		int8 defaultVictoryConditions;
		int8 fogOfWar;
		int8 shroudOfDarkness;
	} data;

public:
	LaunchMessage();
	LaunchMessage(const GameSettings *gameSettings);
	LaunchMessage(RawMessage raw);

	void buildGameSettings(GameSettings *gameSettings) const;

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

class DataSyncMessage : public Message {
private:
	///@todo struct Data { ... } m_data;
	int32 m_cmdTypeCount;
	int32 m_skillTypeCount;
	int32 m_prodTypeCount;
	int32 m_cloakTypeCount;
	int32 *m_data;
	RawMessage rawMsg;

public:
	DataSyncMessage(RawMessage raw);
	DataSyncMessage(World &world);
	~DataSyncMessage();

	int32 getChecksum(int i) { return m_data[i]; }

	int32 getCmdTypeCount()	  const { return m_cmdTypeCount;   }
	int32 getSkillTypeCount() const { return m_skillTypeCount; }
	int32 getProdTypeCount() const { return m_prodTypeCount; }
	int32 getCloakTypeCount() const { return m_cloakTypeCount; }

	int32 getChecksumCount()  const {
		return m_cmdTypeCount + m_skillTypeCount + m_prodTypeCount + m_cloakTypeCount + 4;
	}

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

class GameSpeedMessage : public Message {
private:
	struct Data {
		uint32 messageType	:  8;
		uint32 messageSize	: 24;
		uint32 frame		: 24;
		uint32 setting		:  8;
	} data;

public:
	GameSpeedMessage(int frame, GameSpeed setting);
	GameSpeedMessage(RawMessage raw);

	int getFrame() const { return data.frame; }
	GameSpeed getSetting() const { return enum_cast<GameSpeed>(data.setting); }

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

// ==============================================================
//	class CommandList
// ==============================================================
/**	Message to send (unit) commands to server */
class CommandListMessage : public Message {
	friend class NetworkConnection;
private:
	static const int maxCommandCount = 16 * 4;
	static const int dataHeaderSize = 8;

private:
#	pragma pack(push, 4)
	struct Data {
		uint32 messageType	:  8;
		uint32 messageSize	: 24;
		uint32 commandCount	:  8;
		uint32 frameCount	: 24;
		NetworkCommand commands[maxCommandCount];
	} data;
#	pragma pack(pop)

public:
	CommandListMessage(int32 frameCount= -1);
	CommandListMessage(RawMessage raw);
	
	bool addCommand(const NetworkCommand* networkCommand);
	void clear()									{data.commandCount = 0;}
	int getCommandCount() const						{return data.commandCount;}
	int getFrameCount() const						{return data.frameCount;}
	const NetworkCommand* getCommand(int i) const	{return &data.commands[i];}

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

// ==============================================================
//	class TextMessage
// ==============================================================
/**	Chat text message */
class TextMessage : public Message {
public:
	static const int maxStringSize = 64;

private:
#	pragma pack(push, 2)
	struct Data{
		uint32 messageType :  8;
		uint32 messageSize : 24;
		NetworkString<maxStringSize> text;
		NetworkString<maxStringSize> sender;
		int8 teamIndex;
		int8 colourIndex;
	} data;
#	pragma pack(pop)

public:
	TextMessage(){}
	TextMessage(const string &text, const string &sender, int teamIndex, int colourIndex);
	TextMessage(RawMessage raw);

	string getText() const		{return data.text.getString();}
	string getSender() const	{return data.sender.getString();}
	int getTeamIndex() const	{return data.teamIndex;}
	int getColourIndex() const	{return data.colourIndex;}

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

// =====================================================
//	class QuitMessage
// =====================================================
/** Message sent by clients to quit nicely, or by the server to terminate the game */
class QuitMessage: public Message {
private:
	struct Data {
		uint32 messageType :  8;
		uint32 messageSize : 24;
	} data;

public:
	QuitMessage();
	QuitMessage(RawMessage raw);

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

// =====================================================
//	class KeyFrame
// =====================================================
class KeyFrame : public Message {
private:
	static const int buffer_size = 1024 * 4;
	static const int max_cmds = 512;
	static const int max_checksums = 2048;
	typedef uint8* byte_ptr;

	int32	frame;

	IF_MAD_SYNC_CHECKS(
		int32	checksums[max_checksums];
		int32	checksumCount;
		uint32	checksumCounter;
	)

	uint8	 updateBuffer[buffer_size];
	size_t	 updateSize;
	uint32	 projUpdateCount;
	uint32	 moveUpdateCount;
	byte_ptr writePtr;
	byte_ptr readPtr;

	NetworkCommand commands[max_cmds];
	size_t cmdCount;

public:
	KeyFrame()		{ reset(); }
	KeyFrame(RawMessage raw);

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;

	void setFrameCount(int fc) { frame = fc; }
	int getFrameCount() const { return frame; }

	const size_t& getCmdCount() const	{ return cmdCount; }
	const NetworkCommand* getCmd(size_t ndx) const { return &commands[ndx]; }

	IF_MAD_SYNC_CHECKS(
		int32 getNextChecksum();
		void addChecksum(int32 cs);
	)
	void add(NetworkCommand &nc);
	void reset();
	void addUpdate(MoveSkillUpdate updt);
	void addUpdate(ProjectileUpdate updt);
	MoveSkillUpdate getMoveUpdate();
	ProjectileUpdate getProjUpdate();
};

#if MAD_SYNC_CHECKING

class SyncErrorMsg : public Message {
	struct Data{
		uint32 messageType :  8;
		uint32 messageSize : 24;
		uint32	frameCount;
	} data;

public:
	SyncErrorMsg(int frame) {
		data.messageType = MessageType::SYNC_ERROR;
		data.messageSize = 4;
		data.frameCount = frame;
	}
	SyncErrorMsg() {}
	SyncErrorMsg(RawMessage raw);

	int getFrame() const { return data.frameCount; }

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
};

#endif


}}//end namespace

#endif
