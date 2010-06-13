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

#ifndef _GLEST_GAME_COMMANDTYPE_H_
#define _GLEST_GAME_COMMANDTYPE_H_

#include "element_type.h"
#include "resource_type.h"
#include "lang.h"
#include "skill_type.h"
#include "factory.h"
#include "xml_parser.h"
#include "sound_container.h"
#include "skill_type.h"
#include "upgrade_type.h"
#include "game_constants.h"

#include "prototypes_enums.h"
#include "entities_enums.h"

// All command update methods begin with _PROFILE_COMMAND_UPDATE()
// This allows profiling of command updates to be switched on/off independently
// of other profiling macros
#if 0
#	define _PROFILE_COMMAND_UPDATE() _PROFILE_FUNCTION()
#else
#	define _PROFILE_COMMAND_UPDATE()
#endif

#if defined(LOG_BUILD_COMMAND) && LOG_BUILD_COMMAND
#	define BUILD_LOG(x) STREAM_LOG(x)
#else
#	define BUILD_LOG(x)
#endif

#if defined(LOG_REPAIR_COMMAND) && LOG_REPAIR_COMMAND
#	if LOG_REPAIR_COMMAND > 1
#		define REPAIR_LOG(x) STREAM_LOG(x)
#		define REPAIR_LOG2(x) STREAM_LOG(x)
#	else
#		define REPAIR_LOG(x) STREAM_LOG(x)
#		define REPAIR_LOG2(x)
#	endif
#else
#	define REPAIR_LOG(x)
#	define REPAIR_LOG2(x)
#endif

#if defined(LOG_HARVEST_COMMAND) && LOG_HARVEST_COMMAND
#	define HARVEST_LOG(x) STREAM_LOG(x)
#else
#	define HARVEST_LOG(x)
#endif

using Shared::Util::MultiFactory;
using namespace Glest::Entities;
using Glest::Gui::Clicks;

namespace Glest { namespace ProtoTypes {

// =====================================================
//  class CommandType
//
/// A complex action performed by a unit, composed by skills
// =====================================================

class CommandType : public RequirableType {
	friend class CommandTypeFactory;
protected:
	//CommandClass cc;
	Clicks clicks;
	bool queuable;
	const UnitType *unitType;

	void setIdAndUnitType(int v, UnitType *ut) { id = v; unitType = ut; }

public:
	CommandType(const char* name, /*CommandClass cc,*/ Clicks clicks, bool queuable = false);

	virtual void update(Unit *unit) const = 0;
	virtual void getDesc(string &str, const Unit *unit) const = 0;

	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;

	virtual string toString() const						{return Lang::getInstance().get(name);}

	virtual const ProducibleType *getProduced() const	{return NULL;}

	bool isQueuable() const								{return queuable;}

	//get
	virtual CommandClass getClass() const = 0; //{return cc;}
	Clicks getClicks() const							{return clicks;}
	string getDesc(const Unit *unit) const {
		string str;
		str = name + "\n";
		getDesc(str, unit);
		return str;
	}

protected:
	// static command update helpers... don't really belong here, but it's convenient for now
	///@todo move range checking update helpers to Cartographer (?)
	/// See also: RepairCommandType::repairableInRange()
	static bool unitInRange(const Unit *unit, int range, Unit **rangedPtr,
								const AttackSkillTypes *asts, const AttackSkillType **past);

	static bool attackerInSight(const Unit *unit, Unit **rangedPtr);

	static bool attackableInRange(const Unit *unit, Unit **rangedPtr, 
								const AttackSkillTypes *asts, const AttackSkillType **past);

	static bool attackableInSight(const Unit *unit, Unit **rangedPtr, 
								const AttackSkillTypes *asts, const AttackSkillType **past);

public:
	// must be called before a new game loads anything, savegames need command types to get 
	// the same id everytime a game is started, not just the first game in one 'program session'
	///@todo maybe CommandTypeFactory could be made non-singleton, owned by the World (or Game)
	// and take control of type ids, thus neatly removing the need for this kludge
//	static void resetIdCounter() { nextId = 0; }

