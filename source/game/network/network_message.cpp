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
#include "network_message.h"

#include <cassert>
#include <stdexcept>

#include "types.h"
#include "util.h"
#include "game_settings.h"

#include "leak_dumper.h"
#include "logger.h"

#include "tech_tree.h"
#include "type_factories.h"
#include "unit.h"
#include "world.h"
#include "network_interface.h"
#include "network_connection.h"
#include "profiler.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class Message
// =====================================================

void Message::log() const {
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(getType())]
		<< ", messageSize: " << getSize()
	);
}

// =====================================================
//	class IntroMessage
// =====================================================

IntroMessage::IntroMessage(){
	data.messageType = MessageType::INVALID_MSG;
	data.playerIndex = -1;
}

IntroMessage::IntroMessage(const string &versionString, const string &pName, const string &hName, int playerIndex){
	data.messageType = getType();
	data.messageSize = sizeof(Data) - 4;
	data.versionString = versionString;
	data.playerName = pName;
	data.hostName = hName;
	data.playerIndex = static_cast<int16>(playerIndex);
	cout << "IntroMessage constructed, Version string: " << versionString << endl;
}

IntroMessage::IntroMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.versionString, raw.data, raw.size);
	delete raw.data;
	cout << "IntroMessage received, Version string: " << data.versionString.getString() << endl;
}

void IntroMessage::log() const {
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(data.messageType)]
		<< ", messageSize: " << data.messageSize << ", player name: " << data.playerName.getString()
		<< ", host name: " << data.hostName.getString() << ", player index: " << data.playerIndex
	);
}

// =====================================================
//	class ReadyMessage
// =====================================================

ReadyMessage::ReadyMessage() {
	data.messageType = getType();
	data.messageSize = 0;
}

ReadyMessage::ReadyMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	if (raw.size || raw.data) {
		throw GarbledMessage(MessageType::READY);
	}
}

// =====================================================
//	class AiSeedSyncMessage
// =====================================================

AiSeedSyncMessage::AiSeedSyncMessage(){
	data.messageType = MessageType::INVALID_MSG;
	data.messageSize = 0;
	data.seedCount = -1;
}

AiSeedSyncMessage::AiSeedSyncMessage(int count, int32 *seeds) {
	assert(count > 0 && count <= maxAiSeeds);
	data.messageType = getType();
	data.messageSize = sizeof(Data) - 4;
	data.seedCount = count;
	for (int i=0; i < count; ++i) {
		data.seeds[i] = seeds[i];
	}
}

AiSeedSyncMessage::AiSeedSyncMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.seedCount, raw.data, raw.size);
	delete raw.data;
}

// =====================================================
//	class LaunchMessage
// =====================================================

/** Construct empty message (for clients to receive gamesettings into) */
LaunchMessage::LaunchMessage(){
	data.messageType = MessageType::INVALID_MSG;
}

/** Construct from GameSettings (for the server to send out) */
LaunchMessage::LaunchMessage(const GameSettings *gameSettings){
	data.messageType = getType();
	data.messageSize = sizeof(Data) - 4;

	data.description= gameSettings->getDescription();
	data.map= gameSettings->getMapPath();
	data.tileset= gameSettings->getTilesetPath();
	data.tech= gameSettings->getTechPath();
	data.factionCount= gameSettings->getFactionCount();
	data.thisFactionIndex= gameSettings->getThisFactionIndex();
	data.defaultResources= gameSettings->getDefaultResources();
	data.defaultUnits= gameSettings->getDefaultUnits();
	data.defaultVictoryConditions= gameSettings->getDefaultVictoryConditions();
	data.fogOfWar = gameSettings->getFogOfWar();
	data.shroudOfDarkness = gameSettings->getShroudOfDarkness();

	for(int i= 0; i<data.factionCount; ++i){
		data.factionTypeNames[i]= gameSettings->getFactionTypeName(i);
		data.factionControls[i]= gameSettings->getFactionControl(i);
		data.teams[i]= gameSettings->getTeam(i);
		data.startLocationIndex[i]= gameSettings->getStartLocationIndex(i);
		data.colourIndices[i] = gameSettings->getColourIndex(i);
		data.resourceMultipliers[i] = gameSettings->getResourceMultilpier(i);
	}
}

