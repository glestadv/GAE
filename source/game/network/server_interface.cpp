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
#include "server_interface.h"

#include "platform_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "world.h"
#include "game.h"
#include "network_types.h"
#include "sim_interface.h"

#include "leak_dumper.h"
#include "logger.h"
#include "profiler.h"

#include "type_factories.h"

using namespace Shared::Platform;
using namespace Shared::Util;

using Glest::Sim::SimulationInterface;

namespace Glest { namespace Net {

// =====================================================
//	class ServerInterface
// =====================================================

ServerInterface::ServerInterface(Program &prog)
		: NetworkInterface(prog)
		, m_portBound(false)
		, m_waitingForPlayers(false)
		, m_dataSync(0)
		, m_syncCounter(0)
		, m_dataSyncDone(false){
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		slots[i] = 0;
	}
}

ServerInterface::~ServerInterface() {
	//quitGame(QuitSource::LOCAL);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		delete slots[i];
	}
}

void ServerInterface::bindPort() {
	try {
		m_connection.bind(g_config.getNetServerPort());
	} catch (runtime_error &e) {
		LOG_NETWORK(e.what());
		throw e;
	}
	m_portBound = true;
}

void ServerInterface::updateSlot(int playerIndex) {
	ConnectionSlot* slot = slots[playerIndex];

	try {
		slot->logNextMessage();
		slot->update();
	} catch (DataSyncError &e) {
		LOG_NETWORK( e.what() );
		throw runtime_error("DataSync Fail : " + slot->getName()
			+ "\n" + e.what());
	} catch (GameSyncError &e) {
		LOG_NETWORK( e.what() );
		removeSlot(playerIndex);
		throw runtime_error(e.what());
	} catch (NetworkError &e) {
		LOG_NETWORK( e.what() );
		string playerName = slot->getName();
		removeSlot(playerIndex);
		sendTextMessage("Player " + intToStr(playerIndex) + " [" + playerName + "] diconnected.", -1);
	}
}

void ServerInterface::addSlot(int playerIndex) {
	assert(playerIndex >= 0 && playerIndex < GameConstants::maxPlayers);
	if (!m_portBound) {
		bindPort();
	}
	NETWORK_LOG( "ServerInterface::addSlot(): Opening slot " << playerIndex );
	delete slots[playerIndex];
	slots[playerIndex] = new ConnectionSlot(this, playerIndex);
	updateListen();
}

void ServerInterface::removeSlot(int playerIndex) {
	assert(m_portBound);
	NETWORK_LOG( "ServerInterface::removeSlot(): Closing slot " << playerIndex );
	delete slots[playerIndex];
	slots[playerIndex] = NULL;
	updateListen();
}

ConnectionSlot* ServerInterface::getSlot(int playerIndex) {
	return slots[playerIndex];
}

int ServerInterface::getConnectedSlotCount() {
	assert(m_portBound);
	int connectedSlotCount = 0;

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i] && slots[i]->isConnected()) {
			++connectedSlotCount;
		}
	}
	return connectedSlotCount;
}

int ServerInterface::getOpenSlotCount() {
	int openSlotCount = 0;

	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && !slots[i]->isConnected()) {
			++openSlotCount;
		}
	}
	return openSlotCount;
}

int ServerInterface::getFreeSlotIndex() {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && !slots[i]->isConnected()) {
			return i;
		}
	}
	return -1;
}

int ServerInterface::getSlotIndexBySession(NetworkSession *session) {
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		if(slots[i] && slots[i]->getSession() == session) {
			return i;
		} 
	}
	return -1;
}

void ServerInterface::update() {
	if (!m_portBound) {
		return;
	}
	// chat messages
	while (hasChatMsg()) {
		Console *c = g_userInterface.getDialogConsole();
		c->addDialog(getChatSender() + ": ", factionColours[getChatColourIndex()],
			getChatText(), true);
		popChatMsg();
	}

	m_connection.update(this);

	// update all slots
	bool allFinishedTurn = true;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		if (slots[i] && slots[i]->isConnected()) {
			updateSlot(i);

			/*if (!slots[i]->isFinishedTurn()) {
				allFinishedTurn = false;
			}*/
		}
	}

	if (allFinishedTurn) {
		//send the current turn commands to all clients
		//execute [current turn - n] queued commands
	}
}

void ServerInterface::doDataSync() {
	m_dataSync = new DataSyncMessage(g_world);
	//broadcastMessage(m_dataSync);

	m_syncCounter = getConnectedSlotCount();
	NETWORK_LOG( "ServerInterface::doDataSync(): DataSyncMessage constructed, waiting for " << m_syncCounter << " message(s) from client(s)." );

	int64 start_time = Chrono::getCurMillis();
	while (!m_dataSyncDone) {
		// update all slots
		update();
		if (Chrono::getCurMillis() - start_time > 25 * 1000) {
			throw TimeOut(NetSource::CLIENT);
		}
		sleep(10);
	}
}

