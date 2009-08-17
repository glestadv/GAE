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

#ifndef _GLEST_GAME_UNITUPDATER_H_
#define _GLEST_GAME_UNITUPDATER_H_

#include "gui.h"
#include "path_finder.h"
#include "particle.h"
#include "random.h"
#include "network_manager.h"

using Shared::Graphics::ParticleObserver;
using Shared::Util::Random;

namespace Glest{ namespace Game{

class Unit;
class Map;
class ScriptManager;
class ParticleDamager;
namespace Search { class PathFinder; }

// =====================================================
//	class UnitUpdater
//
///	Updates all units in the game, even the player
///	controlled units, performs basic actions only
///	such as responding to an attack
// =====================================================

class UnitUpdater{
private:
	friend class ParticleDamager;
	friend class World;

private:
	const GameCamera *gameCamera;
	Gui *gui;
	Map *map;
	World *world;
	Console *console;
	ScriptManager *scriptManager;
	Search::PathFinder *pathFinder;
	Random random;

public:
    void init(Game &game);

	//update skills
    void updateUnit(Unit *unit);

    //update commands
    void updateUnitCommand(Unit *unit);

private:
	//REFACTOR perform in AttackSkillType::update () ?
    //attack
    void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked = NULL);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, float distance);
	void startAttackSystems(Unit *unit, const AttackSkillType* ast);

	//REFACTOR EffectType::update () ?
	//effects
	//FIXME this needs to take a target zone, not field
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Field targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, float distance);
	void appyEffect(Unit *unit, Effect *effect);
	void updateEmanations(Unit *unit);

	//misc
	bool isLocal()							{return NetworkManager::getInstance().isLocal();}
	bool isNetworkGame()					{return NetworkManager::getInstance().isNetworkGame();}
	bool isNetworkServer() 					{return NetworkManager::getInstance().isNetworkServer();}
	bool isNetworkClient() 					{return NetworkManager::getInstance().isNetworkClient();}
	ServerInterface *getServerInterface()	{return NetworkManager::getInstance().getServerInterface();}
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
	UnitReference targetRef;


public:
	ParticleDamager(Unit *attacker, Unit *target, UnitUpdater *unitUpdater, const GameCamera *gameCamera);
	virtual void update(ParticleSystem *particleSystem);
};

}}//end namespace

#endif