LaunchMessage::LaunchMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.description, raw.data, raw.size);
	delete raw.data;
}

/** Fills in a GameSettings object from this message (for clients) */
void LaunchMessage::buildGameSettings(GameSettings *gameSettings) const{
	gameSettings->setDescription(data.description.getString());
	gameSettings->setMapPath(data.map.getString());
	gameSettings->setTilesetPath(data.tileset.getString());
	gameSettings->setTechPath(data.tech.getString());
	gameSettings->setFactionCount(data.factionCount);
	gameSettings->setThisFactionIndex(data.thisFactionIndex);
	gameSettings->setDefaultResources(data.defaultResources);
	gameSettings->setDefaultUnits(data.defaultUnits);
	gameSettings->setDefaultVictoryConditions(data.defaultVictoryConditions);
	gameSettings->setFogOfWar(data.fogOfWar);
	gameSettings->setShroudOfDarkness(data.shroudOfDarkness);

	for(int i= 0; i<data.factionCount; ++i){
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
		gameSettings->setColourIndex(i, data.colourIndices[i]);
		gameSettings->setResourceMultiplier(i, data.resourceMultipliers[i]);
	}
}

// =====================================================
//	class DataSyncMessage
// =====================================================

DataSyncMessage::DataSyncMessage(RawMessage raw) : m_packetSize(0), m_packetData(0), m_checkSumData(0)
		/*: m_data(0), rawMsg(raw)*/ {
	if (raw.size < 4 * sizeof(int32) && raw.size % sizeof(int32) != 0) {
		throw GarbledMessage(getType(), NetSource::SERVER);
	}
	header.m_cmdTypeCount	  = reinterpret_cast<int32*>(raw.data)[0];
	header.m_skillTypeCount  = reinterpret_cast<int32*>(raw.data)[1];
	header.m_prodTypeCount   = reinterpret_cast<int32*>(raw.data)[2];
	header.m_cloakTypeCount = reinterpret_cast<int32*>(raw.data)[3];

	if (getChecksumCount()) {
		m_checkSumData = reinterpret_cast<int32*>(raw.data) + 4;
	}
}

