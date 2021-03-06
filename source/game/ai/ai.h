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

#ifndef _GLEST_GAME_AI_H_
#define _GLEST_GAME_AI_H_

#include <vector>
#include <list>

#include "world.h"
#include "commander.h"
#include "command.h"
#include "random.h"

using std::deque;
using std::vector;
using std::list;
using Shared::Util::Random;

namespace Glest { namespace Plan {

class GlestAiInterface;
class AiRule;

// =====================================================
// 	class Task
//
///	An action that has to be performed by the AI
// =====================================================

WRAPPED_ENUM( TaskClass, PRODUCE, BUILD, UPGRADE )

class Task{
protected:
	TaskClass taskClass;

public:
	Task(TaskClass taskClass) : taskClass(taskClass) {}
	virtual ~Task(){}
	TaskClass getClass() const	{return taskClass;}
	virtual string toString() const= 0;

	MEMORY_CHECK_DECLARATIONS(Task)
};

// ==================== ProduceTask ====================

class ProduceTask: public Task{
private:
	UnitClass unitClass;
	const ResourceType *resourceType;
	const UnitType *unitType;

public:
	ProduceTask(UnitClass unitClass);
	ProduceTask(const ResourceType *resourceType);
	ProduceTask(const UnitType *unitType);

	UnitClass getUnitClass() const					{return unitClass;}
	const ResourceType *getResourceType() const		{return resourceType;}
	const UnitType *getUnitType() const				{return unitType;}
	virtual string toString() const;
};

// ==================== BuildTask ====================

class BuildTask: public Task{
private:
	const UnitType *unitType;
	const ResourceType *resourceType;
	bool forcePos;
	Vec2i pos;

public:
	BuildTask(const UnitType *unitType= NULL);
	BuildTask(const ResourceType *resourceType);
	BuildTask(const UnitType *unitType, const Vec2i &pos);

	const UnitType *getUnitType() const			{return unitType;}
	const ResourceType *getResourceType() const	{return resourceType;}
	bool getForcePos() const					{return forcePos;}
	Vec2i getPos() const						{return pos;}
	virtual string toString() const;
};

// ==================== UpgradeTask ====================

class UpgradeTask: public Task{
private:
	const UpgradeType *upgradeType;

public:
	UpgradeTask(const UpgradeType *upgradeType= NULL);
	const UpgradeType *getUpgradeType() const	{return upgradeType;}
	virtual string toString() const;
};

// ===============================
// 	class AI
//
///	Main AI class
// ===============================

class Ai{
private:
	static const int harvesterPercent= 30;
	static const int maxBuildRadius= 40;
	static const int minMinWarriors= 7;
	static const int maxMinWarriors= 20;
	static const int minStaticResources= 10;
	static const int minConsumableResources= 20;
	static const int maxExpansions= 2;
	static const int villageRadius= 15;

public:
	enum ResourceUsage{
		ruHarvester,
		ruWarrior,
		ruBuilding,
		ruUpgrade
	};

private:
	typedef vector<AiRule*> AiRules;
	typedef list<const Task*> Tasks;
	typedef deque<Vec2i> Positions;
	typedef map<Vec2i, const Unit*> UnitPositions;
	typedef map<const UnitType *, int> UnitTypeCount;
	typedef vector<const UnitType*> UnitTypes;
	typedef vector<const UpgradeType*> UpgradeTypes;
	typedef set<const ResourceType*> ResourceTypes;

private:
	GlestAiInterface *aiInterface;
	AiRules aiRules;
	int startLoc;
	bool randomMinWarriorsReached;
	int upgradeCount;
	int minWarriors;
	Tasks tasks;
	Positions expansionPositions;

	UnitPositions   m_knownEnemyLocations;
	ConstUnitVector m_visibleEnemies;
	ConstUnitPtr    m_closestEnemy;

	Random random;
	bool baseSeen;
	float aggressivness;

	//statistics
	UnitTypeCount unitTypeCount;
	UnitTypeCount buildingTypeCount;
	int unspecifiedBuildTasks;
	UnitTypeCount availableBuildings;		//available buildings and how many units can build them
	UnitTypes neededBuildings;
	UpgradeTypes availableUpgrades;
	ResourceTypes staticResourceUsed;
	ResourceTypes usableResources;

private:
	void evaluateEnemies();

public:
	~Ai();
	void init(GlestAiInterface *aiInterface, int32 randomSeed);
	void update();

	//state requests
	GlestAiInterface *getAiInterface() const	{return aiInterface;}
	Random* getRandom()							{return &random;}
	int getCountOfType(const UnitType *ut);

	int getCountOfClass(UnitClass uc);
	float getRatioOfClass(UnitClass uc);

	const ResourceType *getNeededResource();
	bool isStableBase();
	bool findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &pos);
	bool findAbleUnit(int *unitIndex, CmdClass ability, bool idleOnly);
	bool findAbleUnit(int *unitIndex, CmdClass ability, CmdClass currentCommand);

	bool beingAttacked(float radius, Vec2i &out_pos, Field &out_field);
	bool blindAttack(Vec2i &out_pos);

	bool isStaticResourceUsed(const ResourceType *rt) const;
	bool usesStaticResources() const {return !staticResourceUsed.empty();}
	void updateUsableResources();
	void updateStatistics();
	int getNeededUpgrades()		{return availableUpgrades.size();}
	int getNeededBuildings() {
		int count = neededBuildings.size() - unspecifiedBuildTasks;
		return count < 0 ? 0 : count;
	}
	bool isRepairable(const Unit *u) const;
	int getMinWarriors() const {return minWarriors;}

	//tasks
	void addTask(const Task *task);
	void addPriorityTask(const Task *task);
	bool anyTask();
	const Task *getTask() const;
	void removeTask(const Task *task);
	void retryTask(const Task *task);

	//expansions
	void addExpansion(const Vec2i &pos);
	Vec2i getRandomHomePosition();

	//actions
	void sendScoutPatrol();
	void massiveAttack(const Vec2i &pos, Field field, bool ultraAttack= false);
	void returnBase(int unitIndex);
	void harvest(int unitIndex);
};

}}//end namespace

#endif
