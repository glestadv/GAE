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

#include "command_type.h"

#include <algorithm>
#include <cassert>

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
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{


// =====================================================
// 	class CommandType
// =====================================================

CommandType::CommandType(CommandClass commandTypeClass, Clicks clicks, const char* typeName, bool queuable) {
	this->commandTypeClass = commandTypeClass;
	this->clicks = clicks;
	this->typeName = typeName;
	this->queuable = queuable;
}

//get
CommandClass CommandType::getClass() const{
	assert(this!=NULL);
	return commandTypeClass;
}

void CommandType::update(UnitUpdater *unitUpdater, Unit *unit) const{
	switch(commandTypeClass) {
		case ccStop:
			unitUpdater->updateStop(unit);
			break;

		case ccMove:
			unitUpdater->updateMove(unit);
			break;

		case ccAttack:
			unitUpdater->updateAttack(unit);
			break;

		case ccAttackStopped:
			unitUpdater->updateAttackStopped(unit);
			break;

		case ccBuild:
			unitUpdater->updateBuild(unit);
			break;

		case ccHarvest:
			unitUpdater->updateHarvest(unit);
			break;

		case ccRepair:
			unitUpdater->updateRepair(unit);
			break;

		case ccProduce:
			unitUpdater->updateProduce(unit);
			break;

		case ccUpgrade:
			unitUpdater->updateUpgrade(unit);
			break;

		case ccMorph:
			unitUpdater->updateMorph(unit);
			break;

		case ccCastSpell:
			unitUpdater->updateCastSpell(unit);
			break;

		case ccGuard:
			unitUpdater->updateGuard(unit);
			break;

		case ccPatrol:
			unitUpdater->updatePatrol(unit);
			break;

		case ccCount:
		case ccNull:
			assert(0);
			;
	}
}

void CommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	this->id= id;
	name= n->getChild("name")->getAttribute("value")->getRestrictedValue();

	//image
	const XmlNode *imageNode= n->getChild("image");
	image= Renderer::getInstance().newTexture2D(rsGame);
	image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

	RequirableType::load(n, dir, tt, ft);
}

// =====================================================
// 	class MoveBaseCommandType
// =====================================================

void MoveBaseCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));
}

// =====================================================
// 	class StopBaseCommandType
// =====================================================

void StopBaseCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//stop
   	string skillName= n->getChild("stop-skill")->getAttribute("value")->getRestrictedValue();
	stopSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));
}
// ===============================
// 	class AttackCommandTypeBase
// ===============================

void AttackCommandTypeBase::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	//attack
   	string skillName= n->getChild("attack-skill")->getAttribute("value")->getRestrictedValue();
	attackSkillTypes.resize(1);
	attackSkillTypes[0]= static_cast<const AttackSkillType*>(ut.getSkillType(skillName, scAttack));
}


/** Returns an attack skill for the given field if one exists. */
const AttackSkillType * AttackCommandTypeBase::getAttackSkillType(Field field) const {
	for(AttackSkillTypes::const_iterator i = attackSkillTypes.begin();
		   i != attackSkillTypes.end(); i++) {
		if((*i)->getField(field)) {
			return *i;
		}
	}
	return NULL;
}
// =====================================================
// 	class AttackCommandType
// =====================================================

void AttackCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);
	AttackCommandTypeBase::load(id, n, dir, tt, ft, ut);
}

// =====================================================
// 	class AttackStoppedCommandType
// =====================================================

void AttackStoppedCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	StopBaseCommandType::load(id, n, dir, tt, ft, ut);

	//attack
   	string skillName= n->getChild("attack-skill")->getAttribute("value")->getRestrictedValue();
	attackSkillTypes.resize(1);
	attackSkillTypes[0]= static_cast<const AttackSkillType*>(ut.getSkillType(skillName, scAttack));
}

// =====================================================
// 	class BuildCommandType
// =====================================================

BuildCommandType::~BuildCommandType(){
	deleteValues(builtSounds.getSounds().begin(), builtSounds.getSounds().end());
	deleteValues(startSounds.getSounds().begin(), startSounds.getSounds().end());
}

void BuildCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//build
   	string skillName= n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
	buildSkillType= static_cast<const BuildSkillType*>(ut.getSkillType(skillName, scBuild));

	//buildings built
	const XmlNode *buildingsNode= n->getChild("buildings");
	for(int i=0; i<buildingsNode->getChildCount(); ++i){
		const XmlNode *buildingNode= buildingsNode->getChild("building", i);
		string name= buildingNode->getAttribute("name")->getRestrictedValue();
		buildings.push_back(ft->getUnitType(name));
	}

	//start sound
	const XmlNode *startSoundNode= n->getChild("start-sound");
	if(startSoundNode->getAttribute("enabled")->getBoolValue()){
		startSounds.resize(startSoundNode->getChildCount());
		for(int i=0; i<startSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= startSoundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			startSounds[i]= sound;
		}
	}

	//built sound
	const XmlNode *builtSoundNode= n->getChild("built-sound");
	if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
		builtSounds.resize(builtSoundNode->getChildCount());
		for(int i=0; i<builtSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			builtSounds[i]= sound;
		}
	}
}