DataSyncMessage::DataSyncMessage(World &world) : m_packetSize(0), m_packetData(0), m_checkSumData(0) {
	CHECK_HEAP();
	Checksum checksums[4];
	world.getTileset()->doChecksum(checksums[0]);
	NETWORK_LOG(
		"Tilset: " << world.getTileset()->getName() << ", checksum: "
		<< intToHex(checksums[0].getSum())
	);
	world.getMap()->doChecksum(checksums[1]);
	NETWORK_LOG(
		"Map: " << world.getMap()->getName() << ", checksum: "
		<< intToHex(checksums[1].getSum())
	);
	const TechTree *tt = world.getTechTree();
	tt->doChecksumDamageMult(checksums[2]);
	NETWORK_LOG(
		"TechTree: " << tt->getName() << ", Damge Multiplier checksum: "
		<< intToHex(checksums[2].getSum())
	);
	tt->doChecksumResources(checksums[3]);
	NETWORK_LOG(
		"TechTree: " << tt->getName() << ", Resource Types checksum: "
		<< intToHex(checksums[3].getSum())
	);

	header.m_cmdTypeCount	 = g_prototypeFactory.getCommandTypeCount();
	header.m_skillTypeCount = g_prototypeFactory.getSkillTypeCount();
	header.m_prodTypeCount = g_prototypeFactory.getProdTypeCount();
	header.m_cloakTypeCount = g_prototypeFactory.getCloakTypeCount();

	header.messageType = MessageType::DATA_SYNC;
	int msgSize = (sizeof(DataHeader) - 4) + (4 * getChecksumCount());
	header.messageSize = msgSize;

	NETWORK_LOG( "DataSync" );
	NETWORK_LOG( "========" );
	NETWORK_LOG( 
		"CommandType count = " << header.m_cmdTypeCount
		<< ", SkillType count = " << header.m_skillTypeCount 
		<< ", ProdType count = " << header.m_prodTypeCount 
		<< ", CloakType count = " << header.m_cloakTypeCount
	);
	m_checkSumData = new int32[getChecksumCount()];
	for (int i=0; i < 4; ++i) {
		m_checkSumData[i] = checksums[i].getSum();
	}

	int n = 3;
	if (getChecksumCount() - 4 > 0) {
		for (int i=0; i < header.m_cmdTypeCount; ++i) {
			const CommandType *ct = g_prototypeFactory.getCommandType(i);
			m_checkSumData[++n] = g_prototypeFactory.getChecksum(ct);
			NETWORK_LOG(
				"CommandType id:" << ct->getId() << " " << ct->getName() << " of UnitType: "
				<< ct->getUnitType()->getName() << ", checksum[" << (n - 1) << "]: " 
				<< intToHex(m_checkSumData[n - 1])
			);
		}
		for (int i=0; i < header.m_skillTypeCount; ++i) {
			const SkillType *st = g_prototypeFactory.getSkillType(i);
			m_checkSumData[++n] = g_prototypeFactory.getChecksum(st);
			NETWORK_LOG( 
				"SkillType id: " << st->getId() << " " << st->getName() << " of UnitType: "
				<< st->getUnitType()->getName() << ", checksum[" << (n - 1) << "]: "
				<< intToHex(m_checkSumData[n - 1])
			);
		}
		for (int i=0; i < header.m_prodTypeCount; ++i) {
			const ProducibleType *pt = g_prototypeFactory.getProdType(i);
			m_checkSumData[++n] = g_prototypeFactory.getChecksum(pt);
			if (g_prototypeFactory.isUnitType(pt)) {
				const UnitType *ut = static_cast<const UnitType*>(pt);
				NETWORK_LOG(
					"UnitType id: " << ut->getId() << " " << ut->getName() << " of FactionType: " 
					<< ut->getFactionType()->getName() << ", checksum[" << (n - 1) << "]: "
					<< intToHex(m_checkSumData[n - 1])
				);
			} else if (g_prototypeFactory.isUpgradeType(pt)) {
				const UpgradeType *ut = static_cast<const UpgradeType*>(pt);
				NETWORK_LOG( 
					"UpgradeType id: " << ut->getId() << " " << ut->getName() << " of FactionType: "
					<< ut->getFactionType()->getName() << ", checksum[" << (n - 1) << "]: "
					<< intToHex(m_checkSumData[n - 1])
				);
			} else if (g_prototypeFactory.isGeneratedType(pt)) {
				const GeneratedType *gt = static_cast<const GeneratedType*>(pt);
				NETWORK_LOG(
					"GeneratedType id: " << gt->getId() << " " << gt->getName() << " of CommandType: "
					<< gt->getCommandType()->getName() << " of UnitType: " 
					<< gt->getCommandType()->getUnitType()->getName() << ", checksum[" << (n - 1) << "]: "
					<< intToHex(m_checkSumData[n - 1])
				);
			} else {
				throw runtime_error(string("Unknown producible class for type: ") + pt->getName());
			}
		}
		for (int i=0; i < header.m_cloakTypeCount; ++i) {
			const CloakType *ct = g_prototypeFactory.getCloakType(i);
			m_checkSumData[n++] = g_prototypeFactory.getChecksum(ct);
			NETWORK_LOG(
				"CloakType id: " << ct->getId() << ": " << ct->getName() << " of UnitType: "
				<< ct->getUnitType()->getName() << ", checksum[" << (n - 1) << "]: "
				<< intToHex(m_checkSumData[n - 1])
			);
		}
	}
	NETWORK_LOG( "========" );
	CHECK_HEAP();

	m_packetSize = sizeof(DataHeader) + 4 * getChecksumCount();
	m_packetData = new char[m_packetSize];
	memcpy(m_packetData, &header, sizeof(DataHeader));
	memcpy(m_packetData + sizeof(DataHeader), m_checkSumData, 4 * getChecksumCount());
}

