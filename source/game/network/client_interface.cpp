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

#include "pch.h"
#include "client_interface.h"

#include <stdexcept>
#include <cassert>

#include "platform_util.h"
#include "game_util.h"
#include "conversion.h"
#include "config.h"
#include "lang.h"
#include "game.h"
#include "world.h"

#include "random.h"
#include "sim_interface.h"

#include "leak_dumper.h"
#include "logger.h"
#include "profiler.h"
#include "timer.h"

using namespace Glest::Sim;

using namespace Shared::Platform;
using namespace Shared::Util;
using Shared::Platform::Chrono;

namespace Glest { namespace Net {

// =====================================================
//	class ClientInterface
// =====================================================

ClientInterface::ClientInterface(Program &prog)
		: NetworkInterface(prog)
		, m_connection(NULL) 
		, launchGame(false)
		, introDone(false)
		, playerIndex(-1) {
}

ClientInterface::~ClientInterface() {
	if (game || program.isTerminating()) {
		quitGame(QuitSource::LOCAL);
	}
	delete m_connection;
	m_connection = NULL;
}

void ClientInterface::connect(const Ip &ip, int port) {
	NETWORK_LOG( __FUNCTION__ << " connecting to " << ip.getString() << ":" << port );
	delete m_connection;
	m_connection = new ClientConnection();
	m_connection->connect(ip.getString(), port);
	m_connection->update();
}

void ClientInterface::reset() {
	NETWORK_LOG( __FUNCTION__ );
	delete m_connection;
	m_connection = NULL;
}

void ClientInterface::doIntroMessage() {
	NETWORK_LOG( __FUNCTION__ );
	RawMessage msg = m_connection->getNextMessage();
	if (msg.type != MessageType::INTRO) {
		throw InvalidMessage(MessageType::INTRO, msg.type);
	}
	IntroMessage introMsg(msg);
	if (introMsg.getVersionString() != getNetworkVersionString()) {
		throw VersionMismatch(NetSource::CLIENT, getNetworkVersionString(), introMsg.getVersionString());
	}
	playerIndex = introMsg.getPlayerIndex();
	if (playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
		throw GarbledMessage(introMsg.getType(), NetSource::SERVER);
	}
	serverName = introMsg.getHostName();

	m_connection->setRemoteNames(introMsg.getHostName(), introMsg.getPlayerName());
	//send reply
	IntroMessage replyMsg(getNetworkVersionString(), g_config.getNetPlayerName(), m_connection->getHostName(), -1);
	m_connection->send(&replyMsg);
	introDone = true;
}

void ClientInterface::doLaunchMessage() {
	NETWORK_LOG( __FUNCTION__ );
	RawMessage msg = m_connection->getNextMessage();
	if (msg.type != MessageType::LAUNCH) {
		throw InvalidMessage(MessageType::LAUNCH, msg.type);
	}
	LaunchMessage launchMsg(msg);
	GameSettings &gameSettings = g_simInterface.getGameSettings();
	gameSettings.clear();
	launchMsg.buildGameSettings(&gameSettings);
	// replace server player by network
	for (int i=0; i < gameSettings.getFactionCount(); ++i) {
		// replace by network
		if (gameSettings.getFactionControl(i) == ControlType::HUMAN) {
			gameSettings.setFactionControl(i, ControlType::NETWORK);
		}
		// set the faction index
		if (gameSettings.getStartLocationIndex(i) == playerIndex) {
			gameSettings.setThisFactionIndex(i);
			gameSettings.setFactionControl(i, ControlType::HUMAN);
		}
	}
	launchGame = true;
}

void ClientInterface::doDataSync() {
	NETWORK_LOG( __FUNCTION__ );
	DataSyncMessage msg(g_world);
	m_connection->send(&msg);
}

void ClientInterface::createSkillCycleTable(const TechTree *) {
	NETWORK_LOG( __FUNCTION__ << " waiting for server to send Skill Cycle Table." );
	int skillCount = m_prototypeFactory->getSkillTypeCount();
	int expectedSize = skillCount * sizeof(CycleInfo);
	waitForMessage(m_connection->getReadyWaitTimeout());
	RawMessage raw = m_connection->getNextMessage();
	if (raw.type != MessageType::SKILL_CYCLE_TABLE) {
		throw InvalidMessage(MessageType::SKILL_CYCLE_TABLE, raw.type);
	}
	if (raw.size != expectedSize) {
		throw GarbledMessage(MessageType::SKILL_CYCLE_TABLE, NetSource::SERVER);
	}
	//SkillCycleTableMessage skillCycleTableMessage(raw);
	m_skillCycleTable = new SkillCycleTable(/*skillCycleTableMessage*/raw);
}

void ClientInterface::updateLobby() {
	m_connection->update();
	if (!m_connection->hasMessage()) {
		return;
	}
	if (!introDone) {
		doIntroMessage();
	} else {
		doLaunchMessage();
	}
}

void ClientInterface::syncAiSeeds(int aiCount, int *seeds) {
	NETWORK_LOG( __FUNCTION__ );
	waitForMessage(m_connection->getReadyWaitTimeout());
	RawMessage raw = m_connection->getNextMessage();
	if (raw.type != MessageType::AI_SYNC) {
		throw InvalidMessage(MessageType::AI_SYNC, raw.type);
	}
	AiSeedSyncMessage seedSyncMsg(raw);
	assert(aiCount && seeds);
	assert(seedSyncMsg.getSeedCount());
	for (int i=0; i < seedSyncMsg.getSeedCount(); ++i) {
		seeds[i] = seedSyncMsg.getSeed(i);
	}
}

void ClientInterface::waitUntilReady() {
	NETWORK_LOG( __FUNCTION__ );
	ReadyMessage readyMsg;
	m_connection->send(&readyMsg);

	waitForMessage(m_connection->getReadyWaitTimeout());
	RawMessage raw = m_connection->getNextMessage();
	if (raw.type != MessageType::READY) {
		throw InvalidMessage(MessageType::READY, raw.type);
	}

	//delay the start a bit, so clients have more room to get messages
	sleep(GameConstants::networkExtraLatency);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex) {
	int ci = g_world.getThisFaction()->getColourIndex();
	TextMessage textMsg(text, g_config.getNetPlayerName(), teamIndex, ci);
	NetworkInterface::processTextMessage(textMsg);
	m_connection->send(&textMsg);
}

string ClientInterface::getStatus() const {
	return g_lang.get("Server") + ": " + serverName;
}

void ClientInterface::waitForMessage(int timeout) {
	Chrono chrono;
	chrono.start();
	m_connection->update();
	while (true) {
		if (m_connection->hasMessage()) {
			return;
		}
		if (chrono.getMillis() > timeout) {
			throw TimeOut(NetSource::SERVER);
		} else {
			sleep(2);
			m_connection->update();
		}
	}
}

void ClientInterface::quitGame(QuitSource source) {
	NETWORK_LOG( __FUNCTION__ << " QuitSource == " << (source == QuitSource::SERVER ? "SERVER" : "LOCAL") );
	if (m_connection && m_connection->isConnected() && source != QuitSource::SERVER) {
		QuitMessage networkMessageQuit;
		m_connection->send(&networkMessageQuit);
		NETWORK_LOG( "Sent quit message." );
	}

	if (game) {
		if (source == QuitSource::SERVER) {
			throw Net::Disconnect("The server quit the game!");
		}
		game->quitGame();
	}
}

void ClientInterface::startGame() {
	NETWORK_LOG( __FUNCTION__ );
	updateKeyframe(0);
}

void ClientInterface::update() {
	// chat messages
	while (hasChatMsg()) {
		Console *c = g_userInterface.getDialogConsole();
		c->addDialog(getChatSender() + ": ", factionColours[getChatColourIndex()],
			getChatText(), true);
		popChatMsg();
	}

	if (requestedCommands.empty()) {
		return;
	}
	// send as many commands as we can
	CommandListMessage cmdList;
	while (!requestedCommands.empty() && cmdList.addCommand(&requestedCommands.back())) {
		requestedCommands.pop_back();
	}
	if (cmdList.getCommandCount()) {
		m_connection->send(&cmdList);
	}
	m_connection->update();
}

void ClientInterface::updateKeyframe(int frameCount) {
	// give all commands from last KeyFrame
	for (size_t i=0; i < keyFrame.getCmdCount(); ++i) {
		pendingCommands.push_back(*keyFrame.getCmd(i));
	}
	while (true) {
		waitForMessage();
		RawMessage raw = m_connection->getNextMessage();
		if (raw.type == MessageType::KEY_FRAME) {
			keyFrame = KeyFrame(raw);
			NETWORK_LOG( __FUNCTION__ << " received keyframe " << (keyFrame.getFrameCount() / GameConstants::networkFramePeriod)
				<< " @ frame " << frameCount );
			if (keyFrame.getFrameCount() != frameCount + GameConstants::networkFramePeriod) {
				throw GameSyncError("frame count mismatch. Probable garbled message or memory corruption");
			}
			return;
		} else if (raw.type == MessageType::TEXT) {
			TextMessage textMsg(raw);
			NETWORK_LOG( "Received text message from server. Size: " << raw.size << " from: "
				<< textMsg.getSender() << " msg: " << textMsg.getText() );
			NetworkInterface::processTextMessage(textMsg);
		} else if (raw.type == MessageType::QUIT) {
			NETWORK_LOG( "Received quit message from server." );
			QuitMessage quitMsg(raw);
			quitGame(QuitSource::SERVER);
			return;
		} else {
			throw InvalidMessage(MessageType::KEY_FRAME, raw.type);
		}
	}
}

void ClientInterface::updateSkillCycle(Unit *unit) {
//	NETWORK_LOG( __FUNCTION__ );
	if (unit->isMoving()) {
		updateMove(unit);
	} else {
		unit->updateSkillCycle(m_skillCycleTable->lookUp(unit).getSkillFrames());
	}
}

void ClientInterface::updateMove(Unit *unit) {
//	NETWORK_LOG( __FUNCTION__ );
	try{
		MoveSkillUpdate updt = keyFrame.getMoveUpdate();

		if (updt.offsetX < -1 || updt.offsetX > 1 || updt.offsetY < - 1 || updt.offsetY > 1
		|| (!updt.offsetX && !updt.offsetY)) {
			NETWORK_LOG( __FUNCTION__ << " Bad server update, pos offset out of range: " << updt.posOffset() );
			throw GameSyncError("Bad move update"); // msgBox and then graceful exit to Menu please...
		}
		unit->setNextPos(unit->getPos() + Vec2i(updt.offsetX, updt.offsetY));
		unit->updateSkillCycle(updt.end_offset);
	} catch (GameSyncError &e) {
		handleSyncError();
	}
}

void ClientInterface::updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
//	NETWORK_LOG( __FUNCTION__ );
	try {
		ProjectileUpdate updt = keyFrame.getProjUpdate();
		pps->setPath(start, end, updt.end_offset);
	} catch (GameSyncError &e) {
		handleSyncError();
	}
}

#if MAD_SYNC_CHECKING

void ClientInterface::handleSyncError() {
	assert(g_world.getFrameCount());
	int fc = g_world.getFrameCount();
	stringstream ss;
	worldLog->newFrame(fc + 1); // force write of this incomplete frame
	if (fc > 5) { // log last 4 complete frames and this incomplete one
		for (int i = 5; i >= 0; --i) {
			worldLog->logFrame(ss, fc - i);
		}
	}
	NETWORK_LOG( ss.str() );

	SyncErrorMsg se(g_world.getFrameCount());
	m_connection->send(&se); // ask server to also dump a frame log.
	m_connection->update(); // force flush
	throw GameSyncError("Sync error, see glestadv_client.log");
}

void ClientInterface::checkUnitBorn(Unit *unit, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ );
	int32 server_cs = keyFrame.getNextChecksum();
	if (cs != server_cs) {
		NETWORK_LOG( __FUNCTION__ << " Sync Error: unit type: " << unit->getType()->getName()
			<< " unit id: " << unit->getId() << " faction: " << unit->getFactionIndex() );
		NETWORK_LOG( "\tserver checksum " << intToHex(server_cs) << " my checksum " << intToHex(cs) );
		handleSyncError();
	}
}

