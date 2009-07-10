// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//               2008-2009 Daniel Santos
//               2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "command_type.h"

#include <algorithm>
#include <cassert>

#include "upgrade_type.h"
#include "unit_type.h"
#include "world.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "unit_updater.h"
#include "renderer.h"
#include "sound_renderer.h"

#include "leak_dumper.h"


using namespace Shared::Util;

namespace Glest { namespace Game {


//FIXME: We check the subfaction in born too.  Should it be removed from there?
//local func to keep players from getting stuff they aren't supposed to.
bool verifySubfaction(Unit *unit, const ProducibleType *pt) {
	if(pt->isAvailableInSubfaction(unit->getFaction()->getSubfaction())) {
		return true;
	} else {
		unit->finishCommand();
		unit->setCurrSkill(scStop);
		unit->getFaction()->deApplyCosts(pt);
		return false;
	}
}

// =====================================================
// 	class AttackSkillTypes & enum AttackSkillPreferenceFlags
// =====================================================


const char* AttackSkillPreferences::names[aspCount] = {
	"whenever-possible",
	"at-max-range",
	"on-large-units",
	"on-buildings",
	"when-damaged"
};

void AttackSkillTypes::init() {
	maxRange = 0;

	assert(types.size() == associatedPrefs.size());
	for(int i = 0; i < types.size(); ++i) {
		if(types[i]->getMaxRange() > maxRange) {
			maxRange = types[i]->getMaxRange();
		}
		zones.flags |= types[i]->getZones().flags;
		allPrefs.flags |= associatedPrefs[i].flags;
	}
}

void AttackSkillTypes::getDesc(string &str, const Unit *unit) const {
	if(types.size() == 1) {
		types[0]->getDesc(str, unit);
	} else {
		str += Lang::getInstance().get("Attacks") + ": ";
		bool printedFirst = false;

		for(int i = 0; i < types.size(); ++i) {
			if(printedFirst) {
				str += ", ";
			}
			str += types[i]->getName();
			printedFirst = true;
		}
		str += "\n";
	}
}

const AttackSkillType *AttackSkillTypes::getPreferredAttack(const Unit *unit,
		const Unit *target, int rangeToTarget) const {
	const AttackSkillType *ast = NULL;

	if(types.size() == 1) {
		ast = types[0];
		return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
	}

	//a skill for use when damaged gets 1st priority.
	if(hasPreference(aspWhenDamaged) && unit->isDamaged()) {
		return getSkillForPref(aspWhenDamaged, rangeToTarget);
	}

	//If a skill in this collection is specified as use whenever possible and
	//the target resides in a field that skill can attack, we will only use that
	//skill if in range and return NULL otherwise.
	if(hasPreference(aspWheneverPossible)) {
		ast = getSkillForPref(aspWheneverPossible, 0);
		assert(ast);
      if ( ast->getZone(target->getCurrZone()) ) 
			return unit->getMaxRange(ast) >= rangeToTarget ? ast : NULL;
		ast = NULL;
	}

	if(hasPreference(aspOnBuilding) && unit->getType()->isOfClass(ucBuilding)) {
		ast = getSkillForPref(aspOnBuilding, rangeToTarget);
	}

	if(!ast && hasPreference(aspOnLarge) && unit->getType()->getSize() > 1) {
		ast = getSkillForPref(aspOnLarge, rangeToTarget);
	}

	//still haven't found an attack skill then use the 1st that's in range
	if(!ast) {
		for(int i = 0; i < types.size(); ++i) {
			if ( unit->getMaxRange ( types[i] ) >= rangeToTarget 
         &&   types[i]->getZone (target->getCurrZone()) ) 
         {
				ast = types[i];
				break;
			}
		}
	}
	return ast;
}

// =====================================================
// 	class CommandType
// =====================================================

int CommandType::nextId = 0;

CommandType::CommandType(const char* name, CommandClass cc, Clicks clicks, bool queuable) :
		RequirableType(getNextId(), name, NULL),
		cc(cc),
		clicks(clicks),
		queuable(queuable),
		unitType(NULL),
		unitTypeIndex(-1) {
}

void CommandType::setUnitTypeAndIndex(const UnitType *unitType, int unitTypeIndex) {
	if(unitType->getId() > UCHAR_MAX) {
		stringstream str;
		str <<  "A maximum of " << UCHAR_MAX << " unit types are currently allowed per faction.  "
				"This limit is only imposed for network data compactness and can be easily changed "
				"if you *really* need that many different unit types. Do you really?";
		throw runtime_error(str.str());
	}

	if(unitType->getId() > UCHAR_MAX) {
		stringstream str;
		str <<  "A maximum of " << UCHAR_MAX << " commands are currently allowed per unit.  "
				"This limit is only imposed for network data compactness and can be easily changed "
				"if you *really* need that many commands. Do you really?";
		throw runtime_error(str.str());
	}

	this->unitType = unitType;
	this->unitTypeIndex = unitTypeIndex;
}

void CommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	name = n->getChildRestrictedValue("name");

	DisplayableType::load(n, dir);
	RequirableType::load(n, dir, tt, ft);
}


// =====================================================
// 	class StopBaseCommandType
// =====================================================

void StopBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	CommandType::load(n, dir, tt, ft);

	//stop
   	string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
	stopSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, scStop));
}

// =====================================================
// 	class StopCommandType
// =====================================================

void StopCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const
{
	if ( unit->getLastCommandUpdate () < 250000) return;

   unit->resetLastCommandUpdated ();
	Command *command= unit->getCurrCommand();
	Unit *sighted = NULL;
	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if(unit->getCommands().size() > 1 ) {
		unit->finishCommand();
		return;
	}

   unit->setCurrSkill( this->getStopSkillType () );

   // check auto commands
   unitUpdater->doAutoCommand ( unit );
}


// =====================================================
// 	class CastSpellCommandType
// =====================================================

void CastSpellCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	MoveBaseCommandType::load(n, dir, tt, ft);

	//cast spell
   	string skillName= n->getChild("cast-spell-skill")->getAttribute("value")->getRestrictedValue();
	castSpellSkillType= static_cast<const CastSpellSkillType*>(unitType->getSkillType(skillName, scCastSpell));
}

void CastSpellCommandType::getDesc(string &str, const Unit *unit) const{
	castSpellSkillType->getDesc(str, unit);
	castSpellSkillType->descSpeed(str, unit, "Speed");

	//movement speed
	MoveBaseCommandType::getDesc(str, unit);
}


// =====================================================
// 	class CommandFactory
// =====================================================

CommandTypeFactory::CommandTypeFactory(){
	registerClass<StopCommandType>("stop");
	registerClass<MoveCommandType>("move");
	registerClass<AttackCommandType>("attack");
	registerClass<AttackStoppedCommandType>("attack_stopped");
	registerClass<BuildCommandType>("build");
	registerClass<HarvestCommandType>("harvest");
	registerClass<RepairCommandType>("repair");
	registerClass<ProduceCommandType>("produce");
	registerClass<UpgradeCommandType>("upgrade");
	registerClass<MorphCommandType>("morph");
	registerClass<CastSpellCommandType>("cast-spell");
	registerClass<GuardCommandType>("guard");
	registerClass<PatrolCommandType>("patrol");
	registerClass<SetMeetingPointCommandType>("set-meeting-point");
   registerClass<DummyCommandType>("dummy");
}

CommandTypeFactory &CommandTypeFactory::getInstance(){
	static CommandTypeFactory ctf;
	return ctf;
}

}}//end namespace