void DataSyncMessage::log() const {
	NETWORK_LOG( "DataSyncMessage::log(): message sent, type: " << MessageTypeNames[MessageType(getType())]
		<< ", messageSize: " << getSize() );

	uint8 *ptr = (uint8*)m_packetData;
	string res = "0x";
	for (int i=0; i < 16; ++i) {
		string hex = toHex(int(*(ptr + i)));
		if (hex.size() == 1) {
			hex = "0" + hex;
		}
		res += hex;
	}
	NETWORK_LOG( "DataSyncMessage::log(): first 16 bytes of packet: " << res );
}

/* Put as a specialized send in NetworkConnection?
void DataSyncMessage::send(NetworkConnection* connection) const {
	MsgHeader header;
	header.messageType = getType();
	header.messageSize = sizeof(int32) * (getChecksumCount() + 4);
	connection->send(&header, sizeof(MsgHeader));
	connection->send(&m_cmdTypeCount, sizeof(int32) * 4);
	connection->send(m_data, header.messageSize - sizeof(int32) * 4);
	NETWORK_LOG( __FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(header.messageType)]
		<< ", messageSize: " << header.messageSize
	);
}
*/

// =====================================================
//	class GameSpeedMessage
// =====================================================

GameSpeedMessage::GameSpeedMessage(int frame, GameSpeed setting) {
	data.messageType = getType();
	data.messageSize = sizeof(Data) - sizeof(MsgHeader);
	data.frame = frame;
	data.setting = setting;
}

GameSpeedMessage::GameSpeedMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data + sizeof(MsgHeader), raw.data, raw.size);
	delete raw.data;
}

// =====================================================
//	class NetworkCommandList
// =====================================================

CommandListMessage::CommandListMessage(int32 frameCount) {
	data.messageType = getType();
	data.messageSize = 4;
	data.frameCount = frameCount;
	data.commandCount = 0;
}

bool CommandListMessage::addCommand(const NetworkCommand* networkCommand){
	if (data.commandCount < maxCommandCount) {
		data.commands[data.commandCount++]= *networkCommand;
		data.messageSize += sizeof(NetworkCommand);
		return true;
	}
	return false;
}

CommandListMessage::CommandListMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	uint8* ptr = reinterpret_cast<uint8*>(&data);
	ptr += 4;
	memcpy(ptr, raw.data, raw.size);
	delete raw.data;
}

void CommandListMessage::log() const {
	NETWORK_LOG(
		__FUNCTION__ << "(): message sent, type: " << MessageTypeNames[MessageType(data.messageType)]
		<< ", messageSize: " << data.messageSize << ", number of commands: " << data.commandCount
	);
	if (data.commandCount) {
		NETWORK_LOG( "CommandList Message sent on frame " << g_world.getFrameCount() << ", Command list:" );
		for (int i=0; i < data.commandCount; ++i) {
			data.commands[i].log();
		}
	}
}

// =====================================================
//	class TextMessage
// =====================================================

TextMessage::TextMessage(const string &text, const string &sender, int teamIndex, int colourIndex){
	data.messageType = getType();
	data.messageSize = sizeof(Data) - sizeof(MsgHeader);
	data.text = text;
	data.sender = sender;
	data.teamIndex = teamIndex;
	data.colourIndex = colourIndex;
}

TextMessage::TextMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	memcpy(&data.text, raw.data, raw.size);
	delete raw.data;
}

/*
bool TextMessage::receive(NetworkConnection* connection){
	return connection->receive(&data, sizeof(Data));
}
*/

void TextMessage::log() const {
	NETWORK_LOG( "Sending TextMessage, size: " << data.messageSize );
}

// =====================================================
//	class QuitMessage
// =====================================================

QuitMessage::QuitMessage() {
	data.messageType = getType();
	data.messageSize = 0;
}

QuitMessage::QuitMessage(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	assert(raw.size == 0);
}

/*
bool QuitMessage::receive(NetworkConnection* connection) {
	return connection->receive(&data, sizeof(Data));
}
*/

// =====================================================
//	class KeyFrame
// =====================================================

