// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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


#include "upgrade_type.h"
#include "world.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "game.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "unit_type.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

// =====================================================
// 	class RepairCommandType
// =====================================================

RepairCommandType::~RepairCommandType(){
}

//REFACTOR ( move to RepairSkillType || RepairCommandType ?? )
const float RepairCommandType::repairerToFriendlySearchRadius = 1.25f;

void RepairCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	MoveBaseCommandType::load(n, dir, tt, ft);

	//repair
   	string skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
	repairSkillType= static_cast<const RepairSkillType*>(unitType->getSkillType(skillName, scRepair));

	//repaired units
	const XmlNode *unitsNode= n->getChild("repaired-units");
	for(int i=0; i<unitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= unitsNode->getChild("unit", i);
		repairableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
	}
}

void RepairCommandType::update(Unit *unit) const
{
	Command *command= unit->getCurrCommand();
   World *world = Game::getInstance()->getWorld ();
   Map *map = world->getMap ();
   PathFinder::PathFinder *pathFinder = PathFinder::PathFinder::getInstance ();
	
	const RepairSkillType *rst = this->getRepairSkillType();
	bool repairThisFrame = unit->getCurrSkill()->getClass() == scRepair;
	Unit *repaired = command->getUnit();

	// If the unit I was supposed to repair died or is already fixed then finish
	if(repaired && (repaired->isDead() || !repaired->isDamaged())) {
		unit->setCurrSkill(scStop);
		unit->finishCommand();
		return;
	}
/*
	if(command->isAuto() && unit->getType()->hasCommandClass(ccAttack)) {
		Command *autoAttackCmd;
		// attacking is 1st priority

		if((autoAttackCmd = doAutoAttack(unit))) {
			// if doAutoAttack found a target, we need to give it the correct starting address
			if(command->hasPos2()) {
				autoAttackCmd->setPos2(command->getPos2());
			}
			unit->giveCommand(autoAttackCmd);
		}
	}*/

	if(repairableOnRange(unit, map, &repaired, this, rst->getMaxRange(), rst->isSelfAllowed())) {
		unit->setTarget(repaired, true, true);
		unit->setCurrSkill(rst);
	} else {
		Vec2i targetPos;
		if(repairableOnSight(unit, map, &repaired, this, rst->isSelfAllowed())) {
			if(!map->getNearestFreePos(targetPos, unit, repaired, 1, rst->getMaxRange())) {
				unit->setCurrSkill(scStop);
				unit->finishCommand();
				return;
			}

			if(targetPos != unit->getTargetPos()) {
				unit->setTargetPos(targetPos);
				unit->getPath()->clear();
			}
		} else {
			// if no more damaged units and on auto command, turn around
			//finishAutoCommand(unit);

			if(command->isAuto() && command->hasPos2()) {
				if(Config::getInstance().getGsAutoReturnEnabled()) {
					command->popPos();
					unit->getPath()->clear();
				} else {
					unit->finishCommand();
				}
			}
			targetPos = command->getPos();
		}

		switch(pathFinder->findPath(unit, targetPos)) {
		case PathFinder::tsArrived:
			if(repaired && unit->getPos() != targetPos) {
				// presume blocked
				unit->setCurrSkill(scStop);
				unit->finishCommand();
				break;
			}
			if(repaired) {
				unit->setCurrSkill(rst);
			} else {
				unit->setCurrSkill(scStop);
				unit->finishCommand();
			}
			break;

		case PathFinder::tsOnTheWay:
			unit->setCurrSkill(this->getMoveSkillType());
			unit->face(unit->getNextPos());
			break;

		case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()){
				unit->setCurrSkill(scStop);
				unit->finishCommand();
			}
			break;
		}
	}

	if(repaired && !repaired->isDamaged()) {
		unit->setCurrSkill(scStop);
		unit->finishCommand();
	}

	if(repairThisFrame && unit->getCurrSkill()->getClass() == scRepair) {
		//if repairing
		if(repaired){
			unit->setTarget(repaired, true, true);
		}

		if(!repaired) {
			unit->setCurrSkill(scStop);
		} else {
			//shiney
         
			if(rst->getSplashParticleSystemType()){
				const Tile *sc= map->getTile(Map::toTileCoords(repaired->getCenteredPos()));
				bool visible= sc->isVisible(world->getThisTeamIndex());

				SplashParticleSystem *psSplash = rst->getSplashParticleSystemType()->createSplashParticleSystem();
				psSplash->setPos(repaired->getCurrVector());
				psSplash->setVisible(visible);
				Renderer::getInstance().manageParticleSystem(psSplash, rsGame);
			}
         
			bool wasBuilt = repaired->isBuilt();

			assert(repaired->isAlive() && repaired->getHp() > 0);

			if(repaired->repair(rst->getAmount(), rst->getMultiplier())) {
				unit->setCurrSkill(scStop);
				if(!wasBuilt) {
					//building finished
               //repaired->born();//FIXME: born() ?!?!?!
               Game::getInstance()->getScriptManager ()->onUnitCreated(repaired);
					if(unit->getFactionIndex() == world->getThisFactionIndex()) {
						// try to find finish build sound
						BuildCommandType *bct = (BuildCommandType *)unit->getType()->getFirstCtOfClass(ccBuild);
						if(bct) {
							SoundRenderer::getInstance().playFx(
								bct->getBuiltSound(),
								unit->getCurrVector(),
                        Game::getInstance ()->getGameCamera ()->getPos());
						}
					}
               NetworkManager &net = NetworkManager::getInstance();
               if( net.isNetworkServer()) {
						net.getServerInterface()->unitUpdate(unit);
						net.getServerInterface()->unitUpdate(repaired);
						net.getServerInterface()->updateFactions();
					}
				}
			}
		}
	}
}


