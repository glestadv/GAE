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
#include "command_type.h"

#include <algorithm>
#include <cassert>
#include <climits>

#include "upgrade_type.h"
#include "unit_type.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "unit_updater.h"
#include "renderer.h"
#include "world.h"
#include "route_planner.h"

#include "leak_dumper.h"
#include "logger.h"

using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class CommandType
// =====================================================

int CommandType::nextId = 0;

CommandType::CommandType(const char* name, CommandClass cc, Clicks clicks, bool queuable)
		: RequirableType(getNextId(), name, NULL)
		, cc(cc)
		, clicks(clicks)
		, queuable(queuable)
		, unitType(NULL)
		, unitTypeIndex(-1) {
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

bool CommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	name = n->getChildRestrictedValue("name");

	bool ok = DisplayableType::load(n, dir);
	return RequirableType::load(n, dir, tt, ft) && ok;
}

void CommandType::doChecksum(Checksum &checksum) const {
	RequirableType::doChecksum(checksum);
	checksum.add<CommandClass>(cc);
}

typedef const AttackStoppedCommandType* AttackStoppedCmd;
typedef const MoveBaseCommandType* MoveBaseCmdType;
typedef const RepairCommandType* RepairCmd;

///@todo fixme
Command* CommandType::doAutoCommand(Unit *unit) const {
	Command *autoCmd;
	// can we attack any enemy ? ///@todo check all attack commands
	const AttackCommandType *act = unit->getType()->getFirstAttackCommand(Zone::LAND);
	if (act && (autoCmd = act->doAutoAttack(unit))) {
		return autoCmd;
	}
	///@todo check all attack-stopped commands
	AttackStoppedCmd asct = 
		static_cast<AttackStoppedCmd>(unit->getType()->getFirstCtOfClass(CommandClass::ATTACK_STOPPED));
	if (asct && (autoCmd = asct->doAutoAttack(unit))) {
		return autoCmd;
	}
	// can we repair any ally ? ///@todo check all repair commands
	RepairCmd rct = static_cast<RepairCmd>(unit->getType()->getFirstCtOfClass(CommandClass::REPAIR));
	if (rct && (autoCmd = rct->doAutoRepair(unit))) {
		return autoCmd;
	}
	// can we see an enemy we cant attack ? can we run ?
	MoveBaseCmdType mct = 
		static_cast<MoveBaseCmdType>(unit->getType()->getFirstCtOfClass(CommandClass::MOVE));
	if (mct && (autoCmd = mct->doAutoFlee(unit))) {
		return autoCmd;
	}
	return 0;
}


// =====================================================
// 	class MoveBaseCommandType
// =====================================================

bool MoveBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//move
	try { 
		string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		const SkillType *st = unitType->getSkillType(skillName, SkillClass::MOVE);
		moveSkillType= static_cast<const MoveSkillType*>(st);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}
	return loadOk;
}

Command *MoveBaseCommandType::doAutoFlee(Unit *unit) const {
	Unit *sighted = NULL;
	if (attackerInSight(unit, &sighted)) {
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(this, CommandFlags(CommandProperties::AUTO), escapePos);
	}
	return 0;
}


// =====================================================
// 	class MoveCommandType
// =====================================================

void MoveCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const {
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	Vec2i pos;
	if (command->getUnit()) {
		pos = command->getUnit()->getCenteredPos();
		if (!command->getUnit()->isAlive()) {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	} else {
		pos = command->getPos();
	}

	switch (theRoutePlanner.findPath(unit, pos)) {
		case TravelState::MOVING:
			unit->setCurrSkill(moveSkillType);
			unit->face(unit->getNextPos());
			//MOVE_LOG( theWorld.getFrameCount() << "::Unit:" << unit->getId() << " updating move " 
			//	<< "Unit is at " << unit->getPos() << " now moving into " << unit->getNextPos() );
			break;
		case TravelState::BLOCKED:
			unit->setCurrSkill(SkillClass::STOP);
			if (unit->getPath()->isBlocked() && !command->getUnit()) {
				unit->finishCommand();
				return;
			}
			break;	
		default: // TravelState::ARRIVED or TravelState::IMPOSSIBLE
			unit->finishCommand();
			return;
	}

	// if we're doing an auto command, let's make sure we still want to do it
	Command *autoCmd;
	if (command->isAuto() && (autoCmd = doAutoCommand(unit))) {
		unit->giveCommand(autoCmd);
	}
}


// =====================================================
// 	class StopBaseCommandType
// =====================================================

bool StopBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//stop
	try {
		string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
		stopSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, SkillClass::STOP));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		return false;
	}
	return loadOk;
}

