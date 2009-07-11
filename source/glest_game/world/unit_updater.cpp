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
#include "unit_updater.h"

#include <algorithm>
#include <cassert>
#include <map>

#include "sound.h"
#include "upgrade.h"
#include "unit.h"
#include "particle_type.h"
#include "core_data.h"
#include "config.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "game.h"
#include "path_finder.h"
#include "object.h"
#include "faction.h"
#include "network_manager.h"
#include "command_type.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitUpdater
// =====================================================

// ===================== PUBLIC ========================
const float UnitUpdater::repairerToFriendlySearchRadius = 1.25f;

void UnitUpdater::init(Game &game) {
	this->gui = game.getGui();
	this->gameCamera = game.getGameCamera();
	this->world = game.getWorld();
	this->map = world->getMap();
	this->console = game.getConsole();
   pathFinder = PathFinder::PathFinder::getInstance ();
	pathFinder->init(map);
}

// ==================== progress skills ====================

//skill dependent actions
void UnitUpdater::updateUnit(Unit *unit) {
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	//play skill sound
	const SkillType *currSkill = unit->getCurrSkill();
	if (currSkill->getSound() != NULL) {
		float soundStartTime = currSkill->getSoundStartTime();
		if (soundStartTime >= unit->getLastAnimProgress() && soundStartTime < unit->getAnimProgress()) {
			if (map->getTile(Map::toTileCoords(unit->getPos()))->isVisible(world->getThisTeamIndex())) {
				soundRenderer.playFx(currSkill->getSound(), unit->getCurrVector(), gameCamera->getPos());
			}
		}
	}

	//start attack particle system
	if (unit->getCurrSkill()->getClass() == scAttack) {
		const AttackSkillType *ast = static_cast<const AttackSkillType*>(unit->getCurrSkill());
		float attackStartTime = ast->getStartTime();
		if (attackStartTime >= unit->getLastAnimProgress() && attackStartTime < unit->getAnimProgress()) {
			startAttackSystems(unit, ast);
		}
	}

	//update emanations every 8 frames
	if (unit->getGetEmanations().size() && !((world->getFrameCount() + unit->getId()) % 8)
			&& unit->isOperative()) {
		updateEmanations(unit);
	}

	//update unit
	if (unit->update()) 
   {
		const UnitType *ut = unit->getType();

		if (unit->getCurrSkill()->getClass() == scFallDown) {
			assert(ut->getFirstStOfClass(scGetUp));
			unit->setCurrSkill(scGetUp);
		} else if (unit->getCurrSkill()->getClass() == scGetUp) {
			unit->setCurrSkill(scStop);
		}

		updateUnitCommand(unit);

		//if unit is out of EP, it stops
		if (unit->computeEp()) {
			if (unit->getCurrCommand()) {
				unit->cancelCurrCommand();
			}
			unit->setCurrSkill(scStop);
		}

		//move unit in cells
		if (unit->getCurrSkill()->getClass() == scMove) {
			world->moveUnitCells(unit);

			//play water sound
			if (map->getCell(unit->getPos())->getHeight() < map->getWaterLevel() && unit->getCurrField() == ZoneSurface) {
				soundRenderer.playFx(CoreData::getInstance().getWaterSound());
			}
		}
	}

	//unit death
	if (unit->isDead() && unit->getCurrSkill()->getClass() != scDie) {
		unit->kill();
	}
	map->assertUnitCells(unit);
}

// ==================== progress commands ====================