#pragma pack(push, 1)
struct KeyFrameMsgHeader {
	uint8	cmdCount;
	IF_MAD_SYNC_CHECKS(
		uint16	checksumCount;
	)
	uint16 moveUpdateCount;
	uint16 projUpdateCount;
	uint16 moveUpdateSize;
	uint16 projUpdateSize;
	int32 frame;

};
#pragma pack(pop)

KeyFrame::KeyFrame(RawMessage raw) : m_packetSize(0), m_packetData(0) {
	reset();
	uint8 *ptr = raw.data;
	KeyFrameMsgHeader header;
	memcpy(&header, ptr, sizeof(KeyFrameMsgHeader));
	ptr += sizeof(KeyFrameMsgHeader);

	frame = header.frame;
	IF_MAD_SYNC_CHECKS(
		checksumCount = header.checksumCount;
	)

	m_moveUpdates.updateCount = header.moveUpdateCount;
	m_projectileUpdates.updateCount = header.projUpdateCount;
	m_moveUpdates.updateSize = header.moveUpdateSize;
	m_projectileUpdates.updateSize = header.projUpdateSize;

	cmdCount = header.cmdCount;

	//NETWORK_LOG( "KeyFrame::KeyFrame(RawMessage): Message size: " << raw.size << ", Frame: " << frame << ", Move updates: " << moveUpdateCount
	//	<< ", Projectile updates: " << projUpdateCount << ", Commands: " << cmdCount << ", Checksums: " << checksumCount );

	IF_MAD_SYNC_CHECKS(
		if (checksumCount) {
			memcpy(checksums, ptr, checksumCount * sizeof(int32));
			ptr += checksumCount * sizeof(int32);
		}
	)
	if (m_moveUpdates.updateSize) {
		memcpy(m_moveUpdates.updateBuffer, ptr, m_moveUpdates.updateSize);
		memcpy(m_moveUpdateList, ptr, m_moveUpdates.updateSize);
		ptr += m_moveUpdates.updateSize;
	}
	if (m_projectileUpdates.updateSize) {
		memcpy(m_projectileUpdates.updateBuffer, ptr, m_projectileUpdates.updateSize);
		memcpy(m_projectileUpdateList, ptr, m_projectileUpdates.updateSize);
		ptr += m_projectileUpdates.updateSize;
	}
	if (cmdCount) {
		memcpy(commands, ptr, cmdCount * sizeof(NetworkCommand));
	}
	delete raw.data;
	log();
}

void KeyFrame::buildPacket() {
	if (m_packetData) {
		delete m_packetData;
		m_packetData = 0;
		m_packetSize = 0;
	}
	MsgHeader msgHeader;
	KeyFrameMsgHeader header;

	msgHeader.messageType = getType();

	header.frame = this->frame;
	header.cmdCount = this->cmdCount;
	IF_MAD_SYNC_CHECKS(
		header.checksumCount = this->checksumCount;
	)
	header.moveUpdateCount = m_moveUpdates.updateCount;
	header.projUpdateCount = m_projectileUpdates.updateCount;
	
	header.moveUpdateSize = m_moveUpdates.updateSize;
	header.projUpdateSize = m_projectileUpdates.updateSize;
	size_t commandsSize = header.cmdCount * sizeof(NetworkCommand);

	msgHeader.messageSize = sizeof(KeyFrameMsgHeader) 
		+ m_moveUpdates.updateSize + m_projectileUpdates.updateSize + commandsSize;

	IF_MAD_SYNC_CHECKS(
		msgHeader.messageSize += checksumCount * sizeof(int32);
	)
	size_t totalSize = msgHeader.messageSize + sizeof(MsgHeader);

//	NETWORK_LOG( "KeyFrame::buildPacket(): Message size: " << msgHeader.messageSize << ", Frame: " << frame << ", Move updates: " << moveUpdateCount 
//		<< ", Projectile updates: " << projUpdateCount << ", Commands: " << cmdCount << ", Checksums: " << checksumCount );

	m_packetSize = totalSize;
	m_packetData = new uint8[totalSize];
	uint8 *ptr = (uint8*)m_packetData;
	memcpy(ptr, &msgHeader, sizeof(MsgHeader));
	ptr += sizeof(MsgHeader);
	memcpy(ptr, &header, sizeof(KeyFrameMsgHeader));
	ptr += sizeof(KeyFrameMsgHeader);
	IF_MAD_SYNC_CHECKS(
		if (checksumCount) {
			memcpy(ptr, checksums, checksumCount * sizeof(int32));
			ptr += checksumCount * sizeof(int32);
		}
	)
	if (m_moveUpdates.updateSize) {
		memcpy(ptr, m_moveUpdates.updateBuffer, m_moveUpdates.updateSize);
		ptr += m_moveUpdates.updateSize;
	}
	if (m_projectileUpdates.updateSize) {
		memcpy(ptr, m_projectileUpdates.updateBuffer, m_projectileUpdates.updateSize);
		ptr += m_projectileUpdates.updateSize;
	}
	if (commandsSize) {
		memcpy(ptr, commands, commandsSize);
	}
}

