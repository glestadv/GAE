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

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace ProtoTypes {

// ===============================
//  class UnitStats
// ===============================

//size_t UnitStats::damageMultiplierCount = 0;

// ==================== misc ====================

void UnitStats::reset( ) {
	m_maxHp = 0;
	m_hpRegeneration = 0;
	m_maxEp = 0;
	m_epRegeneration = 0;
	m_sight = 0;
	m_armor = 0;

	m_attackStrength = 0;
	m_effectStrength = 0;
	m_attackPctStolen = 0;
	m_attackRange = 0;
	m_moveSpeed = 0;
	m_attackSpeed = 0;
	m_prodSpeed = 0;
	m_repairSpeed = 0;
	m_harvestSpeed = 0;
}

void UnitStats::setValues( const UnitStats &o) {
	m_maxHp = o.m_maxHp;
	m_hpRegeneration = o.m_hpRegeneration;
	m_maxEp = o.m_maxEp;
	m_epRegeneration = o.m_epRegeneration;
	m_sight = o.m_sight;
	m_armor = o.m_armor;

	m_attackStrength = o.m_attackStrength;
	m_effectStrength = o.m_effectStrength;
	m_attackPctStolen = o.m_attackPctStolen;
	m_attackRange = o.m_attackRange;
	m_moveSpeed = o.m_moveSpeed;
	m_attackSpeed = o.m_attackSpeed;
	m_prodSpeed = o.m_prodSpeed;
	m_repairSpeed = o.m_repairSpeed;
	m_harvestSpeed = o.m_harvestSpeed;
}

void UnitStats::addStatic( const EnhancementType &e, fixed strength ) {
	m_maxHp += (e.getMaxHp( ) * strength).intp( );
	m_hpRegeneration += (e.getHpRegeneration( ) * strength).intp( );
	m_maxEp += (e.getMaxEp( ) * strength).intp( );
	m_epRegeneration += (e.getEpRegeneration( ) * strength).intp( );
	m_sight += (e.getSight( ) * strength).intp( );
	m_armor += (e.getArmor( ) * strength).intp( );
	m_attackStrength += (e.getAttackStrength( ) * strength).intp( );
	m_effectStrength += e.getEffectStrength( ) * strength;
	m_attackPctStolen += e.getAttackPctStolen( ) * strength;
	m_attackRange += (e.getAttackRange( ) * strength).intp( );
	m_moveSpeed += (e.getMoveSpeed( ) * strength).intp( );
	m_attackSpeed += (e.getAttackSpeed( ) * strength).intp( );
	m_prodSpeed += (e.getProdSpeed( ) * strength).intp( );
	m_repairSpeed += (e.getRepairSpeed( ) * strength).intp( );
	m_harvestSpeed += (e.getHarvestSpeed( ) * strength).intp( );
}

void UnitStats::applyMultipliers( const EnhancementType &e ) {
	m_maxHp = (m_maxHp * e.getMaxHpMult( )).intp( );
	m_hpRegeneration = (m_hpRegeneration * e.getHpRegenerationMult( )).intp( );
	m_maxEp = (m_maxEp * e.getMaxEpMult( )).intp( );
	m_epRegeneration = (m_epRegeneration * e.getEpRegenerationMult( )).intp( );
	m_sight = std::max( (m_sight * e.getSightMult( )).intp( ), 1 );
	m_armor = (m_armor * e.getArmorMult( )).intp( );
	m_attackStrength = (m_attackStrength * e.getAttackStrengthMult( )).intp( );
	m_effectStrength = m_effectStrength * e.getEffectStrengthMult( );
	m_attackPctStolen = m_attackPctStolen * e.getAttackPctStolenMult( );
	m_attackRange = (m_attackRange * e.getAttackRangeMult( )).intp( );
	m_moveSpeed = (m_moveSpeed * e.getMoveSpeedMult( )).intp( );
	m_attackSpeed = (m_attackSpeed * e.getAttackSpeedMult( )).intp( );
	m_prodSpeed = (m_prodSpeed * e.getProdSpeedMult( )).intp( );
	m_repairSpeed = (m_repairSpeed * e.getRepairSpeedMult( )).intp( );
	m_harvestSpeed = (m_harvestSpeed * e.getHarvestSpeedMult( )).intp( );
}

void UnitStats::sanitiseUnitStats( ) {
	if (m_maxHp < 0) m_maxHp = 0;
	if (m_maxEp < 0) m_maxEp = 0;
	if (m_sight < 0) m_sight = 0;
	if (m_armor < 0) m_armor = 0;
	if (m_attackStrength < 0) m_attackStrength = 0;
	if (m_effectStrength < 0) m_effectStrength = 0;
	if (m_attackPctStolen < 0) m_attackPctStolen = 0;
	if (m_attackRange < 0) m_attackRange = 0;
	if (m_moveSpeed < 0) m_moveSpeed = 0;
	if (m_attackSpeed < 0) m_attackSpeed = 0;
	if (m_prodSpeed < 0) m_prodSpeed = 0;
	if (m_repairSpeed < 0) m_repairSpeed = 0;
	if (m_harvestSpeed < 0) m_harvestSpeed = 0;
}

// legacy load for Unit class
bool UnitStats::load( const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType ) {
	bool loadOk = true;
	//m_maxHp
	try { m_maxHp = baseNode->getChildIntValue( "max-hp" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	//m_hpRegeneration
	try { m_hpRegeneration = baseNode->getChild( "max-hp" )->getIntAttribute( "regeneration" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	//m_maxEp
	try { m_maxEp = baseNode->getChildIntValue( "max-ep" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	try {
		//m_epRegeneration
		XmlAttribute *epRegenAttr = baseNode->getChild( "max-ep" )->getAttribute( "regeneration", false );
		m_epRegeneration = epRegenAttr ? epRegenAttr->getIntValue( ) : 0;
	}
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	//m_sight
	try { m_sight = baseNode->getChildIntValue( "m_sight" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	//m_armor
	try { m_armor = baseNode->getChildIntValue( "m_armor" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	return loadOk;
}

void UnitStats::doChecksum( Checksum &checksum ) const {
	checksum.add( m_maxHp );
	checksum.add( m_hpRegeneration );
	checksum.add( m_maxEp );
	checksum.add( m_epRegeneration );
	checksum.add( m_sight );
	checksum.add( m_armor );

	checksum.add( m_attackStrength );
	checksum.add( m_effectStrength );
	checksum.add( m_attackPctStolen );
	checksum.add( m_attackRange );
	checksum.add( m_moveSpeed );
	checksum.add( m_attackSpeed );
	checksum.add (m_prodSpeed );
	checksum.add( m_repairSpeed );
	checksum.add( m_harvestSpeed );
}

void UnitStats::save( XmlNode *node ) const {
	node->addChild( "max-ep", m_maxHp );
	node->addChild( "hp-regeneration", m_hpRegeneration );
	node->addChild( "max-ep", m_maxEp );
	node->addChild( "ep-regeneration", m_epRegeneration );
	node->addChild( "m_sight", m_sight );
	node->addChild( "m_armor", m_armor );

	node->addChild( "attack-strength", m_attackStrength );
	node->addChild( "effect-strength", m_effectStrength );
	node->addChild( "attack-percent-stolen", m_attackPctStolen );
	node->addChild( "attack-range", m_attackRange );
	node->addChild( "move-speed", m_moveSpeed );
	node->addChild( "attack-speed", m_attackSpeed );
	node->addChild( "production-speed", m_prodSpeed );
	node->addChild( "repair-speed", m_repairSpeed );
	node->addChild( "harvest-speed", m_harvestSpeed );
}

// ===============================
//  class EnhancementType
// ===============================

// ==================== misc ====================

void EnhancementType::reset( ) {
	UnitStats::reset( );
	m_maxHpMult = 1;
	m_hpRegenerationMult = 1;
	m_maxEpMult = 1;
	m_epRegenerationMult = 1;
	m_sightMult = 1;
	m_armorMult = 1;
	m_attackStrengthMult = 1;
	m_effectStrengthMult = 1;
	m_attackPctStolenMult = 1;
	m_attackRangeMult = 1;
	m_moveSpeedMult = 1;
	m_attackSpeedMult = 1;
	m_prodSpeedMult = 1;
	m_repairSpeedMult = 1;
	m_harvestSpeedMult = 1;
	m_hpBoost = m_epBoost = 0;
}

void EnhancementType::save( XmlNode *node ) const {
	XmlNode *m = node->addChild( "multipliers" );

	m->addChild( "max-ep", m_maxHpMult );
	m->addChild( "hp-regeneration", m_hpRegenerationMult );
	m->addChild( "hp-boost", m_hpBoost );
	m->addChild( "max-ep", m_maxEpMult );
	m->addChild( "ep-regeneration", m_epRegenerationMult );
	m->addChild( "ep-boost", m_epBoost );
	m->addChild( "m_sight", m_sightMult );
	m->addChild( "m_armor", m_armorMult );

	m->addChild( "attack-strength", m_attackStrengthMult );
	m->addChild( "effect-strength", m_effectStrengthMult );
	m->addChild( "attack-percent-stolen", m_attackPctStolenMult );
	m->addChild( "attack-range", m_attackRangeMult );
	m->addChild( "move-speed", m_moveSpeedMult );
	m->addChild( "attack-speed", m_attackSpeedMult );
	m->addChild( "production-speed", m_prodSpeedMult );
	m->addChild( "repair-speed", m_repairSpeedMult );
	m->addChild( "harvest-speed", m_harvestSpeedMult );
}

void EnhancementType::addMultipliers( const EnhancementType &e, fixed strength ) {
	m_maxHpMult += (e.getMaxHpMult( ) - 1) * strength;
	m_hpRegenerationMult += (e.getHpRegenerationMult( ) - 1) * strength;
	m_maxEpMult += (e.getMaxEpMult( ) - 1) * strength;
	m_epRegenerationMult += (e.getEpRegenerationMult( ) - 1) * strength;
	m_sightMult += (e.getSightMult( ) - 1) * strength;
	m_armorMult += (e.getArmorMult( ) - 1) * strength;
	m_attackStrengthMult += (e.getAttackStrengthMult( ) - 1) * strength;
	m_effectStrengthMult += (e.getEffectStrengthMult( ) - 1) * strength;
	m_attackPctStolenMult += (e.getAttackPctStolenMult( ) - 1) * strength;
	m_attackRangeMult += (e.getAttackRangeMult( ) - 1) * strength;
	m_moveSpeedMult += (e.getMoveSpeedMult( ) - 1) * strength;
	m_attackSpeedMult += (e.getAttackSpeedMult( ) - 1) * strength;
	m_prodSpeedMult += (e.getProdSpeedMult( ) - 1) * strength;
	m_repairSpeedMult += (e.getRepairSpeedMult( ) - 1) * strength;
	m_harvestSpeedMult += (e.getHarvestSpeedMult( ) - 1) * strength;
}

void EnhancementType::clampMultipliers( ) {
	fixed low;
	low.raw( ) = (1 << 6); // lowest 'fixed' fraction that is 'safe' to multiply
	if (m_maxHpMult < low) {
		m_maxHpMult = low;
	}
	if (m_hpRegenerationMult < low) {
		m_hpRegenerationMult = low;
	}
	if (m_maxEpMult < low) {
		m_maxEpMult = low;
	}
	if (m_epRegenerationMult < low) {
		m_epRegenerationMult = low;
	}
	if (m_sightMult < low) {
		m_sightMult = low;
	}
	if (m_armorMult < low) {
		m_armorMult = low;
	}
	if (m_attackStrengthMult < low) {
		m_attackStrengthMult = low;
	}
	if (m_effectStrengthMult < low) {
		m_effectStrengthMult = low;
	}
	if (m_attackPctStolenMult < low) {
		m_attackPctStolenMult = low;
	}
	if (m_attackRangeMult < low) {
		m_attackRangeMult = low;
	}
	if (m_attackSpeedMult < low) {
		m_attackSpeedMult = low;
	}
	if (m_moveSpeedMult < low) {
		m_moveSpeedMult = low;
	}
	if (m_prodSpeedMult < low) {
		m_prodSpeedMult = low;
	}
	if (m_repairSpeedMult < low) {
		m_repairSpeedMult = low;
	}
	if (m_harvestSpeedMult < low) {
		m_harvestSpeedMult = low;
	}
}

void formatModifier( string &str, const char *pre, const char* label, int value, fixed multiplier ) {
	Lang &lang = Lang::getInstance( );

	if (value) {
		str += pre + lang.get( label ) + ": ";
		if (value > 0) {
			str += "+";
		}
		str += intToStr( value );
	}

	if (multiplier != 1) {
		if (value) {
			str += ", ";
		} else {
			str += pre + lang.get( label ) + ": ";
		}

		if (multiplier > 1) {
			str += "+";
		}
		str += intToStr( ((multiplier - 1) * 100).intp( ) ) + "%";
	}
}

void addBoostsDesc( string &str, const char *pre, int m_hpBoost, int m_epBoost ) {
	if (m_hpBoost) {
		str += pre + g_lang.get( "HpBoost" ) + ": " + intToStr( m_hpBoost );
	}
	if (m_epBoost) {
		str += pre + g_lang.get( "EpBoost" ) + ": " + intToStr( m_epBoost );
	}
}

void EnhancementType::getDesc( string &str, const char *pre ) const {
	formatModifier( str, pre, "Hp", m_maxHp, m_maxHpMult );
	formatModifier( str, pre, "HpRegeneration", m_hpRegeneration, m_hpRegenerationMult );
	formatModifier( str, pre, "Sight", m_sight, m_sightMult );
	formatModifier( str, pre, "Ep", m_maxEp, m_maxEpMult );
	formatModifier( str, pre, "EpRegeneration", m_epRegeneration, m_epRegenerationMult );
	formatModifier( str, pre, "AttackStrength", m_attackStrength, m_attackStrengthMult );
	formatModifier( str, pre, "EffectStrength", (m_effectStrength * 100).intp( ), m_effectStrengthMult );
	formatModifier( str, pre, "AttackPctStolen", (m_attackPctStolen * 100).intp( ), m_attackPctStolenMult );
	formatModifier( str, pre, "AttackSpeed", m_attackSpeed, m_attackSpeedMult );
	formatModifier( str, pre, "AttackDistance", m_attackRange, m_attackRangeMult );
	formatModifier( str, pre, "Armor", m_armor, m_armorMult );
	formatModifier( str, pre, "WalkSpeed", m_moveSpeed, m_moveSpeedMult );
	formatModifier( str, pre, "ProductionSpeed", m_prodSpeed, m_prodSpeedMult );
	formatModifier( str, pre, "RepairSpeed", m_repairSpeed, m_repairSpeedMult );
	formatModifier( str, pre, "HarvestSpeed", m_harvestSpeed, m_harvestSpeedMult );
	addBoostsDesc( str, pre, m_hpBoost, m_epBoost );
}

//Initialize value from <static-modifiers>
void EnhancementType::initStaticModifier( const XmlNode *node, const string &dir ) {
	const string &name = node->getName( );
	int value = node->getAttribute( "value" )->getIntValue( );

	if (name == "max-hp") {
		m_maxHp = value;
	} else if (name == "max-ep") {
		m_maxEp = value;
	} else if (name == "hp-regeneration") {
		m_hpRegeneration = value;
	} else if (name == "ep-regeneration") {
		m_epRegeneration = value;
	} else if (name == "m_sight") {
		m_sight = value;
	} else if (name == "m_armor") {
		m_armor = value;
	} else if (name == "attack-strength") {
		m_attackStrength = value;
	} else if (name == "effect-strength") {
		m_effectStrength = fixed(value) / 100;
	} else if (name == "attack-percent-stolen") {
		m_attackPctStolen	= fixed(value) / 100;
	} else if (name == "attack-range") {
		m_attackRange = value;
	} else if (name == "move-speed") {
		m_moveSpeed = value;
	} else if (name == "attack-speed") {
		m_attackSpeed = value;
	} else if (name == "production-speed") {
		m_prodSpeed = value;
	} else if (name == "repair-speed") {
		m_repairSpeed = value;
	} else if (name == "harvest-speed") {
		m_harvestSpeed = value;
	} else {
		throw runtime_error( "Not a valid child of <static-modifiers>: " + name + ": " + dir );
	}
}

//Initialize value from <multipliers>
void EnhancementType::initMultiplier( const XmlNode *node, const string &dir ) {
	const string &name = node->getName( );
	fixed value = node->getAttribute( "value" )->getFixedValue( );

	if (name == "max-hp") {
		m_maxHpMult = value;
	} else if (name == "max-ep") {
		m_maxEpMult = value;
	} else if (name == "hp-regeneration") {
		m_hpRegenerationMult = value;
	} else if (name == "ep-regeneration") {
		m_epRegenerationMult = value;
	} else if (name == "m_sight") {
		m_sightMult = value;
	} else if (name == "m_armor") {
		m_armorMult = value;
	} else if (name == "attack-strength") {
		m_attackStrengthMult = value;
	} else if (name == "effect-strength") {
		m_effectStrengthMult = value;
	} else if (name == "attack-percent-stolen") {
		m_attackPctStolenMult = value;
	} else if (name == "attack-range") {
		m_attackRangeMult = value;
	} else if (name == "move-speed") {
		m_moveSpeedMult = value;
	} else if (name == "attack-speed") {
		m_attackSpeedMult = value;
	} else if (name == "production-speed") {
		m_prodSpeedMult = value;
	} else if (name == "repair-speed") {
		m_repairSpeedMult = value;
	} else if (name == "harvest-speed") {
		m_harvestSpeedMult = value;
	} else {
		throw runtime_error( "Not a valid child of <multipliers>: <" + name + ">: " + dir );
	}
}

bool EnhancementType::load( const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft ) {
	const XmlNode *node;
	bool loadOk = true;
	// static modifiers
	try {
		node = baseNode->getChild( "static-modifiers", 0, false );
		if(node) {
			for (int i = 0; i < node->getChildCount( ); ++i) {
				initStaticModifier( node->getChild( i ), dir );
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	// multipliers
	try {
		node = baseNode->getChild( "multipliers", 0, false );
		if (node) {
			for (int i = 0; i < node->getChildCount( ); ++i) {
				initMultiplier( node->getChild( i ), dir );
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	// stat boost
	try {
		node = baseNode->getChild( "point-boosts", 0, false );
		if (node) {
			m_hpBoost = node->getOptionalIntValue( "hp-boost" );
			m_epBoost = node->getOptionalIntValue( "ep-boost" );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	return loadOk;
}

void EnhancementType::doChecksum( Checksum &checksum ) const {
	UnitStats::doChecksum( checksum );
	checksum.add( m_maxHpMult );
	checksum.add( m_hpRegenerationMult );
	checksum.add( m_hpBoost );
	checksum.add( m_maxEpMult );
	checksum.add( m_epRegenerationMult );
	checksum.add( m_epBoost );
	checksum.add( m_sightMult );
	checksum.add( m_armorMult );
	checksum.add( m_attackStrengthMult );
	checksum.add( m_effectStrengthMult );
	checksum.add( m_attackPctStolenMult );
	checksum.add( m_attackRangeMult );
	checksum.add( m_moveSpeedMult );
	checksum.add( m_attackSpeedMult );
	checksum.add( m_prodSpeedMult );
	checksum.add( m_repairSpeedMult );
	checksum.add( m_harvestSpeedMult );
}

}}//end namespace