// =====================================================
// 	class StopCommandType
// =====================================================

void StopCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const {
	Command *command = unit->getCurrCommand();
	assert(command->getType() == this);

	Unit *sighted = NULL;
	Command *autoCmd;

	// if we have another command then stop sitting on your ass
	if (unit->getCommands().size() > 1) {
		unit->finishCommand();
		UNIT_LOG( theWorld.getFrameCount() << "::Unit:" << unit->getId() << " cancelling stop" );
		return;
	}
	unit->setCurrSkill(stopSkillType);

	// check auto commands
	if (autoCmd = doAutoCommand(unit)) {
		unit->giveCommand(autoCmd);
		return;
	}
}

// =====================================================
// 	class ProduceCommandType
// =====================================================

//varios
bool ProduceCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//produce
	try { 
		string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
		produceSkillType= static_cast<const ProduceSkillType*>(unitType->getSkillType(skillName, SkillClass::PRODUCE));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}

	try { 
		string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
		producedUnit= ft->getUnitType(producedUnitName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void ProduceCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(produceSkillType->getName());
	checksum.add(producedUnit->getName());
}

void ProduceCommandType::getDesc(string &str, const Unit *unit) const {
	produceSkillType->getDesc(str, unit);
	str+= "\n" + getProducedUnit()->getReqDesc();
}

string ProduceCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUnit()->getReqDesc();
}

const ProducibleType *ProduceCommandType::getProduced() const{
	return producedUnit;
}

// =====================================================
// 	class UpgradeCommandType
// =====================================================

bool UpgradeCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){

	bool loadOk = CommandType::load(n, dir, tt, ft);

	//upgrade
	try {
		string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
		upgradeSkillType= static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, SkillClass::UPGRADE));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	try {
		string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
		producedUpgrade= ft->getUpgradeType(producedUpgradeName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void UpgradeCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(upgradeSkillType->getName());
	checksum.add(producedUpgrade->getName());
}

string UpgradeCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUpgrade()->getReqDesc();
}

const ProducibleType *UpgradeCommandType::getProduced() const{
	return producedUpgrade;
}

// =====================================================
// 	class MorphCommandType
// =====================================================

bool MorphCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	//morph skill
	try {
		string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
		morphSkillType= static_cast<const MorphSkillType*>(unitType->getSkillType(skillName, SkillClass::MORPH));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//morph unit
	try {
		string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
		morphUnit= ft->getUnitType(morphUnitName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	//discount
	try { discount= n->getChild("discount")->getAttribute("value")->getIntValue(); }
	catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void MorphCommandType::doChecksum(Checksum &checksum) const {
	CommandType::doChecksum(checksum);
	checksum.add(morphSkillType->getName());
	checksum.add(morphUnit->getName());
	checksum.add<int>(discount);
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	morphSkillType->getDesc(str, unit);

	//discount
	if(discount!=0){
		str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
	}

	str+= "\n"+getProduced()->getReqDesc();
}

string MorphCommandType::getReqDesc() const{
	return RequirableType::getReqDesc() + "\n" + getProduced()->getReqDesc();
}

const ProducibleType *MorphCommandType::getProduced() const{
	return morphUnit;
}

// =====================================================
// 	class CastSpellCommandType
// =====================================================

bool CastSpellCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);

	//cast spell
	try {
		string skillName= n->getChild("cast-spell-skill")->getAttribute("value")->getRestrictedValue();
		castSpellSkillType= static_cast<const CastSpellSkillType*>(unitType->getSkillType(skillName, SkillClass::CAST_SPELL));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what ());
		loadOk = false;
	}
	return loadOk;
}

void CastSpellCommandType::getDesc(string &str, const Unit *unit) const{
	castSpellSkillType->getDesc(str, unit);
	castSpellSkillType->descSpeed(str, unit, "Speed");

	//movement speed
	MoveBaseCommandType::getDesc(str, unit);
}