//VERY IMPORTANT: compute next state depending on the first order of the list
void UnitUpdater::updateUnitCommand(Unit *unit) {
	const SkillType *st = unit->getCurrSkill();

	//commands aren't updated for these skills
	switch(st->getClass()) {
		case scWaitForServer:
		case scFallDown:
		case scGetUp:
			return;

		default:
			break;
	}

	//if unit has command process it
	if(unit->anyCommand()) 
   {
      const CommandType *ct = unit->getCurrCommand()->getType();
      ct->update ( this, unit );
	}

	//if no commands stop and add stop command or guard command for pets
	if(!unit->anyCommand() && unit->isOperative()) {
		const UnitType *ut = unit->getType();
		unit->setCurrSkill(scStop);
		if(unit->getMaster() && ut->hasCommandClass(ccGuard)) {
			unit->giveCommand(new Command(ut->getFirstCtOfClass(ccGuard), CommandFlags(cpAuto), unit->getMaster()));
		} else {
			if(ut->hasCommandClass(ccStop)) {
				unit->giveCommand(new Command(ut->getFirstCtOfClass(ccStop), CommandFlags()));
			}
		}
	}
}
/*
void UnitUpdater::GetClear ( Unit *unit, const Vec2i &pos1, const Vec2i &pos2 )
{
   static const int DistanceToClear = 10; // +/- 5

   Vec2i dir = pos1 - pos2;
   Vec2f perp1 ( dir.y * -1, dir.x );
   Vec2f perp2 ( dir.y, dir.x * -1);
   perp1.normalize (); perp1 = perp1 * DistanceToClear;
   perp2.normalize (); perp2 = perp2 * DistanceToClear;
   Vec2i p1 ( (int)ceil (perp1.x), (int)ceil (perp1.y) );
   Vec2i p2 ( (int)ceil (perp2.x), (int)ceil (perp2.y) );
   Vec2i dest, midPoint;
   midPoint.x = pos1.x > pos2.x ? pos2.x + ( pos1.x - pos2.x ) / 2
                                : pos1.x + ( pos2.x - pos1.x ) / 2;
   midPoint.y = pos1.y > pos2.y ? pos1.y + ( pos2.y - pos1.x ) / 2
                                : pos2.y + ( pos1.y - pos2.y ) / 2;
   if ( map->getNearestFreePos ( dest, unit, midPoint + p1, 1, 5 ) )
      unit->giveCommand ( new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), dest) );
   else if ( map->getNearestFreePos ( dest, unit, midPoint + p2, 1, 5 ) )
      unit->giveCommand ( new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), dest) );
   else
   {
      // look elsewhere ?
   }
   //
}
*/
Command *UnitUpdater::doAutoAttack(Unit *unit) {
	if(unit->getType()->hasCommandClass(ccAttack) || unit->getType()->hasCommandClass(ccAttackStopped)) {

		for(int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if(!unit->getFaction()->isAvailable(ct)) {
				continue;
			}

			//look for an attack skill
			const AttackSkillType *ast = NULL;
			const AttackSkillTypes *asts = NULL;
			Unit *sighted = NULL;

			switch (ct->getClass()) {
			case ccAttack:
				asts = ((const AttackCommandType*)ct)->getAttackSkillTypes();
				break;

			case ccAttackStopped:
				asts = ((const AttackStoppedCommandType*)ct)->getAttackSkillTypes();
				break;

			default:
				break;
			}

			//use it to attack
			if(asts) {
				if( attackableOnSight(unit, &sighted, asts, NULL) ) 
            {
					Command *newCommand = new Command(ct, CommandFlags(cpAuto), sighted->getPos());
					newCommand->setPos2(unit->getPos());
					return newCommand;
				}
			}
		}
	}

	return NULL;
}


Command *UnitUpdater::doAutoRepair(Unit *unit) {
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

Command *UnitUpdater::doAutoFlee(Unit *unit) {
	
	Unit *sighted = NULL;
	if( unit->getType()->hasCommandClass(ccMove) && attackerOnSight(unit, &sighted)) 
   {
		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if(unit->getType()->hasCommandClass(ccRepair)) {
			Vec2f myCenter = unit->getFloatCenteredPos();
			Vec2f signtedCenter = sighted->getFloatCenteredPos();
			Vec2f fcenter = (myCenter + signtedCenter) / 2.f;
			Unit *myHero = NULL;

			// calculate the real distance to hostile by subtracting half of the size of each.
			float actualDistance = myCenter.dist(signtedCenter)
					- (unit->getType()->getSize() + sighted->getType()->getSize()) / 2.f;

			// allow friendly combat unit to be within a radius of 65% of the actual distance to
			// hostile, starting from the half way intersection of the repairer and hostile.
			int searchRadius = (int)roundf(actualDistance * repairerToFriendlySearchRadius / 2.f);
			Vec2i center((int)roundf(fcenter.x), (int)roundf(fcenter.y));

			//try all of our repair commands
			for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
				const CommandType *ct= unit->getType()->getCommandType(i);
				if(ct->getClass() != ccRepair) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if(rct->repairableOnRange(unit, map, center, 1, &myHero, rct, rst, searchRadius, false, true, false)) {
					return NULL;
				}
			}
		}
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), escapePos);
	}
	return NULL;
}