	Command* doAutoCommand(Unit *unit) const;
};

// ===============================
//  class MoveBaseCommandType
// ===============================

class MoveBaseCommandType: public CommandType {
protected:
	const MoveSkillType *moveSkillType;

public:
	MoveBaseCommandType(const char* name, /*CommandClass commandTypeClass,*/ Clicks clicks) :
			CommandType(name, /*commandTypeClass,*/ clicks) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.add(moveSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{moveSkillType->getDesc(str, unit);}
	const MoveSkillType *getMoveSkillType() const				{return moveSkillType;}

public:
	Command* doAutoFlee(Unit *unit) const;
};

// ===============================
//  class StopBaseCommandType
// ===============================

class StopBaseCommandType: public CommandType {
protected:
	const StopSkillType *stopSkillType;

public:
	StopBaseCommandType(const char* name, /*CommandClass commandTypeClass,*/ Clicks clicks) :
			CommandType(name, /*commandTypeClass,*/ clicks) {}
	virtual void doChecksum(Checksum &checksum) const {
		CommandType::doChecksum(checksum);
		checksum.add(stopSkillType->getName());
	}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc(string &str, const Unit *unit) const	{stopSkillType->getDesc(str, unit);}
	const StopSkillType *getStopSkillType() const				{return stopSkillType;}
};

// ===============================
//  class StopCommandType
// ===============================

class StopCommandType: public StopBaseCommandType {
public:
	StopCommandType() : StopBaseCommandType("Stop", /*CommandClass::STOP,*/ Clicks::ONE) {}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::STOP; }
};

// ===============================
//  class MoveCommandType
// ===============================

class MoveCommandType: public MoveBaseCommandType {
public:
	MoveCommandType() : MoveBaseCommandType("Move", /*CommandClass::MOVE,*/ Clicks::TWO) {}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::MOVE; }
};

// ===============================
//  class AttackCommandTypeBase
// ===============================

class AttackCommandTypeBase {
protected:
	AttackSkillTypes attackSkillTypes;

public:
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType *ut);
	virtual void getDesc(string &str, const Unit *unit) const {attackSkillTypes.getDesc(str, unit);}
	virtual void doChecksum(Checksum &checksum) const {
		attackSkillTypes.doChecksum(checksum);
	}
// const AttackSkillType *getAttackSkillType() const	{return attackSkillTypes.begin()->first;}
// const AttackSkillType *getAttackSkillType(Field field) const;
	const AttackSkillTypes *getAttackSkillTypes() const	{return &attackSkillTypes;}
};

// ===============================
//  class AttackCommandType
// ===============================

class AttackCommandType: public MoveBaseCommandType, public AttackCommandTypeBase {

public:
	AttackCommandType(const char* name = "Attack", /*CommandClass commandTypeClass = CommandClass::ATTACK,*/ Clicks clicks = Clicks::TWO) :
			MoveBaseCommandType(name, /*commandTypeClass,*/ clicks) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		MoveBaseCommandType::doChecksum(checksum);
		AttackCommandTypeBase::doChecksum(checksum);
	}
	virtual void getDesc(string &str, const Unit *unit) const {
		AttackCommandTypeBase::getDesc(str, unit);
		MoveBaseCommandType::getDesc(str, unit);
	}

	bool updateGeneric(Unit *unit, Command *command, const AttackCommandType *act, Unit* target, const Vec2i &targetPos) const;

	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::ATTACK; }

public:
	Command* doAutoAttack(Unit *unit) const;
};

// =======================================
//  class AttackStoppedCommandType
// =======================================

class AttackStoppedCommandType: public StopBaseCommandType, public AttackCommandTypeBase {
public:
	AttackStoppedCommandType() : StopBaseCommandType("AttackStopped", /*CommandClass::ATTACK_STOPPED,*/ Clicks::ONE) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		StopBaseCommandType::doChecksum(checksum);
		AttackCommandTypeBase::doChecksum(checksum);
	}
	virtual void getDesc(string &str, const Unit *unit) const {
		AttackCommandTypeBase::getDesc(str, unit);
	}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::ATTACK_STOPPED; }

public:
	Command* doAutoAttack(Unit *unit) const;
};


// ===============================
//  class BuildCommandType
// ===============================