// Update helpers...

/** Check for enemies unit can smite (or who can smite him)
  * @param rangedPtr [in/out] should point to a pointer that is either NULL or a valid Unit (a target unit)
  * @param asts [in] The attack skill types available to unit
  * @param past [out] Preferred attack skill
  * @return true if the unit has any enemy in range, with results written to rangedPtr and past.
  *  false if no enemies in range, rangedPtr & past will be unchanged.
  */
bool CommandType::unitInRange(const Unit *unit, int range, Unit **rangedPtr, 
					const AttackSkillTypes *asts, const AttackSkillType **past) {
	fixedVec2 fixedCentre = unit->getFixedCenteredPos();
	fixed halfSize = unit->getType()->getHalfSize();
	fixed distance;
	bool needDistance = false;

	if (*rangedPtr && ((*rangedPtr)->isDead() || !asts->getZone((*rangedPtr)->getCurrZone()))) {
		*rangedPtr = NULL;
	}
	if (*rangedPtr) {
		needDistance = true;
	} else {
		Targets enemies;
		Vec2i pos;
		PosCircularIteratorOrdered pci(theWorld.getMap()->getBounds(), unit->getPos(), 
			theWorld.getPosIteratorFactory().getInsideOutIterator(1, range + halfSize.intp()));

		Map *map = theWorld.getMap();
		while (pci.getNext(pos, distance)) {
			foreach_enum (Zone, z) { // all zones
				if (!asts || asts->getZone(z)) { // looking for target in z?
					// does cell contain a bad guy?
					Unit *possibleEnemy = map->getCell(pos)->getUnit(z);
					if (possibleEnemy && possibleEnemy->isAlive() && !unit->isAlly(possibleEnemy)) {
						// If bad guy has an attack command we can short circut this loop now
						if (possibleEnemy->getType()->hasCommandClass(CommandClass::ATTACK)) {
							*rangedPtr = possibleEnemy;
							goto unitOnRange_exitLoop;
						}
						// otherwise, we'll record it and figure out who to slap later.
						enemies.record(possibleEnemy, distance);
					}
				}
			}
		}
	
		if (!enemies.size()) {
			return false;
		}
	
		*rangedPtr = enemies.getNearest();
		needDistance = true;
	}
unitOnRange_exitLoop:
	assert(*rangedPtr);
	
	if (needDistance) {
		const fixed &targetHalfSize = (*rangedPtr)->getType()->getHalfSize();
		distance = fixedCentre.dist((*rangedPtr)->getFixedCenteredPos()) - targetHalfSize;
	}

	// check to see if we like this target.
	if (asts && past) {
		return bool(*past = asts->getPreferredAttack(unit, *rangedPtr, (distance - halfSize).intp()));
	}
	return true;
}

bool CommandType::attackerInSight(const Unit *unit, Unit **rangedPtr) {
	return unitInRange(unit, unit->getSight(), rangedPtr, NULL, NULL);
}

bool CommandType::attackableInRange(const Unit *unit, Unit **rangedPtr, 
			const AttackSkillTypes *asts, const AttackSkillType **past) {
	int range = min(unit->getMaxRange(asts), unit->getSight());
	return unitInRange(unit, range, rangedPtr, asts, past);
}

bool CommandType::attackableInSight(const Unit *unit, Unit **rangedPtr, 
			const AttackSkillTypes *asts, const AttackSkillType **past) {
	return unitInRange(unit, unit->getSight(), rangedPtr, asts, past);
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
}

CommandTypeFactory &CommandTypeFactory::getInstance(){
	static CommandTypeFactory ctf;
	return ctf;
}

//
//REFACTOR: Command updates that haven't been moved yet
//
#define STILL_IN_UNIT_UPDATER(X) \
	void X##CommandType::update(UnitUpdater *unitUpdater, Unit *unit) const {	\
		unitUpdater->update##X(unit);											\
	}
STILL_IN_UNIT_UPDATER( Build )
STILL_IN_UNIT_UPDATER( Harvest )
STILL_IN_UNIT_UPDATER( Produce )
STILL_IN_UNIT_UPDATER( Upgrade )
STILL_IN_UNIT_UPDATER( Morph )
STILL_IN_UNIT_UPDATER( CastSpell )

}}//end namespace