void ServerInterface::dataSync(int playerNdx, DataSyncMessage &msg) {
	NETWORK_LOG( "ServerInterface::dataSync()" );
	assert(m_dataSync);
	--m_syncCounter;
	if (!m_syncCounter) {
		m_dataSyncDone = true;
		NETWORK_LOG( "received DataSyncMessage from client " << playerNdx << ", all dataSync messages received." );
	} else {
		NETWORK_LOG( "received DataSyncMessage from client " << playerNdx << ", awaiting " << m_syncCounter << " more." );
	}
	bool ok = true;
	if (m_dataSync->getChecksumCount() != msg.getChecksumCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getChecksumCount()
			<< " total checksums, I have " << m_dataSync->getChecksumCount() )
		ok = false;
	}
	if (m_dataSync->getCmdTypeCount() != msg.getCmdTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getCmdTypeCount()
			<< " CommandType checksums, I have " << m_dataSync->getCmdTypeCount() )
		ok = false;
	}
	if (m_dataSync->getSkillTypeCount() != msg.getSkillTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getSkillTypeCount()
			<< " SkillType checksums, I have " << m_dataSync->getSkillTypeCount() )
		ok = false;
	}
	if (m_dataSync->getProdTypeCount() != msg.getProdTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getProdTypeCount()
			<< " ProducibleType checksums, I have " << m_dataSync->getProdTypeCount() )
		ok = false;
	}
	if (m_dataSync->getCloakTypeCount() != msg.getCloakTypeCount()) {
		NETWORK_LOG( "DataSync Fail: Client has sent " << msg.getProdTypeCount()
			<< " CloakType checksums, I have " << m_dataSync->getProdTypeCount() )
		ok = false;
	}

	if (!ok) {
		throw DataSyncError(NetSource::SERVER);
	}

	int cmdOffset = 4;
	int skllOffset = cmdOffset + m_prototypeFactory->getCommandTypeCount();
	int prodOffset = skllOffset + m_prototypeFactory->getSkillTypeCount();
	int cloakOffset = prodOffset + m_prototypeFactory->getProdTypeCount();

	const int n = m_dataSync->getChecksumCount();
	for (int i=0; i < n; ++i) {
		if (m_dataSync->getChecksum(i) != msg.getChecksum(i)) {
			NETWORK_LOG(
				"Checksum[" << i << "] mismatch. mine: " << intToHex(m_dataSync->getChecksum(i))
				<< ", client's: " << intToHex(msg.getChecksum(i))
			);
			ok = false;
			if (i < cmdOffset) {
				string badBit;
				switch (i) {
					case 0: badBit = "Tileset"; break;
					case 1: badBit = "Map"; break;
					case 2: badBit = "Damage multiplier"; break;
					case 3: badBit = "Resource"; break;
				}
				NETWORK_LOG( "DataSync Fail: " << badBit << " data does not match." )
			} else if (i < skllOffset) {
				const CommandType *ct = m_prototypeFactory->getCommandType(i - cmdOffset);
				NETWORK_LOG(
					"DataSync Fail: CommandType '" << ct->getName() << "' of UnitType '"
					<< ct->getUnitType()->getName() << "' of FactionType '"
					<< ct->getUnitType()->getFactionType()->getName() << "'";
				)
			} else if (i < prodOffset) {
				const SkillType *skillType = m_prototypeFactory->getSkillType(i - skllOffset);
				NETWORK_LOG(
					"DataSync Fail: SkillType '" << skillType->getName() << "' of UnitType '"
					<< skillType->getUnitType()->getName() << "' of FactionType '"
					<< skillType->getUnitType()->getFactionType()->getName() << "'";
				)
			} else if (i < cloakOffset) {
				const ProducibleType *pt = m_prototypeFactory->getProdType(i - prodOffset);
				//int id = i - prodOffset;
				if (m_prototypeFactory->isUnitType(pt)) {
					const UnitType *ut = static_cast<const UnitType*>(pt);
					NETWORK_LOG( "DataSync Fail: UnitType " << ut->getId() << ": " << ut->getName()
						<< " of FactionType: " << ut->getFactionType()->getName() );
				} else if (m_prototypeFactory->isUpgradeType(pt)) {
					const UpgradeType *ut = static_cast<const UpgradeType*>(pt);
					NETWORK_LOG( "DataSync Fail: UpgradeType " << ut->getId() << ": " << ut->getName()
						<< " of FactionType: " << ut->getFactionType()->getName() );
				} else if (m_prototypeFactory->isGeneratedType(pt)) {
					const GeneratedType *gt = static_cast<const GeneratedType*>(pt);
					NETWORK_LOG( "DataSync Fail: GeneratedType " << gt->getId() << ": " << gt->getName() << " of CommandType: "
						<< gt->getCommandType()->getName() << " of UnitType: "
						<< gt->getCommandType()->getUnitType()->getName() );
				} else {
					throw runtime_error(string("Unknown producible class for type: ") + pt->getName());
				}
			} else {
				const CloakType *ct = m_prototypeFactory->getCloakType(i - cloakOffset);
				NETWORK_LOG(
					"DataSync Fail: CloakType '" << ct->getName() << "' of UnitType '"
					<< ct->getUnitType()->getName() << "' of FactionType '"
					<< ct->getUnitType()->getFactionType()->getName() << "'";
				)
			}
		}
	}
	if (!ok) {
		throw DataSyncError(NetSource::SERVER);
	}
}