// ==================== Auto-Commands ====================

// doAutoCommand ( unit )
// check if any autocommands can be executed, add command if so
void UnitUpdater::doAutoCommand ( Unit *unit )
{
   Command *autoCmd;
	//we can attack any unit => attack it
	if((autoCmd = doAutoAttack(unit))) {
		unit->giveCommand(autoCmd);
		return;
	}

	//we can repair any ally => repair it
	if((autoCmd = doAutoRepair(unit))) {
		unit->giveCommand(autoCmd);
		return;
	}

	//see any unit and cant attack it => run
	if((autoCmd = doAutoFlee(unit))) {
		unit->giveCommand(autoCmd);
	}
}

// updateAutoCommand ( unit )
// if unit is currently doing an auto command, re-evaluate
void UnitUpdater::updateAutoCommand ( Unit *unit )
{
   if ( unit->anyCommand () && unit->getCurrCommand()->isAuto() )
      doAutoCommand ( unit );
}

void UnitUpdater::updateEmanations(Unit *unit) {
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation;
	for(Emanations::const_iterator i = unit->getGetEmanations().begin();
			i != unit->getGetEmanations().end(); i++) {
		singleEmanation.resize(1);
		singleEmanation[0] = *i;
		applyEffects(unit, singleEmanation, unit->getPos(), ZoneSurface, (*i)->getRadius());
		applyEffects(unit, singleEmanation, unit->getPos(), ZoneAir, (*i)->getRadius());
	}
}

// ==================== PRIVATE ====================

// ==================== attack ====================

void UnitUpdater::hit(Unit *attacker) {
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void UnitUpdater::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Zone targetField, Unit *attacked){

	//hit attack positions
	if(ast->getSplash() && ast->getSplashRadius()) {
		std::map<Unit*, float> hitList;
		std::map<Unit*, float>::iterator i;
		Vec2i pos;
		float distance;

		PosCircularIteratorSimple pci(*map, targetPos, ast->getSplashRadius());
		while(pci.getNext(pos, distance)) {
			attacked = map->getCell(pos)->getUnit(targetField);
			if(attacked && (distance == 0  || ast->getSplashDamageAll() || !attacker->isAlly(attacked))) {
				//float distance = pci.getPos().dist(attacker->getTargetPos());
				damage(attacker, ast, attacked, distance);

				// Remember all units we hit with the closest distance. We only
				// want to hit them with effects once.
				if(ast->isHasEffects()) {
					i = hitList.find(attacked);
					if(i == hitList.end() || i->second > distance) {
						hitList[attacked] = distance;
					}
				}
			}
		}

		if (ast->isHasEffects()) {
			for(i = hitList.begin(); i != hitList.end(); i++) {
				applyEffects(attacker, ast->getEffectTypes(), i->first, i->second);
			}
		}
	} else {
		if(!attacked) {
			attacked= map->getCell(targetPos)->getUnit(targetField);
		}

		if(attacked){
			damage(attacker, ast, attacked, 0.f);
			if(ast->isHasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), attacked, 0.f);
			}
		}
	}
}