void KeyFrame::log() const {
	if (cmdCount) {
		NETWORK_LOG( "Keyframe " << (frame / GameConstants::networkFramePeriod) << " (frame: " << frame << ") Command list:" );
		for (int i=0; i < cmdCount; ++i) {
			commands[i].log();
		}
	}
}

#if MAD_SYNC_CHECKING

int32 KeyFrame::getNextChecksum() {
	if (checksumCounter >= checksumCount) {
		NETWORK_LOG( "Attempt to retrieve checksum #" << checksumCounter
			<< ", Insufficient checksums in keyFrame. Sync Error."
		);
		throw GameSyncError("Insufficient checksums in keyFrame.");
	}
	return checksums[checksumCounter++];
}

void KeyFrame::addChecksum(int32 cs) {
	if (checksumCount >= max_checksums) {
		throw runtime_error("Error: insufficient room for checksums in keyframe, increase KeyFrame::max_checksums.");
	}
	checksums[checksumCount++] = cs;
}

#endif

void KeyFrame::add(NetworkCommand &nc) {
	assert(cmdCount < max_cmds);
	memcpy(&commands[cmdCount++], &nc, sizeof(NetworkCommand));
}

void KeyFrame::reset() {
	IF_MAD_SYNC_CHECKS(
		checksumCounter = checksumCount = 0;
	)
	cmdCount = 0;
	m_moveIndex = m_projectileIndex = 0;

	m_moveUpdates.updateCount = m_moveUpdates.updateSize = 0;
	m_moveUpdates.writePtr = m_moveUpdates.updateBuffer;
	m_moveUpdates.readPtr = m_moveUpdates.updateBuffer;

	m_projectileUpdates.updateCount = m_projectileUpdates.updateSize = 0;
	m_projectileUpdates.writePtr = m_projectileUpdates.updateBuffer;
	m_projectileUpdates.readPtr = m_projectileUpdates.updateBuffer;
}

void KeyFrame::addUpdate(MoveSkillUpdate updt) {
//	assert(writePtr < &updateBuffer[buffer_size - 2]);
	memcpy(m_moveUpdates.writePtr, &updt, sizeof(MoveSkillUpdate));
	m_moveUpdates.writePtr += sizeof(MoveSkillUpdate);
	m_moveUpdates.updateSize += sizeof(MoveSkillUpdate);
	++m_moveUpdates.updateCount;
	NETWORK_LOG( "KeyFrame::addUpdate(MoveSkillUpdate): Pos Offset:" << updt.posOffset()
		<< " Frame Offset: " << int(updt.end_offset) 
		<< " UnitId: " << updt.unitId
		<< " Update Size: " << m_moveUpdates.updateSize );
}

void KeyFrame::addUpdate(ProjectileUpdate updt) {
//	assert(writePtr < &updateBuffer[buffer_size - 1]);
	memcpy(m_projectileUpdates.writePtr, &updt, sizeof(ProjectileUpdate));
	m_projectileUpdates.writePtr += sizeof(ProjectileUpdate);
	m_projectileUpdates.updateSize += sizeof(ProjectileUpdate);
	++m_projectileUpdates.updateCount;
	//NETWORK_LOG( "KeyFrame::addUpdate(ProjectileUpdate): Frame Offset: " << int(updt.end_offset) );
}

