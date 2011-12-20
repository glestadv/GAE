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
	switch (unit->getCurrSkill()->getClass()) {
		case SkillClass::MOVE:
			//NETWORK_LOG( "NetworkInterface::postCommandUpdate(): unit->getNextPos(): " << unit->getNextPos());
			cs.add(unit->getNextPos());
			break;
		case SkillClass::ATTACK:
		case SkillClass::REPAIR:
		case SkillClass::BUILD:
			if (unit->getTarget()) {
				cs.add(unit->getTarget());
				//NETWORK_LOG( "NetworkInterface::postCommandUpdate(): unit->getTarget(): " << unit->getTarget());
			}
			break;
	}
	//NETWORK_LOG( "NetworkInterface::postCommandUpdate(): UnitID: " << unit->getId()
	//		<< ", FactionIndex: " << unit->getFactionIndex()
	//		<< ", CurrSkillId: " << unit->getCurrSkill()->getId() );
	checkCommandUpdate(unit, cs.getSum());
}

void NetworkInterface::postProjectileUpdate(Unit *u, int endFrame) {
	Checksum cs;
	cs.add(u->getId());
	cs.add(u->getCurrSkill()->getName());
	if (u->anyCommand()) {
		if (u->getCurrCommand()->getUnit()) {
			cs.add(u->getCurrCommand()->getUnit()->getId());
		} else {
			cs.add(u->getCurrCommand()->getPos());
		}
	}
	cs.add(endFrame);
	checkProjectileUpdate(u, endFrame, cs.getSum());
}

void NetworkInterface::postAnimUpdate(Unit *unit) {
	if (unit->getSystemStartFrame() != -1) {
		Checksum cs;
		cs.add(unit->getId());
		cs.add(unit->getSystemStartFrame());
		checkAnimUpdate(unit, cs.getSum());
	}
}

void NetworkInterface::postUnitBorn(Unit *unit) {
	Checksum cs;
	cs.add(unit->getType()->getName());
	cs.add(unit->getId());
	cs.add(unit->getFactionIndex());
	checkUnitBorn(unit, cs.getSum());
}

#endif

}} // end namespace
