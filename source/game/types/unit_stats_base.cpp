// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <algorithm>
#include <cassert>

#include "unit_type.h"
#include "util.h"
#include "logger.h"
#include "lang.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "resource.h"
#include "renderer.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Game {

const char *Zones::names[] = {
	"prop",
	"land" /*<- == "surface"*/,
	"air", /*, "subsurface"*/
};

const char *Fields::names[] = {
	"land"/*<- == "walkable"*/,
	"air",
	"any_water",
	"deep_water",
	"amphibious"
};

const char *UnitProperties::names[] = {
	"burnable",
	"rotated_climb",
	"wall"
};

// ===============================
//  class UnitStatsBase
// ===============================

size_t UnitStatsBase::damageMultiplierCount = 0;

// ==================== misc ====================

void UnitStatsBase::reset() {
	maxHp = 0;
	hpRegeneration = 0;
	maxEp = 0;
	epRegeneration = 0;
	fields.flags = 0;
	properties.flags = 0;
	sight = 0;
	armor = 0;
	armorType = NULL;
//	bodyType = NULL;
	light = false;
	lightColor.x = 0.0f;
	lightColor.y = 0.0f;
	lightColor.z = 0.0f;
	size = 0;
	height = 0;

	attackStrength = 0;
	effectStrength = 0.0f;
	attackPctStolen = 0.0f;
	attackRange = 0;
	moveSpeed = 0;
	attackSpeed = 0;
	prodSpeed = 0;
	repairSpeed = 0;
	harvestSpeed = 0;

	for (int i = 0; i < damageMultiplierCount; ++i) {
		damageMultipliers[i] = 0.f;
	}
}

void UnitStatsBase::setValues(const UnitStatsBase &o) {
	maxHp			= o.maxHp;
	hpRegeneration	= o.hpRegeneration;
	maxEp			= o.maxEp;
	epRegeneration	= o.epRegeneration;
	fields.flags	= o.fields.flags;
	properties.flags = o.properties.flags;
	sight			= o.sight;
	armor			= o.armor;
	armorType		= o.armorType;
	light			= o.light;
	lightColor		= o.lightColor;
	size			= o.size;
	height			= o.height;

	attackStrength	= o.attackStrength;
	effectStrength	= o.effectStrength;
	attackPctStolen	= o.attackPctStolen;
	attackRange		= o.attackRange;
	moveSpeed		= o.moveSpeed;
	attackSpeed		= o.attackSpeed;
	prodSpeed		= o.prodSpeed;
	repairSpeed		= o.repairSpeed;
	harvestSpeed	= o.harvestSpeed;

	for (int i = 0; i < damageMultiplierCount; ++i) {
		damageMultipliers[i] = o.damageMultipliers[i];
	}

}

void UnitStatsBase::addStatic(const EnhancementTypeBase &e, float strength) {
	maxHp			+= lroundf(e.getMaxHp() * strength);
	hpRegeneration	+= lroundf(e.getHpRegeneration() * strength);
	maxEp			+= lroundf(e.getMaxEp() * strength);
	epRegeneration	+= lroundf(e.getEpRegeneration() * strength);
	fields.flags	= (fields.flags | e.getFields().flags) & ~(e.getRemovedFields().flags);
	properties.flags = (properties.flags | e.getProperties().flags) & ~(e.getRemovedProperties().flags);
	sight			+= lroundf(e.getSight() * strength);
	armor			+= lroundf(e.getArmor() * strength);
	if (e.getArmorType() != NULL) {			//if non-null, overwrite previous
		armorType = e.getArmorType();
	}
	light = light || e.getLight();
	if (e.getLight()) {						//if non-null, overwrite previous
		lightColor = e.getLightColor();
	}
	size			+= e.getSize();
	height			+= e.getHeight();

	attackStrength	+= lroundf(e.getAttackStrength() * strength);
	effectStrength	+= e.getEffectStrength() * strength;
	attackPctStolen	+= e.getAttackPctStolen() * strength;
	attackRange		+= lroundf(e.getAttackRange() * strength);
	moveSpeed		+= lroundf(e.getMoveSpeed() * strength);
	attackSpeed		+= lroundf(e.getAttackSpeed() * strength);
	prodSpeed		+= lroundf(e.getProdSpeed() * strength);
	repairSpeed		+= lroundf(e.getRepairSpeed() * strength);
	harvestSpeed	+= lroundf(e.getHarvestSpeed() * strength);
}