void ServerInterface::createSkillCycleTable(const TechTree *techTree) {
	NETWORK_LOG( "ServerInterface::createSkillCycleTable(): Creating and sending SkillCycleTable." );
	SimulationInterface::createSkillCycleTable(techTree);
	//SkillCycleTableMessage skillCycleTableMessage(m_skillCycleTable);
	//broadcastMessage(m_skillCycleTable/*&skillCycleTableMessage*/);
}

#if MAD_SYNC_CHECKING

void ServerInterface::checkUnitBorn(Unit *unit, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkUnitDeath(Unit *unit, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkCommandUpdate(Unit*, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkProjectileUpdate(Unit*, int, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

void ServerInterface::checkAnimUpdate(Unit*, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ << " Adding checksum " << intToHex(cs) );
	keyFrame.addChecksum(cs);
}

#endif

void ServerInterface::updateSkillCycle(Unit *unit) {
	NETWORK_LOG( "UnitId: " << unit->getId() << " IsMoving: " << unit->isMoving());
	SimulationInterface::updateSkillCycle(unit);
	if (unit->isMoving() /*&& !unit->getCurrCommand()->isAuto()*/) {
		MoveSkillUpdate updt(unit);
		keyFrame.addUpdate(updt);
		//NETWORK_LOG( "ServerInterface::updateSkillCycle(): UnitId: " << unit->getId() << " Moving, NextPos: " << unit->getNextPos() );
	}
}

void ServerInterface::updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
	SimulationInterface::updateProjectilePath(u, pps, start, end);
	ProjectileUpdate updt(u, pps);
	keyFrame.addUpdate(updt);
	//string logStart = "ServerInterface::updateProjectilePath(): ProjectileId: " + intToStr(pps->getId());
	//if (pps->getTarget()) {
	//	logStart += ", TargetId: " + intToStr(pps->getTarget()->getId());
	//}
	//NETWORK_LOG( logStart << " startPos: " << start << ", endPos: " << end << ", arrivalOffset: " << updt.end_offset );
}

void ServerInterface::process(TextMessage &msg, int requestor) {
	broadcastMessage(&msg, requestor);
	NetworkInterface::processTextMessage(msg);
}

void ServerInterface::updateKeyframe(int frameCount) {

	update();

	//NETWORK_LOG( "ServerInterface::updateKeyframe(): Building & sending keyframe "
	//	<< (frameCount / GameConstants::networkFramePeriod) << " @ frame " << frameCount);

	// build command list, remove commands from requested and add to pending
	while (!requestedCommands.empty()) {
		keyFrame.add(requestedCommands.back());
		pendingCommands.push_back(requestedCommands.back());
		requestedCommands.pop_back();
	}
	keyFrame.setFrameCount(frameCount);
	keyFrame.buildPacket();
	broadcastMessage(&keyFrame);
	
	keyFrame.reset();
}

void ServerInterface::waitUntilReady() {
	NETWORK_LOG( __FUNCTION__ );
	Chrono chrono;
	chrono.start();
	bool allReady = false;

	// wait until we get a ready message from all clients
	while (!allReady) {
		update();
		allReady = true;
		for (int i = 0; i < GameConstants::maxPlayers; ++i) {
			ConnectionSlot* slot = slots[i];
			if (slot && !slot->isReady()) {
				allReady = false;
				// better to check them all since they will appear
				// not ready if it times out - hailstone 07June2011
				//break;
			}
		}
		// check for timeout
		if (!allReady && chrono.getMillis() > m_connection.getReadyWaitTimeout()) {
			NETWORK_LOG( "ServerInterface::waitUntilReady(): Timed out." );
			for (int i = 0; i < GameConstants::maxPlayers; ++i) {
				if (slots[i] && !slots[i]->isReady()) {
					slots[i]->logNextMessage();
				}
			}
			throw TimeOut(NetSource::CLIENT);
		}
		sleep(2);
	}
	NETWORK_LOG( "ServerInterface::waitUntilReady(): Received all ready messages, sending ready message(s)." );
	ReadyMessage readyMsg;
	broadcastMessage(&readyMsg);
}

void ServerInterface::sendTextMessage(const string &text, int teamIndex){
	NETWORK_LOG( __FUNCTION__ );
	int ci = g_world.getThisFaction()->getColourIndex();
	TextMessage txtMsg(text, Config::getInstance().getNetPlayerName(), teamIndex, ci);
	broadcastMessage(&txtMsg);
	m_connection.flush();
	NetworkInterface::processTextMessage(txtMsg);
}

void ServerInterface::quitGame(QuitSource source) {
	NETWORK_LOG( "ServerInterface::quitGame(): Aborting game." );
	QuitMessage networkMessageQuit;
	broadcastMessage(&networkMessageQuit);
	m_connection.flush();
	if (game && !program.isTerminating()) {
		game->quitGame();
	}
}

// network events
void ServerInterface::onConnect(NetworkSession *session) {
	if (game) {
		session->disconnect(DisconnectReason::IN_GAME);
		return;
	}
	int index = getFreeSlotIndex();
	if (index != -1) {
		slots[index]->setSession(session);

		NETWORK_LOG( "Connection established, slot " << index << " awaiting hello message." );
		//IntroMessage networkMessageIntro(getNetworkVersionString(),
		//	g_config.getNetPlayerName(), m_connection.getHostName(), index);
		//slots[index]->send(&networkMessageIntro);
	} else {
		session->disconnect(DisconnectReason::NO_FREE_SLOTS);
		///@todo add spectator handling code here
	}
}

void ServerInterface::onDisconnect(NetworkSession *session, DisconnectReason reason) {
	int index = getSlotIndexBySession(session);
	if (index != -1) {
		if (this->getGameState()) {
			string playerName = slots[index]->getName();
			removeSlot(index);
			sendTextMessage("Player " + intToStr(index) + " [" + playerName + "] diconnected.", -1);
		} else {
			slots[index]->reset();
		}
	}
}

string ServerInterface::getStatus() const {
	string str;
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		str += intToStr(i) + ": ";
		if (slots[i] && slots[i]->isConnected()) {
				str += slots[i]->getName(); //str += connectionSlot->getRemotePlayerName() + ": " + connectionSlot->getStatus();
		} else {
			str += g_lang.get("NotConnected");
		}
		str += '\n';
	}
	return str;
}

void ServerInterface::syncAiSeeds(int aiCount, int *seeds) {
	NETWORK_LOG( "ServerInterface::syncAiSeeds(): Sending " << aiCount << " Ai random number seeds..." );
	assert(aiCount && seeds);
	SimulationInterface::syncAiSeeds(aiCount, seeds);
	AiSeedSyncMessage seedSyncMsg;
	seedSyncMsg.data().seedCount = aiCount;
	for (int i=0; i < aiCount; ++i) {
		seedSyncMsg.data().seeds[i] = seeds[i];
	}
	broadcastMessage(&seedSyncMsg);
	m_connection.flush();
}

void ServerInterface::doLaunchBroadcast() {
	NETWORK_LOG( "ServerInterface::doLaunchBroadcast(): Launching game." );
	gameSettings.setTilesetSeed(int(Chrono::getCurMillis()));
	gameSettings.setWorldSeed(-int(Chrono::getCurMillis()));
	LaunchMessage networkMessageLaunch(&gameSettings);
	
	broadcastMessage(&networkMessageLaunch);
	m_connection.flush();
}

void ServerInterface::broadcastMessage(const Message* networkMessage, int excludeSlot) {
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		try {
			if (i != excludeSlot && slots[i] && slots[i]->isConnected()) {
				slots[i]->send(networkMessage);
			}
		} catch (Disconnect &d) {
			removeSlot(i);
			if (World::isConstructed()) {
				g_userInterface.getRegularConsole()->addLine(d.what());
			}
		}
	}
}

void ServerInterface::updateListen() {
	m_connection.listen(/*getOpenSlotCount()*/GameConstants::maxPlayers);
}

IF_MAD_SYNC_CHECKS(
	void ServerInterface::dumpFrame(int frame) {
		if (frame > 5) {
			stringstream ss;
			for (int i = 5; i >= 0; --i) {
				worldLog->logFrame(ss, frame - i);
			}
			NETWORK_LOG( ss.str() );
		}
	}
)

}}//end namespace
