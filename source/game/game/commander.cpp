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
#include "commander.h"

#include "game.h"
#include "unit.h"
#include "conversion.h"
#include "upgrade.h"
#include "command.h"
#include "command_type.h"
#include "network_manager.h"
#include "console.h"
#include "config.h"
#include "platform_util.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Game {

// =====================================================
// 	class Commander
// =====================================================

// ===================== PUBLIC ========================

CommandResult Commander::tryGiveCommand(
		const Selection &selection,
		CommandFlags flags,
		const CommandType *ct,
		CommandClass cc,
		const Vec2i &pos,
		Unit *targetUnit,
		const UnitType* unitType) const {

	// can't have a target and unit type
	assert(!(unitType && targetUnit));

	// build commands must include position
	assert(!unitType || pos != Command::invalidPos);

	// if a build command, make sure it makes sense
	assert(!unitType || ((ct && ct->getClass() == ccBuild) || cc == ccBuild));

	if(!selection.isEmpty()) {
		Vec2i refPos = computeRefPos(selection);
		CommandResultContainer results;

		//give orders to all selected units
		const Selection::UnitContainer &units = selection.getUnits();
		for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
			const CommandType *effectiveCt;
			if(ct) {
				effectiveCt = ct;
			} else if(cc != ccNull) {
				effectiveCt = (*i)->getFirstAvailableCt(cc);
			} else {
				effectiveCt = (*i)->computeCommandType(pos, targetUnit);
			}
			if(effectiveCt) {
				CommandResult result;
				if(unitType) {
					result = pushCommand(new Command(effectiveCt, flags, pos, unitType, *i));
				} else if(targetUnit) {
					result = pushCommand(new Command(effectiveCt, flags, targetUnit, *i));
				} else if(pos != Command::invalidPos) {
					//every unit is ordered to a different pos
					Vec2i currPos = computeDestPos(refPos, (*i)->getPos(), pos);
					result = pushCommand(new Command(effectiveCt, flags, currPos, *i));
				} else {
					result = pushCommand(new Command(effectiveCt, flags, Command::invalidPos, *i));
				}
				if(result == crSuccess) {
					(*i)->resetLastCommanded();
				}
				results.push_back(result);
			} else {
				results.push_back(crFailUndefined);
			}
		}
		return computeResult(results);
	} else {
		return crFailUndefined;
	}
}

CommandResult Commander::tryCancelCommand(const Selection *selection) const{
	const Selection::UnitContainer &units = selection->getUnits();
	for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
		pushCommand(new Command(CAT_CANCEL_COMMAND, CommandFlags(), Command::invalidPos, *i));
	}

	return crSuccess;
}

void Commander::trySetAutoRepairEnabled(const Selection &selection, CommandFlags flags, bool enabled) const{
	const Selection::UnitContainer &units = selection.getUnits();
	for(Selection::UnitIterator i = units.begin(); i != units.end(); ++i) {
		pushCommand(new Command(CAT_SET_AUTO_REPAIR, CommandFlags(CP_AUTO_REPAIR_ENABLED, enabled),
				Command::invalidPos, *i));
	}
}

// ==================== PRIVATE ====================

Vec2i Commander::computeRefPos(const Selection &selection) const{
	Vec2i total = Vec2i(0);
	for (int i = 0; i < selection.getCount(); ++i) {
		total = total + selection.getUnit(i)->getPos();
	}

	return Vec2i(total.x / selection.getCount(), total.y / selection.getCount());
}

Vec2i Commander::computeDestPos(const Vec2i &refUnitPos, const Vec2i &unitPos, const Vec2i &commandPos) const {
	Vec2i pos;
	Vec2i posDiff = unitPos - refUnitPos;

	if (abs(posDiff.x) >= 3) {
		posDiff.x = posDiff.x % 3;
	}

	if (abs(posDiff.y) >= 3) {
		posDiff.y = posDiff.y % 3;
	}

	pos = commandPos + posDiff;
	game.getWorld()->getMap()->clampPos(pos);
	return pos;
}

CommandResult Commander::computeResult(const CommandResultContainer &results) const {
	switch (results.size()) {
	case 0:
		return crFailUndefined;
	case 1:
		return results.front();
	default: {
			bool anySucceed = false;
			bool anyFail = false;
			bool unique = true;
			foreach(CommandResult cr, results) {
				if (cr != results.front()) {
					unique = false;
				}
				if (cr == crSuccess) {
					anySucceed = true;
				} else {
					anyFail = true;
				}
			}
			if (anySucceed) {
				return anyFail ? crSomeFailed : crSuccess;
			} else {
				return unique ? results.front() : crFailUndefined;
			}
		}
	}
}

CommandResult Commander::pushCommand(Command *command) const {
	assert(command->getCommandedUnit());
	CommandResult result = command->getCommandedUnit()->checkCommand(*command);
	game.getMessenger().postCommand(command);
#if 0
	NetworkManager &netman = NetworkManager::getInstance();
	if(netman.isNetworkGame()) {
		netman.getNetworkMessenger()->requestCommand(command);
	} else {
		giveCommand(command);
	}
#endif
	return result;
}

void Commander::getNetworkData() {
	NetworkManager &netman = NetworkManager::getInstance();
		//FIXME needs total rework
/*
	// on network games, we need to get recently recieved commands from the network interface.
	if(netman.isNetworkGame()) {
		NetworkMessenger *gameInterface = netman.getNetworkMessenger();
		MutexLock lock(gameInterface->getMutex());

		//give pending commands locally
		for(int i = 0; i < gameInterface->getPendingCommandCount(); ++i) {
			giveCommand(gameInterface->getPendingCommand(i));
		}

		gameInterface->clearPendingCommands();
	}*/
}

void Commander::giveCommand(Command *command) const {
	Unit* unit = command->getCommandedUnit();

	//execute command, if unit is still alive and non-deleted
	if(unit && unit->isAlive()) {
		switch(command->getArchetype()) {
			case CAT_GIVE_COMMAND:
				assert(command->getType());
				unit->giveCommand(command);
				break;
			case CAT_CANCEL_COMMAND:
				unit->cancelCommand();
				delete command;
				break;
			case CAT_SET_AUTO_REPAIR:
				unit->setAutoRepairEnabled(command->isAutoRepairEnabled());
				delete command;
				break;
			default:
				assert(false);
		}
	}
}

} // end namespace
