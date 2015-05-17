// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "network_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"
#include "world.h"

#include "leak_dumper.h"
#include "logger.h"
#include "network_message.h"
#include "script_manager.h"
#include "command.h"
#include "network_connection.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Net {

// =====================================================
//	class NetworkInterface
// =====================================================

NetworkInterface::NetworkInterface(Program &prog) 
		: SimulationInterface(prog) {
	keyFrame.reset();
	Network::init();
}

NetworkInterface::~NetworkInterface() {
	Network::deinit();
}

void NetworkInterface::processTextMessage(TextMessage &msg) {
	if (msg.getTeamIndex() == -1 
	|| msg.getTeamIndex() == g_world.getThisFaction()->getTeam()) {
		chatMessages.push_back(ChatMsg(msg.getText(), msg.getSender(), msg.getColourIndex()));
	}
}

void NetworkInterface::frameProccessed() {
	if (world->getFrameCount() % GameConstants::networkFramePeriod == 0) {
		updateKeyframe(world->getFrameCount());
	}
}

#if MAD_SYNC_CHECKING

void NetworkInterface::postCommandUpdate(Unit *unit) {
	Checksum cs;
	cs.add(unit->getId());
	cs.add(unit->getFactionIndex());
	cs.add(unit->getCurrSkill()->getId());

	int frame = g_world.getFrameCount();

	switch (unit->getCurrSkill()->getClass()) {
		case SkillClass::MOVE:
			cs.add(unit->getNextPos());
			SYNC_LOG( "Cmd:: Frame: " << frame << ", Unit: " << unit->getId() << ", FactionIndex: " << unit->getFactionIndex() 
				<< ", CurrCmdId: " << unit->getCurrCommand()->getId() << ", CurrSkillId: " << unit->getCurrSkill()->getId() << ", NextPos: " << unit->getNextPos() );
			break;
		case SkillClass::ATTACK:
		case SkillClass::REPAIR:
		case SkillClass::BUILD:
			cs.add(unit->getTarget());
			SYNC_LOG( "Cmd:: Frame: " << frame << ", Unit: " << unit->getId() << ", FactionIndex: " << unit->getFactionIndex() 
				<< ", CurrCmdId: " << unit->getCurrCommand()->getId() << ", CurrSkillId: " << unit->getCurrSkill()->getId() << ", TargetUnit: " << unit->getTarget() );
			break;
		default:
			SYNC_LOG( "Cmd:: Frame: " << frame << ", Unit: " << unit->getId() << ", FactionIndex: " << unit->getFactionIndex()
				<< ", CurrCmdId: " << (unit->anyCommand() ? unit->getCurrCommand()->getId() : -1) << ", CurrSkillId: " << unit->getCurrSkill()->getId() );
			break;
	}
	checkCommandUpdate(unit, cs.getSum());
}

void NetworkInterface::postProjectileUpdate(Unit *u, int endFrame) {
	int frame = g_world.getFrameCount();
	Checksum cs;
	cs.add(u->getId());
	cs.add(u->getCurrSkill()->getName());
	if (u->anyCommand()) {
		if (u->getCurrCommand()->getUnit()) {
			cs.add(u->getCurrCommand()->getUnit()->getId());
			SYNC_LOG( "Proj:: Frame: " << frame << ", Unit: " << u->getId() << ", Skill: " << u->getCurrSkill()->getName() 
				<< ", TargetUnit: " << u->getCurrCommand()->getUnit()->getId() << ", EndFrame: " << endFrame );
		} else {
			cs.add(u->getCurrCommand()->getPos());
			SYNC_LOG( "Proj:: Frame: " << frame << ", Unit: " << u->getId() << ", Skill: " << u->getCurrSkill()->getName() 
				<< ", TargetPos: " << u->getCurrCommand()->getPos().x << "," << u->getCurrCommand()->getPos().y << ", EndFrame: " << endFrame );
		}
	} else {
		SYNC_LOG( "Proj:: Frame: " << frame << ", Unit: " << u->getId() << ", Skill: " << u->getCurrSkill()->getName() << ", EndFrame: " << endFrame );
	}
	cs.add(endFrame);
	checkProjectileUpdate(u, endFrame, cs.getSum());
}

void NetworkInterface::postAnimUpdate(Unit *unit) {
	int frame = g_world.getFrameCount();
	if (unit->getSystemStartFrame() != -1) {
		Checksum cs;
		cs.add(unit->getId());
		cs.add(unit->getSystemStartFrame());
		checkAnimUpdate(unit, cs.getSum());
		SYNC_LOG( "Anim:: Frame: " << frame << ", Unit: " << unit->getId() << ", SystemStartFrame: " << unit->getSystemStartFrame() );
	}
}

void NetworkInterface::postUnitBorn(Unit *unit) {
	int frame = g_world.getFrameCount();
	Checksum cs;
	cs.add(unit->getType()->getName());
	cs.add(unit->getId());
	cs.add(unit->getFactionIndex());
	SYNC_LOG( "Born:: Frame: " << frame << ", Type: " << unit->getType()->getName() << ", Unit: " << unit->getId() << ", FactionIndex: " << unit->getFactionIndex() );
	checkUnitBorn(unit, cs.getSum());
}

#endif

}} // end namespace