class BuildCommandType: public MoveBaseCommandType {
private:
	const BuildSkillType* buildSkillType;
	vector<const UnitType*> buildings;
	SoundContainer startSounds;
	SoundContainer builtSounds;

public:
	BuildCommandType() : MoveBaseCommandType("Build", /*CommandClass::BUILD,*/ Clicks::TWO) {}
	~BuildCommandType();
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const {
		buildSkillType->getDesc(str, unit);
	}
	virtual void update(Unit *unit) const;

	//get
	const BuildSkillType *getBuildSkillType() const	{return buildSkillType;}
	int getBuildingCount() const					{return buildings.size();}
	const UnitType * getBuilding(int i) const		{return buildings[i];}
	StaticSound *getStartSound() const				{return startSounds.getRandSound();}
	StaticSound *getBuiltSound() const				{return builtSounds.getRandSound();}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::BUILD; }

private:
	bool hasArrived(Unit *unit, const Command *command, const UnitType *builtUnitType) const;
	void existingBuild(Unit *unit, Command *command, Unit *builtUnit) const;
	/** @returns true if successful */
	bool attemptMoveUnits(const vector<Unit *> &occupants) const;
	void blockedBuild(Unit *unit) const;
	void acceptBuild(Unit *unit, Command *command, const UnitType *builtUnitType) const;
	void newBuild(Unit *unit, const Command *command, const UnitType *builtUnitType) const;
};


// ===============================
//  class HarvestCommandType
// ===============================

class HarvestCommandType: public MoveBaseCommandType {
private:
	const MoveSkillType *moveLoadedSkillType;
	const HarvestSkillType *harvestSkillType;
	const StopSkillType *stopLoadedSkillType;
	vector<const ResourceType*> harvestedResources;
	int maxLoad;
	int hitsPerUnit;

public:
	HarvestCommandType() : MoveBaseCommandType("Harvest", /*CommandClass::HARVEST,*/ Clicks::TWO) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;

	//get
	const MoveSkillType *getMoveLoadedSkillType() const		{return moveLoadedSkillType;}
	const HarvestSkillType *getHarvestSkillType() const		{return harvestSkillType;}
	const StopSkillType *getStopLoadedSkillType() const		{return stopLoadedSkillType;}
	int getMaxLoad() const									{return maxLoad;}
	int getHitsPerUnit() const								{return hitsPerUnit;}
	int getHarvestedResourceCount() const					{return harvestedResources.size();}
	const ResourceType* getHarvestedResource(int i) const	{return harvestedResources[i];}
	bool canHarvest(const ResourceType *resourceType) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::HARVEST; }
};

// ===============================
//  class RepairCommandType
// ===============================

class RepairCommandType: public MoveBaseCommandType {
private:
	const RepairSkillType* repairSkillType;
	vector<const UnitType*>  repairableUnits;

public:
	RepairCommandType() : MoveBaseCommandType("Repair", /*CommandClass::REPAIR,*/ Clicks::TWO) {}
	~RepairCommandType() {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;

	//get
	const RepairSkillType *getRepairSkillType() const	{return repairSkillType;}
	bool canRepair(const UnitType *unitType) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::REPAIR; }

protected:
	///@todo move to Cartographer, generalise so that the same code can be used for searching 
	// for bad guys to kill/run-from and looking for friendlies to repair.
	static bool repairableInRange(const Unit *unit, Vec2i centre, int centreSize, Unit **rangedPtr, 
			const RepairCommandType *rct, const RepairSkillType *rst, int range, 
			bool allowSelf, bool militaryOnly, bool damagedOnly);

	static bool repairableInRange(const Unit *unit, Unit **rangedPtr, const RepairCommandType *rct,
			int range, bool allowSelf = false, bool militaryOnly = false, bool damagedOnly = true);

	static bool repairableInSight(const Unit *unit, Unit **rangedPtr, const RepairCommandType *rct, bool allowSelf);

public:
	Command* doAutoRepair(Unit *unit) const;
};


// ===============================
//  class ProduceCommandType
// ===============================

class ProduceCommandType: public CommandType {
private:
	const ProduceSkillType* produceSkillType;
	const UnitType *producedUnit;

public:
	ProduceCommandType() : CommandType("Produce", /*CommandClass::PRODUCE,*/ Clicks::ONE, true) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

	//get
	const ProduceSkillType *getProduceSkillType() const	{return produceSkillType;}
	const UnitType *getProducedUnit() const				{return producedUnit;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::PRODUCE; }
};