//find a unit we can repair
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool RepairCommandType::repairableOnRange ( const Unit *unit, Map *map, Vec2i center, int centerSize,
		Unit **rangedPtr, const RepairCommandType *rct, const RepairSkillType *rst,
		int range, bool allowSelf, bool militaryOnly, bool damagedOnly) const
{
	Targets repairables;

	Vec2f floatCenter(center.x + centerSize / 2.f, center.y + centerSize / 2.f);
	int targetRange = 0x10000;
	Unit *target = *rangedPtr;
	range += centerSize / 2;

	if(target && target->isAlive() && target->isDamaged() && rct->isRepairableUnitType(target->getType())) {
		float rangeToTarget = floatCenter.dist(target->getFloatCenteredPos()) - (float)(centerSize + target->getSize()) / 2.0f;
		if(rangeToTarget <= range) // current target is good
			return true;
		else
			return false;
	}
	target = NULL;

	//nearby cells
	Vec2i pos;
	float distance;
	PosCircularIteratorSimple pci(*map, center, range);
	while (pci.getNext(pos, distance)) {
		//all fields
		for (int f = 0; f < ZoneCount; f++) {
			Unit *candidate = map->getCell(pos)->getUnit((Zone)f);
	
			//is it a repairable?
			if (candidate
					&& (allowSelf || candidate != unit)
					&& (!rst->isSelfOnly() || candidate == unit)
					&& candidate->isAlive()
					&& unit->isAlly(candidate)
					&& (!rst->isPetOnly() || unit->isPet(candidate))
					&& (!damagedOnly || candidate->isDamaged())
					&& (!militaryOnly || candidate->getType()->hasCommandClass(ccAttack))
					&& rct->isRepairableUnitType(candidate->getType())) {
	
				//record the nearest distance to target (target may be on multiple cells)
				repairables.record(candidate, (int)distance);
			}
		}
	}

	// if no repairables or just one then it's a simple choice.
	if(repairables.empty()) {
		return false;
	} else if(repairables.size() == 1) {
		*rangedPtr = repairables.begin()->first;
		return true;
	}

	//heal cloesest ally that can attack (and are probably fighting) first.
	//if none, go for units that are less than 20%
	//otherwise, take the nearest repairable unit
	if(!(*rangedPtr = repairables.getNearest(scAttack))
			&& !(*rangedPtr = repairables.getNearest(scCount, 0.2f))
			&& !(*rangedPtr= repairables.getNearest())) {
		return false;
	}
	return true;
}

void RepairCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	repairSkillType->getDesc(str, unit);

	str+="\n" + lang.get("CanRepair") + ":\n";
	if(repairSkillType->isSelfOnly()) {
		str += lang.get("SelfOnly");
	} else if(repairSkillType->isPetOnly()) {
		str += lang.get("PetOnly");
	} else {
		for(int i=0; i<repairableUnits.size(); ++i){
			const UnitType *ut = (const UnitType*)repairableUnits[i];
			if(ut->isAvailableInSubfaction(unit->getFaction()->getSubfaction())) {
				str+= ut->getName()+"\n";
			}
		}
	}
}

//get
bool RepairCommandType::isRepairableUnitType(const UnitType *unitType) const{
	for(int i=0; i<repairableUnits.size(); ++i){
		if(static_cast<const UnitType*>(repairableUnits[i])==unitType){
			return true;
		}
	}
	return false;
}

bool RepairCommandType::repairableOnRange ( const Unit *unit, Map *map, Unit **rangedPtr, const RepairCommandType *rct, 
                                           int range, bool allowSelf, bool militaryOnly, 
                                           bool damagedOnly ) const
{
	return repairableOnRange(unit, map, unit->getPos(), unit->getType()->getSize(),
			rangedPtr, rct, rct->getRepairSkillType(), range, allowSelf, militaryOnly, damagedOnly);
}

bool RepairCommandType::repairableOnSight(const Unit *unit, Map *map, Unit **rangedPtr, 
                                          const RepairCommandType *rct, bool allowSelf) const
{
	return repairableOnRange(unit, map, rangedPtr, rct, unit->getSight(), allowSelf);
}


//REFACTOR ( move to RepairCommandType )
Command *RepairCommandType::doAutoRepair(Unit *unit) 
{
   Map *map = Game::getInstance ()->getWorld ()->getMap ();
	if(unit->getType()->hasCommandClass(ccRepair) && unit->isAutoRepairEnabled()) {

		for(int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if(!unit->getFaction()->isAvailable(ct) || ct->getClass() != ccRepair) {
				continue;
			}

			//look a repair skill
			const RepairCommandType *rct = (const RepairCommandType*)ct;
			const RepairSkillType *rst = rct->getRepairSkillType();
			Unit *sighted = NULL;

			if(unit->getEp() >= rst->getEpCost() && rct->repairableOnSight(unit, map, &sighted, rct, rst->isSelfAllowed())) {
				Command *newCommand;
				newCommand = new Command(rct, CommandFlags(cpQueue, cpAuto),
						Map::getNearestPos(unit->getPos(), sighted, rst->getMinRange(), rst->getMaxRange()));
				newCommand->setPos2(unit->getPos());
				return newCommand;
			}
		}
	}
	return NULL;
}


}}

