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

#ifndef _GLEST_GAME_NETWORKMESSAGE_H_
#define _GLEST_GAME_NETWORKMESSAGE_H_

#include "game_constants.h"
#include "network_types.h"
#include "checksum.h"

#include <map>

using std::map;
using Shared::Platform::int8;
using Shared::Platform::int16;
using Shared::Util::Checksum;
using Glest::Sim::GameSpeed;

namespace Glest {
	namespace Sim {
		class SkillCycleTable;
		class CycleInfo;
	}
}

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
  * to implement receive(NetworkConnection*)/send(NetworkConnection*)
  */
class Message {
public:
	virtual ~Message(){}

	virtual MessageType getType() const = 0;
	virtual unsigned int getSize() const = 0;
	virtual const void* getData() const = 0;
	virtual void log() const;
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

	// Implementing Message
	virtual MessageType getType() const		{return MessageType::INTRO;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}
	virtual void log() const override;

	string getVersionString() const		{return data.versionString.getString();}
	string getPlayerName() const		{return data.playerName.getString();}
	string getHostName() const			{return data.hostName.getString();}
	int getPlayerIndex() const			{return data.playerIndex;}
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

	// Implementing Message
	virtual MessageType getType() const		{return MessageType::AI_SYNC;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}

	int getSeedCount() const { return data.seedCount; }
	int32 getSeed(int i) const { return data.seeds[i]; }
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

	virtual MessageType getType() const		{return MessageType::READY;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}
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
		int32 tilesetSeed;
	} data;

public:
	LaunchMessage();
	LaunchMessage(const GameSettings *gameSettings);
	LaunchMessage(RawMessage raw);

	virtual MessageType getType() const		{return MessageType::LAUNCH;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}

	void buildGameSettings(GameSettings *gameSettings) const;
};

// ==============================================================
//	class DataSyncMessage
// ==============================================================

class DataSyncMessage : public Message {
private:
	struct DataHeader {
		uint32 messageType :  8;
		uint32 messageSize : 24;
		int32 m_cmdTypeCount;
		int32 m_skillTypeCount;
		int32 m_prodTypeCount;
		int32 m_cloakTypeCount;
	} header;

	size_t m_packetSize;
	char *m_packetData;

	int32 *m_checkSumData;

public:
	DataSyncMessage(RawMessage raw);
	DataSyncMessage(World &world);

	virtual MessageType getType() const		{return MessageType::DATA_SYNC;}
	virtual unsigned int getSize() const	{return m_packetSize;}
	virtual const void* getData() const		{return m_packetData;}
	virtual void log() const override;

	int32 getChecksum(int i) { m_checkSumData[i]; }

	int32 getCmdTypeCount()	  const { return header.m_cmdTypeCount;   }
	int32 getSkillTypeCount() const { return header.m_skillTypeCount; }
	int32 getProdTypeCount() const { return header.m_prodTypeCount; }
	int32 getCloakTypeCount() const { return header.m_cloakTypeCount; }

	int32 getChecksumCount()  const {
		return header.m_cmdTypeCount + header.m_skillTypeCount + header.m_prodTypeCount + header.m_cloakTypeCount + 4;
	}
};

// ==============================================================
//	class GameSpeedMessage
// ==============================================================

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

	virtual MessageType getType() const		{return MessageType::GAME_SPEED;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}

	int getFrame() const { return data.frame; }
	GameSpeed getSetting() const { return enum_cast<GameSpeed>(data.setting); }
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

	virtual MessageType getType() const     { return MessageType::COMMAND_LIST; }
	virtual unsigned int getSize() const    { return (dataHeaderSize + sizeof(NetworkCommand) * data.commandCount); }
	virtual const void* getData() const     { return &data; }
	virtual void log() const override;

	bool addCommand(const NetworkCommand* networkCommand);
	void clear()									{data.commandCount = 0;}
	int getCommandCount() const						{return data.commandCount;}
	int getFrameCount() const						{return data.frameCount;}
	const NetworkCommand* getCommand(int i) const	{return &data.commands[i];}
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

	virtual MessageType getType() const		{return MessageType::TEXT;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}
	virtual void log() const;

	string getText() const		{return data.text.getString();}
	string getSender() const	{return data.sender.getString();}
	int getTeamIndex() const	{return data.teamIndex;}
	int getColourIndex() const	{return data.colourIndex;}
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

	virtual MessageType getType() const		{return MessageType::QUIT;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}
};

// =====================================================
//	class KeyFrame
// =====================================================
class KeyFrame : public Message {
private:
	static const int buffer_size = 1024 * 4;
	static const int max_cmds = 512;
	static const int max_updates = 128;
	static const int max_checksums = 2048;
	typedef uint8* byte_ptr;

	int32	frame;

	IF_MAD_SYNC_CHECKS(
		int32	checksums[max_checksums];
		int32	checksumCount;
		uint32	checksumCounter;
	)

	struct Updates {
		uint8	 updateBuffer[buffer_size];
		byte_ptr writePtr;
		byte_ptr readPtr;
		size_t	 updateSize;
		uint32	 updateCount;
	};

	int m_moveIndex;
	int m_projectileIndex;

	Updates m_moveUpdates;
	Updates m_projectileUpdates;

	MoveSkillUpdate m_moveUpdateList[max_updates];
	ProjectileUpdate m_projectileUpdateList[max_updates];
	NetworkCommand commands[max_cmds];
	size_t cmdCount;

	int   m_packetSize;
	void *m_packetData;

public:
	KeyFrame() : m_packetSize(0), m_packetData(0) { reset(); }
	KeyFrame(RawMessage raw);

	virtual MessageType getType() const		{return MessageType::KEY_FRAME;}
	virtual unsigned int getSize() const	{return m_packetSize;}
	virtual const void* getData() const		{return m_packetData;}
	virtual void log() const override;
	void logMoveUpdates();

	void setFrameCount(int fc) { frame = fc; }
	int getFrameCount() const { return frame; }

	const size_t& getCmdCount() const	{ return cmdCount; }
	const NetworkCommand* getCmd(size_t ndx) const { return &commands[ndx]; }

	void buildPacket();

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

// =====================================================
//  class SkillCycleTableMessage
//  indexed by SkillType::m_id
// =====================================================
/*
class SkillCycleTableMessage : public Message {
private:
	struct Data {
		uint32 messageType :  8;
		uint32 messageSize : 24;
		uint32 numEntries; //could this be determined by removing header size?
		CycleInfo *cycleTable;
	} data;

public:
	SkillCycleTableMessage(Glest::Sim::SkillCycleTable *skillCycleTable);
	SkillCycleTableMessage(RawMessage raw);

	virtual MessageType getType() const		{return MessageType::SKILL_CYCLE_TABLE;}
	virtual unsigned int getSize() const;
	virtual const void* getData() const		{return &data;}
	//virtual void log() const override {}
};
*/
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

	virtual MessageType getType() const		{return MessageType::SYNC_ERROR;}
	virtual unsigned int getSize() const	{return sizeof(data);}
	virtual const void* getData() const		{return &data;}
	virtual void log() const override;

	int getFrame() const { return data.frameCount; }
};

#endif


}}//end namespace

#endif
