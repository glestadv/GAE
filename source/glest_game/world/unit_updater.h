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

#ifndef _GLEST_GAME_UNITUPDATER_H_
#define _GLEST_GAME_UNITUPDATER_H_

#include "gui.h"
#include "path_finder.h"
#include "particle.h"
#include "random.h"

using Shared::Graphics::ParticleObserver;
using Shared::Util::Random;

namespace Glest{ namespace Game{

class Unit;
class Map;

// =====================================================
//	class UnitUpdater
//
///	Updates all units in the game, even the player
///	controlled units, performs basic actions only
///	such as responding to an attack
// =====================================================

class ParticleDamager;

class UnitUpdater{
private:
	friend class ParticleDamager;

private:
	static const int maxResSearchRadius= 10;
	static const int harvestDistance= 5;
	static const int ultraResourceFactor= 3;

private:
	const GameCamera *gameCamera;
	Gui *gui;
	Map *map;
	World *world;
	Console *console;
	PathFinder pathFinder;
	Random random;

public:
    void init(Game *game);

	//update skills
    void updateUnit(Unit *unit);

    //update commands
    void updateUnitCommand(Unit *unit);
    void updateStop(Unit *unit);
    void updateMove(Unit *unit);
    void updateAttack(Unit *unit);
    void updateAttackStopped(Unit *unit);
    void updateBuild(Unit *unit);
    void updateHarvest(Unit *unit);
    void updateRepair(Unit *unit);
    void updateProduce(Unit *unit);
    void updateUpgrade(Unit *unit);
	void updateMorph(Unit *unit);
	void updateCastSpell(Unit *unit);
	void updateGuard(Unit *unit);
	void updatePatrol(Unit *unit);

private:
    //attack
    void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance);
	void startAttackParticleSystem(Unit *unit);

	//effects
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Field targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, float distance);
	void appyEffect(Unit *unit, Effect *effect);
	void updateEmanations(Unit *unit);

	//misc
    bool searchForResource(Unit *unit, const HarvestCommandType *hct);
    bool attackerOnSight(const Unit *unit, Unit **enemyPtr);
    bool attackableOnSight(const Unit *unit, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past);
    bool attackableOnRange(const Unit *unit, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past);
	bool unitOnRange(const Unit *unit, int range, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past);
	bool repairableOnRange(
			const Unit *unit,
			Vec2i center,
			int centerSize,
			Unit **rangedPtr,
			const RepairCommandType *rct,
			const RepairSkillType *rst,
			int range,
			bool allowSelf = false,
			bool militaryOnly = false,
			bool damagedOnly = true);

	bool repairableOnRange(
			const Unit *unit,
			Unit **rangedPtr,
			const RepairCommandType *rct,
			int range,
			bool allowSelf = false,
			bool militaryOnly = false,
			bool damagedOnly = true) {
		return repairableOnRange(unit, unit->getPos(), unit->getType()->getSize(),
				rangedPtr, rct, rct->getRepairSkillType(), range, allowSelf, militaryOnly, damagedOnly);
	}

	bool repairableOnSight(const Unit *unit, Unit **rangedPtr, const RepairCommandType *rct, bool allowSelf) {
		return repairableOnRange(unit, rangedPtr, rct, unit->getSight(), allowSelf);
	}

	void enemiesAtDistance(const Unit *unit, const Unit *priorityUnit, int distance, vector<Unit*> &enemies);
	bool updateAttackGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos);
	Vec2i getNear(const Vec2i &orig, Vec2i dest, int minRange, int maxRange, int destSize = 1);
	Vec2i getNear(const Vec2i &orig, const Unit *destUnit, int minRange, int maxRange) {
		return getNear(orig, destUnit->getPos(), minRange, maxRange, destUnit->getType()->getSize());
	}
};

// =====================================================
//	class ParticleDamager
// =====================================================

class ParticleDamager: public ParticleObserver{
public:
	UnitReference attackerRef;
	const AttackSkillType* ast;
	UnitUpdater *unitUpdater;
	const GameCamera *gameCamera;
	Vec2i targetPos;
	Field targetField;

public:
	ParticleDamager(Unit *attacker, UnitUpdater *unitUpdater, const GameCamera *gameCamera);
	virtual void update(ParticleSystem *particleSystem);
};

}}//end namespace

#endif