// =====================================================
// 	class HarvestCommandType
// =====================================================

void HarvestCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//harvest
   	string skillName= n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
	harvestSkillType= static_cast<const HarvestSkillType*>(ut.getSkillType(skillName, scHarvest));

	//stop loaded
   	skillName= n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
	stopLoadedSkillType= static_cast<const StopSkillType*>(ut.getSkillType(skillName, scStop));

	//move loaded
   	skillName= n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
	moveLoadedSkillType= static_cast<const MoveSkillType*>(ut.getSkillType(skillName, scMove));

	//resources can harvest
	const XmlNode *resourcesNode= n->getChild("harvested-resources");
	for(int i=0; i<resourcesNode->getChildCount(); ++i){
		const XmlNode *resourceNode= resourcesNode->getChild("resource", i);
		harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
	}

	maxLoad= n->getChild("max-load")->getAttribute("value")->getIntValue();
	hitsPerUnit= n->getChild("hits-per-unit")->getAttribute("value")->getIntValue();
}

void HarvestCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	str+= lang.get("HarvestSpeed")+": "+ intToStr(harvestSkillType->getSpeed()/hitsPerUnit);
	EnhancementTypeBase::describeModifier(str, (unit->getSpeed(harvestSkillType) - harvestSkillType->getSpeed())/hitsPerUnit);
	str+= "\n";
	str+= lang.get("MaxLoad")+": "+ intToStr(maxLoad)+"\n";
	str+= lang.get("LoadedSpeed")+": "+ intToStr(moveLoadedSkillType->getSpeed())+"\n";
	if(harvestSkillType->getEpCost()!=0){
		str+= lang.get("EpCost")+": "+intToStr(harvestSkillType->getEpCost())+"\n";
	}
	str+=lang.get("Resources")+":\n";
	for(int i=0; i<getHarvestedResourceCount(); ++i){
		str+= getHarvestedResource(i)->getName()+"\n";
	}
}

bool HarvestCommandType::canHarvest(const ResourceType *resourceType) const{
	return find(harvestedResources.begin(), harvestedResources.end(), resourceType) != harvestedResources.end();
}

// =====================================================
// 	class RepairCommandType
// =====================================================

RepairCommandType::~RepairCommandType(){
}

void RepairCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//repair
   	string skillName= n->getChild("repair-skill")->getAttribute("value")->getRestrictedValue();
	repairSkillType= static_cast<const RepairSkillType*>(ut.getSkillType(skillName, scRepair));

	//repaired units
	const XmlNode *unitsNode= n->getChild("repaired-units");
	for(int i=0; i<unitsNode->getChildCount(); ++i){
		const XmlNode *unitNode= unitsNode->getChild("unit", i);
		repairableUnits.push_back(ft->getUnitType(unitNode->getAttribute("name")->getRestrictedValue()));
	}
}

void RepairCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	repairSkillType->getDesc(str, unit);

	str+="\n"+lang.get("CanRepair")+":\n";
	for(int i=0; i<repairableUnits.size(); ++i){
		str+= (static_cast<const UnitType*>(repairableUnits[i]))->getName()+"\n";
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

// =====================================================
// 	class ProduceCommandType
// =====================================================

//varios
void ProduceCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//produce
   	string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
	produceSkillType= static_cast<const ProduceSkillType*>(ut.getSkillType(skillName, scProduce));

	string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
	producedUnit= ft->getUnitType(producedUnitName);
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

void UpgradeCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){

	CommandType::load(id, n, dir, tt, ft, ut);

	//upgrade
   	string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
	upgradeSkillType= static_cast<const UpgradeSkillType*>(ut.getSkillType(skillName, scUpgrade));

	string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
	producedUpgrade= ft->getUpgradeType(producedUpgradeName);

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

void MorphCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	CommandType::load(id, n, dir, tt, ft, ut);

	//morph skill
   	string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
	morphSkillType= static_cast<const MorphSkillType*>(ut.getSkillType(skillName, scMorph));

	//morph unit
   	string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
	morphUnit= ft->getUnitType(morphUnitName);

	//discount
	discount= n->getChild("discount")->getAttribute("value")->getIntValue();
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

void CastSpellCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	MoveBaseCommandType::load(id, n, dir, tt, ft, ut);

	//cast spell
   	string skillName= n->getChild("cast-spell-skill")->getAttribute("value")->getRestrictedValue();
	castSpellSkillType= static_cast<const CastSpellSkillType*>(ut.getSkillType(skillName, scCastSpell));
}

void CastSpellCommandType::getDesc(string &str, const Unit *unit) const{
	castSpellSkillType->getDesc(str, unit);
	castSpellSkillType->descSpeed(str, unit, "Speed");

	//movement speed
	MoveBaseCommandType::getDesc(str, unit);
}

// =====================================================
// 	class GuardCommandType
// =====================================================

void GuardCommandType::load(int id, const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType &ut){
	AttackCommandType::load(id, n, dir, tt, ft, ut);

	//distance
	maxDistance = n->getChild("max-distance")->getAttribute("value")->getIntValue();
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
}

CommandTypeFactory &CommandTypeFactory::getInstance(){
	static CommandTypeFactory ctf;
	return ctf;
}

}}//end namespace