void UnitUpdater::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance){

	if(isNetworkClient()) {
		return;
	}

	//get vars
	float damage= (float)attacker->getAttackStrength(ast);
	int var= ast->getAttackVar();
	int armor= attacked->getArmor();
	float damageMultiplier= world->getTechTree()->getDamageMultiplier(ast->getAttackType(),
			attacked->getType()->getArmorType());
	int startingHealth= attacked->getHp();
	int actualDamage;

	//compute damage
	damage+= random.randRange(-var, var);
	damage/= distance + 1.0f;
	damage-= armor;
	damage*= damageMultiplier;
	if(damage<1){
		damage= 1;
	}

	//damage the unit
	if (attacked->decHp(static_cast<int>(damage))) {
		world->doKill(attacker, attacked);
		actualDamage = startingHealth;
	} else {
		actualDamage = (int)roundf(damage);
	}

	//add stolen health to attacker
	if(attacker->getAttackPctStolen(ast) || ast->getAttackPctVar()) {
		float pct = attacker->getAttackPctStolen(ast)
				+ random.randRange(-ast->getAttackPctVar(), ast->getAttackPctVar());
		int stolen = (int)roundf((float)actualDamage * pct);
		if(stolen && attacker->doRegen(stolen, 0)) {
			// stealing a negative percentage and dying?
			world->doKill(attacker, attacker);
		}
	}
	if(isNetworkServer()) {
		getServerInterface()->minorUnitUpdate(attacker);
		getServerInterface()->minorUnitUpdate(attacked);
	}

	//complain
	const Vec3f &attackerVec = attacked->getCurrVector();
	if(!gui->isVisible(Vec2i((int)roundf(attackerVec.x), (int)roundf(attackerVec.y)))) {
		attacked->getFaction()->attackNotice(attacked);
	}
}

void UnitUpdater::startAttackSystems(Unit *unit, const AttackSkillType *ast) {
	Renderer &renderer= Renderer::getInstance();

	ProjectileParticleSystem *psProj = 0;
	SplashParticleSystem *psSplash;

	ParticleSystemTypeProjectile *pstProj= ast->getProjParticleType();
	ParticleSystemTypeSplash *pstSplash= ast->getSplashParticleType();

	Vec3f startPos = unit->getCurrVector();
	Vec3f endPos = unit->getTargetVec();

	//make particle system
	const Tile *sc= map->getTile(Map::toTileCoords(unit->getPos()));
	const Tile *tsc= map->getTile(Map::toTileCoords(unit->getTargetPos()));
	bool visible= sc->isVisible(world->getThisTeamIndex()) || tsc->isVisible(world->getThisTeamIndex());

	//projectile
	if(pstProj!=NULL){
		psProj= pstProj->createProjectileParticleSystem();

		switch(pstProj->getStart()) {
			case ParticleSystemTypeProjectile::psSelf:
				break;

			case ParticleSystemTypeProjectile::psTarget:
				startPos = unit->getTargetVec();
				break;

			case ParticleSystemTypeProjectile::psSky:
				float skyAltitude = 30.f;
				startPos = endPos;
				startPos.x += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
				startPos.y += skyAltitude;
				startPos.z += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
				break;
		}

		psProj->setPath(startPos, endPos);
		if(pstProj->isTracking() && unit->getTarget()) {
			psProj->setTarget(unit->getTarget());
			psProj->setObserver(new ParticleDamager(unit, unit->getTarget(), this, gameCamera));
		} else {
			psProj->setObserver(new ParticleDamager(unit, NULL, this, gameCamera));
		}

		psProj->setVisible(visible);
		renderer.manageParticleSystem(psProj, rsGame);
	}
	else{
		hit(unit);
	}

	//splash
	if(pstSplash!=NULL){
		psSplash= pstSplash->createSplashParticleSystem();
		psSplash->setPos(endPos);
		psSplash->setVisible(visible);
		renderer.manageParticleSystem(psSplash, rsGame);
		if(pstProj!=NULL){
			psProj->link(psSplash);
		}
	}

	const EarthquakeType *et = ast->getEarthquakeType();
	if (et) {
		et->spawn(*map, unit, unit->getTargetPos(), 1.f);
		if (et->getSound()) {
			// play rather visible or not
			SoundRenderer::getInstance().playFx(et->getSound(), unit->getTargetVec(), gameCamera->getPos());
		}
		// FIXME: hacky mechanism of keeping attackers from walking into their own earthquake :(
		unit->finishCommand();
	}
}

// ==================== effects ====================