void UnitStatsBase::applyMultipliers(const EnhancementTypeBase &e) {
	maxHp			= lroundf(maxHp * e.getMaxHpMult());
	hpRegeneration	= lroundf(hpRegeneration * e.getHpRegenerationMult());
	maxEp			= lroundf(maxEp * e.getMaxEpMult());
	epRegeneration	= lroundf(epRegeneration * e.getEpRegenerationMult());
	sight			= lroundf(sight * e.getSightMult());
	armor			= lroundf(armor * e.getArmorMult());
	attackStrength	= lroundf(attackStrength * e.getAttackStrengthMult());
	effectStrength	= effectStrength * e.getEffectStrengthMult();
	attackPctStolen	= attackPctStolen * e.getAttackPctStolenMult();
	attackRange		= lroundf(attackRange * e.getAttackRangeMult());
	moveSpeed		= lroundf(moveSpeed * e.getMoveSpeedMult());
	attackSpeed		= lroundf(attackSpeed * e.getAttackSpeedMult());
	prodSpeed		= lroundf(prodSpeed * e.getProdSpeedMult());
	repairSpeed		= lroundf(repairSpeed * e.getRepairSpeedMult());
	harvestSpeed	= lroundf(harvestSpeed * e.getHarvestSpeedMult());
}

