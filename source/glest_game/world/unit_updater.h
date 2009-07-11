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
#include "network_manager.h"
//#include "earthquake.h"

using Shared::Graphics::ParticleObserver;
using Shared::Util::Random;

namespace Glest{ namespace Game{

class Unit;
class Map;
class ScriptManager;

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
   friend class World; // need's access to PathFinder::UpdateMapMetrics()

private:
	/**
	 * When a unit who can repair, but not attack is faced with a hostile, this is the percentage
	 * of the radius that we search from the center of the intersection point for a friendly that
	 * can attack.  This is used to decide if the repiarer stays put in to backup a friendly who
	 * we presume will be fighting, of if the repairer flees.
	 */
	static const float repairerToFriendlySearchRadius;

private:
public:
	const GameCamera *gameCamera;
	Gui *gui;
	Map *map;
	World *world;
	Console *console;
   ScriptManager *scriptManager;
	Random random;
   PathFinder::PathFinder *pathFinder;

public:
   void init(Game &game);

   Map* getMap () const { return map; }
   World* getWorld () const { return world; }

    //update skills
    void updateUnit(Unit *unit);

    //update commands
    void updateUnitCommand(Unit *unit);

    // auto commands
    void doAutoCommand ( Unit *unit );
    void updateAutoCommand ( Unit *unit );

private:
    //attack
    void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Zone targetField, Unit *attacked = NULL);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance);
	void startAttackSystems(Unit *unit, const AttackSkillType* ast);

	//effects
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Zone targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, float distance);
	void appyEffect(Unit *unit, Effect *effect);
	void updateEmanations(Unit *unit);

	//misc
	Command *doAutoAttack(Unit *unit);
	Command *doAutoRepair(Unit *unit);
	Command *doAutoFlee(Unit *unit);

    // If the unit is vaguely between pos1 and pos2, give move command to clear the area
    //void GetClear ( Unit *unit, const Vec2i &pos1, const Vec2i &pos2 );

public:
	bool isLocal()							{return NetworkManager::getInstance().isLocal();}
	bool isNetworkGame()					{return NetworkManager::getInstance().isNetworkGame();}
	bool isNetworkServer() 					{return NetworkManager::getInstance().isNetworkServer();}
	bool isNetworkClient() 					{return NetworkManager::getInstance().isNetworkClient();}
	ServerInterface *getServerInterface()	{return NetworkManager::getInstance().getServerInterface();}

   bool attackerOnSight(const Unit *unit, Unit **enemyPtr) const;
   bool attackableOnSight(const Unit *unit, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past) const;
   bool attackableOnRange(const Unit *unit, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past) const;
	bool unitOnRange(const Unit *unit, int range, Unit **enemyPtr, const AttackSkillTypes *asts, const AttackSkillType **past) const;
	//void enemiesAtDistance(const Unit *unit, const Unit *priorityUnit, int distance, vector<Unit*> &enemies);
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
	Zone targetField;
	UnitReference targetRef;


public:
	ParticleDamager(Unit *attacker, Unit *target, UnitUpdater *unitUpdater, const GameCamera *gameCamera);
	virtual void update(ParticleSystem *particleSystem);
};

}}//end namespace

#endif