// Apply effects to a specific location, with or without splash
void UnitUpdater::applyEffects(Unit *source, const EffectTypes &effectTypes,
		const Vec2i &targetPos, Zone targetField, int splashRadius) {

	Unit *target;

	if (splashRadius != 0) {
		std::map<Unit*, float> hitList;
		std::map<Unit*, float>::iterator i;
		Vec2i pos;
		float distance;

		PosCircularIteratorSimple pci(*map, targetPos, splashRadius);
		while (pci.getNext(pos, distance)) {
			target = map->getCell(pos)->getUnit(targetField);
			if (target) {
				//float distance = pci.getPos().dist(targetPos);

				// Remember all units we hit with the closest distance. We only
				// want to hit them once.
				i = hitList.find(target);
				if (i == hitList.end() || (*i).second > distance) {
					hitList[target] = distance;
				}
			}
		}

		for (i = hitList.begin(); i != hitList.end(); i++) {
			applyEffects(source, effectTypes, (*i).first, (*i).second);
		}
	} else {
		target = map->getCell(targetPos)->getUnit(targetField);
		if (target) {
			applyEffects(source, effectTypes, target, 0.f);
		}
	}
}

//apply effects to a specific target
void UnitUpdater::applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *target, float distance) {
	//apply effects
	for (EffectTypes::const_iterator i = effectTypes.begin();
			i != effectTypes.end(); i++) {
		// lots of tests, roughly in order of speed of evaluation.
		if(		// ally/foe test
				(source->isAlly(target)
						? (*i)->isEffectsAlly()
						: (*i)->isEffectsFoe()) &&

				// building/normal unit test
				(target->getType()->isOfClass(ucBuilding)
						? (*i)->isEffectsBuildings()
						: (*i)->isEffectsNormalUnits()) &&

				// pet test
				((*i)->isEffectsPetsOnly()
						? source->isPet(target)
						: true) &&

				// random chance test
				((*i)->getChance() != 100.0f
						? random.randRange(0.0f, 100.0f) < (*i)->getChance()
						: true)) {

			float strength = (*i)->isScaleSplashStrength() ? 1.0f / (float)(distance + 1) : 1.0f;
			Effect *primaryEffect = new Effect((*i), source, NULL, strength, target, world->getTechTree());

			target->add(primaryEffect);

			for (EffectTypes::const_iterator j = (*i)->getRecourse().begin();
					j != (*i)->getRecourse().end(); j++) {
				source->add(new Effect((*j), NULL, primaryEffect, strength, source, world->getTechTree()));
			}
		}
	}
}

void UnitUpdater::appyEffect(Unit *u, Effect *e) {
	if(u->add(e)){
		Unit *attacker = e->getSource();
		if(attacker) {
			world->getStats().kill(attacker->getFactionIndex(), u->getFactionIndex());
			attacker->incKills();
		} else if (e->getRoot()) {
			// if killed by a recourse effect, this was suicide
			world->getStats().kill(u->getFactionIndex(), u->getFactionIndex());
		}
	}
}

bool UnitUpdater::attackerOnSight(const Unit *unit, Unit **rangedPtr) const
{
	int range = unit->getSight();
	return unitOnRange(unit, range, rangedPtr, NULL, NULL);
}

bool UnitUpdater::attackableOnSight(const Unit *unit, Unit **rangedPtr, 
                    const AttackSkillTypes *asts, const AttackSkillType **past) const
{
	int range = unit->getSight();
	return unitOnRange(unit, range, rangedPtr, asts, past);
}

bool UnitUpdater::attackableOnRange(const Unit *unit, Unit **rangedPtr, 
                    const AttackSkillTypes *asts, const AttackSkillType **past) const
{
	// can't attack beyond range of vision
	int range = min(unit->getMaxRange(asts), unit->getSight());
	return unitOnRange(unit, range, rangedPtr, asts, past);
}

