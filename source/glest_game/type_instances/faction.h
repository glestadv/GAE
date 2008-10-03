// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_FACTION_H_
#define _GLEST_GAME_FACTION_H_

#include <vector>
#include <map>
#include <cassert>

#include "upgrade.h"
#include "texture.h"
#include "resource.h"
#include "game_constants.h"
#include "element_type.h"
#include "vec.h"

using std::map;
using std::vector;

namespace Glest{ namespace Game{

using Shared::Graphics::Texture2D;
using Shared::Graphics::Vec3f;

class Unit;
class TechTree;
class FactionType;
class RequirableType;
class CommandType;
class UnitType;
class Game;
class World;

// =====================================================
// 	class Faction
//
///	Each of the game players
// =====================================================

class Faction{
private:
    typedef vector<Resource> Resources;
    typedef vector<Resource> Store;
//	typedef vector<Faction*> Allies;
	typedef vector<Unit*> Units;
	typedef map<int, Unit*> UnitMap;

private:
	UpgradeManager upgradeManager;

    Resources resources;
    Store store;
//	Allies allies;
	Units units;
	UnitMap unitMap;

    ControlType control;

	Texture2D *texture;
	const FactionType *factionType;

	int index;
	int teamIndex;
	int startLocationIndex;

	bool thisFaction;
	int subfaction;			// the current subfaction index starting at zero
	time_t lastAttackNotice;
	time_t lastEnemyNotice;
	Vec3f lastEventLoc;

public:
    void init(
		const FactionType *factionType, ControlType control, TechTree *techTree,
		int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction);
	void end();

    //get
	const Resource *getResource(const ResourceType *rt) const;
	const Resource *getResource(int i) const			{assert(i < resources.size()); return &resources[i];}
	int getStoreAmount(const ResourceType *rt) const;
	const FactionType *getType() const					{return factionType;}
	int getIndex() const								{return index;}
	int getTeam() const									{return teamIndex;}
	bool getCpuControl() const;
	bool getCpuUltraControl() const						{return control==ctCpuUltra;}
	Unit *getUnit(int i) const							{assert(units.size() == unitMap.size()); assert(i < units.size()); return units[i];}
	int getUnitCount() const							{return units.size();}
	const UpgradeManager *getUpgradeManager() const		{return &upgradeManager;}
	const Texture2D *getTexture() const					{return texture;}
	int getStartLocationIndex() const					{return startLocationIndex;}
	int getSubfaction() const							{return subfaction;}
	Vec3f getLastEventLoc() const						{return lastEventLoc;}

	//upgrades
	void startUpgrade(const UpgradeType *ut);
	void cancelUpgrade(const UpgradeType *ut);
	void finishUpgrade(const UpgradeType *ut);

	//cost application
	bool applyCosts(const ProducibleType *p);
	void applyDiscount(const ProducibleType *p, int discount);
	void applyStaticCosts(const ProducibleType *p);
	void applyStaticProduction(const ProducibleType *p);
	void deApplyCosts(const ProducibleType *p);
	void deApplyStaticCosts(const ProducibleType *p);
	void applyCostsOnInterval();
	bool checkCosts(const ProducibleType *pt);

	//reqs
	bool reqsOk(const RequirableType *rt) const;
	bool reqsOk(const CommandType *ct) const;
	bool isCommandAvailable(const CommandType *ct) const;
	bool isCommandAvailable(const RequirableType *rt) const	{return rt->isAvailableInSubfaction(subfaction);}

	//diplomacy
	bool isAlly(const Faction *faction);

    //other
	Unit *findUnit(int id) {
		assert(units.size() == unitMap.size());
		UnitMap::iterator it = unitMap.find(id);
		return it == unitMap.end() ? NULL : it->second;
	}

	void addUnit(Unit *unit);
	void removeUnit(Unit *unit);
	void addStore(const UnitType *unitType);
	void removeStore(const UnitType *unitType);
	void setLastEventLoc(Vec3f lastEventLoc)	{this->lastEventLoc = lastEventLoc;}
	void attackNotice(const Unit *u);
	void advanceSubfaction(int subfaction);

	void checkAdvanceSubfaction(const ProducibleType *pt, bool finished) {
		int advance = pt->getAdvancesToSubfaction();
		if(advance && subfaction < advance) {
			bool immediate = pt->isAdvanceImmediately();
			if(immediate && !finished || !immediate && finished) {
				advanceSubfaction(advance);
			}
		}
	}

	//resources
	void incResourceAmount(const ResourceType *rt, int amount);
	void setResourceBalance(const ResourceType *rt, int balance);

	void load(const XmlNode *node, World *world, const FactionType *ft, ControlType control, TechTree *tt);
//	void reinit(World *world);
	void save(XmlNode *node) const;

private:
	void limitResourcesToStore();
	void resetResourceAmount(const ResourceType *rt);
};

}}//end namespace

#endif