void ClientInterface::checkCommandUpdate(Unit *unit, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ );
	if (cs != keyFrame.getNextChecksum()) {
		NETWORK_LOG( __FUNCTION__ << " Sync Error: unit type: " << unit->getType()->getName()
			<< ", skill class: " << SkillClassNames[unit->getCurrSkill()->getClass()] );
		handleSyncError();
	}
}

void ClientInterface::checkProjectileUpdate(Unit *unit, int endFrame, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ );
	if (cs != keyFrame.getNextChecksum()) {
		if (unit->getCurrCommand()->getUnit()) {
			NETWORK_LOG( __FUNCTION__ << " Sync Error: unit id: " << unit->getId() << " skill: "
				<< unit->getCurrSkill()->getName() << " target id: "
				<< unit->getCurrCommand()->getUnit()->getId() << " end frame: " << endFrame );
		} else {
			NETWORK_LOG( __FUNCTION__ << " Sync Error: unit id: " << unit->getId() << " skill: "
				<< unit->getCurrSkill()->getName() << " target pos: "
				<< unit->getCurrCommand()->getPos() << " end frame: " << endFrame );
		}
		handleSyncError();
	}
}

void ClientInterface::checkAnimUpdate(Unit *unit, int32 cs) {
//	NETWORK_LOG( __FUNCTION__ );
	if (cs != keyFrame.getNextChecksum()) {
		const CycleInfo &inf = m_skillCycleTable->lookUp(unit);
		NETWORK_LOG( __FUNCTION__ << " Sync Error: unit id: " << unit->getId()
			<< " attack offset: " << inf.getAttackOffset() );
		handleSyncError();
	}
}

#endif

}}//end namespace