MoveSkillUpdate KeyFrame::getMoveUpdate() {
	assert(m_moveUpdates.readPtr < m_moveUpdates.updateBuffer + m_moveUpdates.updateSize - 1);
	if (!m_moveUpdates.updateCount) {
		NETWORK_LOG( "KeyFrame::getMoveUpdate(): ERROR: Insufficient move skill updates in keyframe." );
		throw GameSyncError("Insufficient move skill updates in keyframe");
	}
	MoveSkillUpdate res = m_moveUpdateList[m_moveIndex];//(m_moveUpdates.readPtr);
	++m_moveIndex;
	//m_moveUpdates.readPtr += sizeof(MoveSkillUpdate);
	--m_moveUpdates.updateCount;
	NETWORK_LOG( "KeyFrame::getMoveUpdate(): Pos Offset:" << res.posOffset()
		<< " Frame Offset: " << int(res.end_offset) 
		<< " Update Size: " << m_moveUpdates.updateSize
		<< " Move Update Count: " << m_moveUpdates.updateCount 
		<< " Frame: " << frame );
	return res;
}

void KeyFrame::logMoveUpdates() {

	int updateCount = m_moveUpdates.updateCount;
	byte_ptr readPtr = m_moveUpdates.readPtr;

	NETWORK_LOG( "KeyFrame::logMoveUpdates(): Update Size: " << m_moveUpdates.updateSize
		<< " Frame: " << frame );

	while (updateCount) {
		MoveSkillUpdate res(readPtr);
		readPtr += sizeof(MoveSkillUpdate);
		--updateCount;

		NETWORK_LOG( "\tPos Offset:" << res.posOffset()
			<< " Frame Offset: " << int(res.end_offset) 
			<< " UnitId: " << res.unitId
			<< " Move Update Count: " << updateCount );
	}
}

ProjectileUpdate KeyFrame::getProjUpdate() {
	assert(m_projectileUpdates.readPtr < m_projectileUpdates.updateBuffer + m_projectileUpdates.updateSize);
	if (!m_projectileUpdates.updateCount) {
		NETWORK_LOG( "KeyFrame::getProjUpdate(): ERROR: Insufficient projectile updates in keyframe." );
		throw GameSyncError("Insufficient projectile updates in keyframe");
	}
	ProjectileUpdate res = m_projectileUpdateList[m_projectileIndex];//(m_projectileUpdates.readPtr);
	++m_projectileIndex;
	//m_projectileUpdates.readPtr += sizeof(ProjectileUpdate);
	--m_projectileUpdates.updateCount;
	//NETWORK_LOG( "KeyFrame::getProjUpdate(): Frame Offset: " << int(res.end_offset) );
	return res;
}

// =====================================================
//	class SkillCycleTableMessage
// =====================================================
/*
SkillCycleTableMessage::SkillCycleTableMessage(Glest::Sim::SkillCycleTable *skillCycleTable) {
	data.messageType = getType();
	data.numEntries = 0;
	data.cycleTable = 0;
}

SkillCycleTableMessage::SkillCycleTableMessage(RawMessage raw) {
	data.numEntries = raw.size / sizeof(CycleInfo);
	data.cycleTable = reinterpret_cast<CycleInfo*>(raw.data);
}

unsigned int SkillCycleTableMessage::getSize() const {
	return sizeof(data) + (sizeof(CycleInfo) * data.numEntries);
}*/

#if MAD_SYNC_CHECKING

SyncErrorMsg::SyncErrorMsg(RawMessage raw) {
	data.messageType = raw.type;
	data.messageSize = raw.size;
	data.frameCount = *reinterpret_cast<uint32*>(raw.data);
	delete raw.data;
}

void SyncErrorMsg::log() const {
	NETWORK_LOG( "SyncErrorMsg::log(): Sync error while processing frame " << data.frameCount );
}

#endif

}}//end namespace