// legacy load for Unit class
bool UnitStatsBase::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
	//maxHp
	try {
		maxHp = baseNode->getChildIntValue("max-hp");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//hpRegeneration
	try {
		hpRegeneration = baseNode->getChild("max-hp")->getIntAttribute("regeneration");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//maxEp
	try {
		maxEp = baseNode->getChildIntValue("max-ep");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	try {
		//epRegeneration
		if (maxEp) {
			epRegeneration = baseNode->getChild("max-ep")->getIntAttribute("regeneration");
		} else {
			XmlAttribute *epRegenAttr = baseNode->getChild("max-ep")->getAttribute("regeneration", false);
			epRegeneration = epRegenAttr ? epRegenAttr->getIntValue() : 0;
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//sight
	try {
		sight = baseNode->getChildIntValue("sight");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//armor
	try {
		armor = baseNode->getChildIntValue("armor");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//armor type string
	try {
		string armorTypeName = baseNode->getChildRestrictedValue("armor-type");
		armorType = techTree->getArmorType(armorTypeName);
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//light & lightColor
	try {
		const XmlNode *lightNode = baseNode->getChild("light");
		light = lightNode->getAttribute("enabled")->getBoolValue();
		if (light)
			lightColor = lightNode->getColor3Value();
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//size
	try {
		size = baseNode->getChildIntValue("size");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//height
	try {
		height = baseNode->getChildIntValue("height");
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void UnitStatsBase::save(XmlNode *node) {
	node->addChild("max-ep", maxHp);
	node->addChild("hp-regeneration", hpRegeneration);
	node->addChild("max-ep", maxEp);
	node->addChild("ep-regeneration", epRegeneration);
	node->addChild("fields", (int)fields.flags);
	node->addChild("properties", (int)properties.flags);
	node->addChild("sight", sight);
	node->addChild("armor", armor);
	node->addChild("armor-type", armorType);
	node->addChild("light", light);
	node->addChild("lightColor", lightColor);
	node->addChild("size", size);
	node->addChild("height", height);
	node->addChild("attack-strength", attackStrength);
	node->addChild("effect-strength", effectStrength);
	node->addChild("attack-percent-stolen", attackPctStolen);
	node->addChild("attack-range", attackRange);
	node->addChild("move-speed", moveSpeed);
	node->addChild("attack-speed", attackSpeed);
	node->addChild("production-speed", prodSpeed);
	node->addChild("repair-speed", repairSpeed);
	node->addChild("harvest-speed", harvestSpeed);
}

// ===============================
//  class EnhancementTypeBase
// ===============================

// ==================== misc ====================

void EnhancementTypeBase::reset() {
	UnitStatsBase::reset();

	maxHpMult			= 1.0f;
	hpRegenerationMult	= 1.0f;
	maxEpMult			= 1.0f;
	epRegenerationMult	= 1.0f;
	sightMult			= 1.0f;
	armorMult			= 1.0f;
	attackStrengthMult	= 1.0f;
	effectStrengthMult	= 1.0f;
	attackPctStolenMult	= 1.0f;
	attackRangeMult		= 1.0f;
	moveSpeedMult		= 1.0f;
	attackSpeedMult		= 1.0f;
	prodSpeedMult		= 1.0f;
	repairSpeedMult		= 1.0f;
	harvestSpeedMult	= 1.0f;

	antiFields.flags	= 0;
	antiProperties.flags = 0;
}

void EnhancementTypeBase::save(XmlNode *node) const {
	XmlNode *m = node->addChild("multipliers");
	m->addChild("max-ep", maxHpMult);
	m->addChild("hp-regeneration", hpRegenerationMult);
	m->addChild("max-ep", maxEpMult);
	m->addChild("ep-regeneration", epRegenerationMult);
	m->addChild("sight", sightMult);
	m->addChild("armor", armorMult);
	m->addChild("attack-strength", attackStrengthMult);
	m->addChild("effect-strength", effectStrengthMult);
	m->addChild("attack-percent-stolen", attackPctStolenMult);
	m->addChild("attack-range", attackRangeMult);
	m->addChild("move-speed", moveSpeedMult);
	m->addChild("attack-speed", attackSpeedMult);
	m->addChild("production-speed", prodSpeedMult);
	m->addChild("repair-speed", repairSpeedMult);
	m->addChild("harvest-speed", harvestSpeedMult);
}

void EnhancementTypeBase::addMultipliers(const EnhancementTypeBase &e, float strength) {
	maxHpMult			+=  strength * (e.getMaxHpMult() - 1.0f);
	hpRegenerationMult	+=  strength * (e.getHpRegenerationMult() - 1.0f);
	maxEpMult			+=  strength * (e.getMaxEpMult() - 1.0f);
	epRegenerationMult	+=  strength * (e.getEpRegenerationMult() - 1.0f);
	sightMult			+=  strength * (e.getSightMult() - 1.0f);
	armorMult			+=  strength * (e.getArmorMult() - 1.0f);
	attackStrengthMult	+=  strength * (e.getAttackStrengthMult() - 1.0f);
	effectStrengthMult	+=  strength * (e.getEffectStrengthMult() - 1.0f);
	attackPctStolenMult	+=  strength * (e.getAttackPctStolenMult() - 1.0f);
	attackRangeMult		+=  strength * (e.getAttackRangeMult() - 1.0f);
	moveSpeedMult		+=  strength * (e.getMoveSpeedMult() - 1.0f);
	attackSpeedMult		+=  strength * (e.getAttackSpeedMult() - 1.0f);
	prodSpeedMult		+=  strength * (e.getProdSpeedMult() - 1.0f);
	repairSpeedMult		+=  strength * (e.getRepairSpeedMult() - 1.0f);
	harvestSpeedMult	+=  strength * (e.getHarvestSpeedMult() - 1.0f);
}

/*inline */void EnhancementTypeBase::formatModifier(string &str, const char *pre, const char* label,
        int value, float multiplier) {
	Lang &lang = Lang::getInstance();

	if (value) {
		str += pre + lang.get(label) + ": ";
		if (value > 0) {
			str += "+";
		}
		str += intToStr(value);
	}

	if (multiplier != 1.0f) {
		if (value) {
			str += ", ";
		} else {
			str += pre + lang.get(label) + ": ";
		}

		if (multiplier > 1.0f) {
			str += "+";
		}
		str += intToStr((int)((multiplier - 1.0f) * 100.0f)) + "%";
	}
}

void EnhancementTypeBase::getDesc(string &str, const char *pre) const {
	formatModifier(str, pre, "Hp", maxHp, maxHpMult);
	formatModifier(str, pre, "HpRegeneration", hpRegeneration, hpRegenerationMult);
	formatModifier(str, pre, "Sight", sight, sightMult);
	formatModifier(str, pre, "Ep", maxEp, maxEpMult);
	formatModifier(str, pre, "EpRegeneration", epRegeneration, epRegenerationMult);
	formatModifier(str, pre, "AttackStrength", attackStrength, attackStrengthMult);
	formatModifier(str, pre, "EffectStrength", (int)round(effectStrength * 100.0f), effectStrengthMult);
	formatModifier(str, pre, "AttackPctStolen", (int)round(attackPctStolen * 100.0f), attackPctStolenMult);
	formatModifier(str, pre, "AttackSpeed", attackSpeed, attackSpeedMult);
	formatModifier(str, pre, "AttackDistance", attackRange, attackRangeMult);
	formatModifier(str, pre, "Armor", armor, armorMult);
	formatModifier(str, pre, "WalkSpeed", moveSpeed, moveSpeedMult);
	formatModifier(str, pre, "ProductionSpeed", prodSpeed, prodSpeedMult);
	formatModifier(str, pre, "RepairSpeed", repairSpeed, repairSpeedMult);
	formatModifier(str, pre, "HarvestSpeed", harvestSpeed, harvestSpeedMult);
}

//Initialize value from <static-modifiers>
void EnhancementTypeBase::initStaticModifier(const XmlNode *node, const string &dir) {
	const string &name = node->getName();
	int value = node->getAttribute("value")->getIntValue();

	if (name == "max-hp") {
		maxHp = value;
	} else if (name == "max-ep") {
		maxEp = value;
	} else if (name == "hp-regeneration") {
		hpRegeneration = value;
	} else if (name == "ep-regeneration") {
		epRegeneration = value;
	} else if (name == "sight") {
		sight = value;
	} else if (name == "armor") {
		armor = value;
	} else if (name == "attack-strength") {
		attackStrength = value;
	} else if (name == "effect-strength") {
		effectStrength = (float)value / 100.f;
	} else if (name == "attack-percent-stolen") {
		attackPctStolen	= (float)value / 100.f;
	} else if (name == "attack-range") {
		attackRange = value;
	} else if (name == "move-speed") {
		moveSpeed = value;
	} else if (name == "attack-speed") {
		attackSpeed = value;
	} else if (name == "production-speed") {
		prodSpeed = value;
	} else if (name == "repair-speed") {
		repairSpeed = value;
	} else if (name == "harvest-speed") {
		harvestSpeed = value;
	} else {
		throw runtime_error("Not a valid child of <static-modifiers>: " + name + ": " + dir);
	}
}

//Initialize value from <multipliers>
void EnhancementTypeBase::initMultiplier(const XmlNode *node, const string &dir) {
	const string &name = node->getName();
	float value = node->getAttribute("value")->getFloatValue();

	if (name == "max-hp") {
		maxHpMult = value;
	} else if (name == "max-ep") {
		maxEpMult = value;
	} else if (name == "hp-regeneration") {
		hpRegenerationMult = value;
	} else if (name == "ep-regeneration") {
		epRegenerationMult = value;
	} else if (name == "sight") {
		sightMult = value;
	} else if (name == "armor") {
		armorMult = value;
	} else if (name == "attack-strength") {
		attackStrengthMult = value;
	} else if (name == "effect-strength") {
		effectStrengthMult = value;
	} else if (name == "attack-percent-stolen") {
		attackPctStolenMult = value;
	} else if (name == "attack-range") {
		attackRangeMult = value;
	} else if (name == "move-speed") {
		moveSpeedMult = value;
	} else if (name == "attack-speed") {
		attackSpeedMult = value;
	} else if (name == "production-speed") {
		prodSpeedMult = value;
	} else if (name == "repair-speed") {
		repairSpeedMult = value;
	} else if (name == "harvest-speed") {
		harvestSpeedMult = value;
	} else {
		throw runtime_error("Not a valid child of <multipliers>: <" + name + ">: " + dir);
	}
}

bool EnhancementTypeBase::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	const XmlNode *node;
	bool loadOk = true;
	//static modifiers
	try {
		node = baseNode->getChild("static-modifiers", 0, false);
		if (node) {
			for (int i = 0; i < node->getChildCount(); ++i) {
				initStaticModifier(node->getChild(i), dir);
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	//multipliers
	try {
		node = baseNode->getChild("multipliers", 0, false);
		if (node)
			for (int i = 0; i < node->getChildCount(); ++i) {
				initMultiplier(node->getChild(i), dir);
			}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}


} // end namespace