// ===============================
//  class UpgradeCommandType
// ===============================

class UpgradeCommandType: public CommandType {
private:
	const UpgradeSkillType* upgradeSkillType;
	const UpgradeType* producedUpgrade;

public:
	UpgradeCommandType() : CommandType("Upgrade", /*CommandClass::UPGRADE,*/ Clicks::ONE, true) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;
	virtual void getDesc(string &str, const Unit *unit) const {
		upgradeSkillType->getDesc(str, unit);
		str += "\n" + getProducedUpgrade()->getDesc();
	}
	virtual void update(Unit *unit) const;

	//get
	const UpgradeSkillType *getUpgradeSkillType() const	{return upgradeSkillType;}
	const UpgradeType *getProducedUpgrade() const		{return producedUpgrade;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::UPGRADE; }
};

// ===============================
//  class MorphCommandType
// ===============================

class MorphCommandType: public CommandType {
private:
	const MorphSkillType* morphSkillType;
	const UnitType* morphUnit;
	int discount;

public:
	MorphCommandType() : CommandType("Morph", /*CommandClass::MORPH,*/ Clicks::ONE) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void update(Unit *unit) const;
	virtual string getReqDesc() const;
	virtual const ProducibleType *getProduced() const;

	//get
	const MorphSkillType *getMorphSkillType() const	{return morphSkillType;}
	const UnitType *getMorphUnit() const			{return morphUnit;}
	int getDiscount() const							{return discount;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::MORPH; }
};

// ===============================
//  class GuardCommandType
// ===============================

class GuardCommandType: public AttackCommandType {
private:
	int maxDistance;

public:
	GuardCommandType(const char* name = "Guard", /*CommandClass commandTypeClass = CommandClass::GUARD,*/ Clicks clicks = Clicks::TWO) :
			AttackCommandType(name, /*commandTypeClass,*/ clicks) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void update(Unit *unit) const;
	int getMaxDistance() const {return maxDistance;}

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::GUARD; }
};

// ===============================
//  class PatrolCommandType
// ===============================

class PatrolCommandType: public GuardCommandType {
public:
	PatrolCommandType() : GuardCommandType("Patrol", /*CommandClass::PATROL,*/ Clicks::TWO) {}
	virtual void update(Unit *unit) const;

	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::PATROL; }
};

// ===============================
//  class SetMeetingPointCommandType
// ===============================

class SetMeetingPointCommandType: public CommandType {
public:
	SetMeetingPointCommandType() :
			CommandType("SetMeetingPoint", /*CommandClass::SET_MEETING_POINT,*/ Clicks::TWO) {}
	virtual bool load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {return true;}
	virtual void getDesc(string &str, const Unit *unit) const {}
	virtual void update(Unit *unit) const {
		throw std::runtime_error("Set meeting point command in queue. Thats wrong.");
	}
	virtual CommandClass getClass() const { return typeClass(); }
	static CommandClass typeClass() { return CommandClass::SET_MEETING_POINT; }
};

// ===============================
//  class CommandFactory
// ===============================

class CommandTypeFactory: private MultiFactory<CommandType> {
private:
	int idCounter;
	vector<CommandType *> types;

public:
	CommandTypeFactory();

	CommandType *newInstance(string classId, UnitType *owner) {
		CommandType *ct = MultiFactory<CommandType>::newInstance(classId);
		ct->setIdAndUnitType(idCounter++, owner);
		types.push_back(ct);
		return ct;
	}

	CommandType* getType(int id) {
		if (id < 0 || id >= types.size()) {
			throw runtime_error("Error: Unknown command type id: " + intToStr(id));
		}
		return types[id];
	}
};

// update helper, move somewhere sensible
// =====================================================
// 	class Targets
// =====================================================

/** Utility class for managing multiple targets by distance. */
class Targets : public std::map<Unit*, fixed> {
private:
	Unit *nearest;
	fixed distance;

public:
	Targets() : nearest(0), distance(fixed::max_value()) {}
	void record(Unit *target, fixed dist);
	Unit* getNearest() { return nearest; }
	Unit* getNearestSkillClass(SkillClass sc);
	Unit* getNearestHpRatio(fixed hpRatio);
};

}}//end namespace

#endif