//if the unit has any enemy on range
/** rangedPtr should point to a pointer that is either NULL or a valid Unit */
bool UnitUpdater::unitOnRange(const Unit *unit, int range, Unit **rangedPtr, 
                  const AttackSkillTypes *asts, const AttackSkillType **past) const
{
	Vec2f floatCenter = unit->getFloatCenteredPos();
	float halfSize = (float)unit->getType()->getSize() / 2.f;
	float distance;
	bool needDistance = false;

	if(*rangedPtr && (*rangedPtr)->isDead()) {
		*rangedPtr = NULL;
	}

	if(*rangedPtr) {
		needDistance = true;
	} else {
		Targets enemies;
		Vec2i pos;
		PosCircularIteratorOrdered pci(*world->getMap(), unit->getPos(), world->getPosIteratorFactory()
				.getInsideOutIterator(1, (int)roundf(range + halfSize)));

		while (pci.getNext(pos, distance)) {

			//all fields
			for(int k=0; k<ZoneCount; k++){
				Zone z= static_cast<Zone>(k);

				//check field
				if(!asts || asts->getZone(z)) {
               Unit *possibleEnemy= world->getMap()->getCell(pos)->getUnit(z);

					//check enemy
					if(possibleEnemy && possibleEnemy->isAlive() && !unit->isAlly(possibleEnemy)) {
						// If enemy and has an attack command we can short circut this loop now
						if(possibleEnemy->getType()->hasCommandClass(ccAttack)) {
							*rangedPtr = possibleEnemy;
							goto unitOnRange_exitLoop;
						}
						// otherwise, we'll record it and figure out who to slap later.
						enemies.record(possibleEnemy, (int)distance);
					}
				}
			}
		}
	
		if(!enemies.size()) {
			return false;
		}
	
		*rangedPtr = enemies.getNearest();
		needDistance = true;
	}
unitOnRange_exitLoop:
	assert(*rangedPtr);
	
	if(needDistance) {
		float targetHalfSize = (float)(*rangedPtr)->getType()->getSize() / 2.f;
		distance = floatCenter.dist((*rangedPtr)->getFloatCenteredPos()) - targetHalfSize;
	}

	// check to see if we like this target.
	if(asts && past) {
		return (bool)(*past = asts->getPreferredAttack(unit, *rangedPtr, (int)(distance - halfSize)));
	}
	return true;
}

// ==================== misc ====================

/**
 * Finds the nearest position to dest, presuming that dest is the NW corner of
 * a quare destSize cells, that is at least minRange from the target, but no
 * greater than maxRange.
 */
/*
Vec2i UnitUpdater::getNear(const Vec2i &orig, const Vec2i destNW, int destSize, int minRange, int maxRange) {
	if(destSize == 1) {
		return getNear(orig, destNW, minRange, maxRange);
	}

	Vec2f dest;
	int offset = destSize - 1;
	if(orig.x < destNW.x) {
		dest.x = destNW.x;
	} else if (orig.x > destNW.x + offset) {
		dest.x = destNW.x + offset;
	} else {
		dest.x = orig.x;
	}

	if(orig.y < destNW.y) {
		dest.y = destNW.y;
	} else if (orig.y >= destNW.y + offset) {
		dest.y = destNW.y + offset;
	} else {
		dest.y = orig.y;
	}

	return getNear(orig, dest, minRange, maxRange);
}
*/


// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker, Unit *target, UnitUpdater *unitUpdater, const GameCamera *gameCamera){
	this->gameCamera= gameCamera;
	this->attackerRef= attacker;
	this->targetRef = target;
	this->ast= static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos= attacker->getTargetPos();
	this->targetField= attacker->getTargetField();
	this->unitUpdater= unitUpdater;
}

void ParticleDamager::update(ParticleSystem *particleSystem){
	Unit *attacker= attackerRef.getUnit();

	if(attacker) {
		Unit *target = targetRef.getUnit();
		if(target) {
			targetPos = target->getCenteredPos();
			// manually feed the attacked unit here to avoid problems with cell maps and such
			unitUpdater->hit(attacker, ast, targetPos, targetField, target);
		} else {
			unitUpdater->hit(attacker, ast, targetPos, targetField, NULL);
		}

		//play sound
		StaticSound *projSound= ast->getProjSound();
		if(particleSystem->getVisible() && projSound){
			SoundRenderer::getInstance().playFx(projSound, Vec3f(targetPos.x, 0.f, targetPos.y), gameCamera->getPos());
		}
	}
	particleSystem->setObserver(NULL);
	delete this;
}

}}//end namespace
