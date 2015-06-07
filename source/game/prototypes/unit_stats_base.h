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

#ifndef _GLEST_GAME_UNIT_STAT_BASE_H_
#define _GLEST_GAME_UNIT_STAT_BASE_H_

#include <cassert>
#include <stdexcept>
#include "vec.h"
#include "damage_multiplier.h"
#include "xml_parser.h"
#include "conversion.h"
#include "util.h"
#include "flags.h"
#include "game_constants.h"
#include "prototypes_enums.h"

using std::runtime_error;

using namespace Shared::Graphics;
using namespace Shared::Xml;
using namespace Shared::Util;
using namespace Glest::Sim;

namespace Glest { namespace ProtoTypes {

class UpgradeType;

/** Fields of travel, and indirectly zone of occupance */
class Fields : public XmlBasedFlags<Field, Field::COUNT> {
public:
	void load( const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft ) {
		XmlBasedFlags<Field, Field::COUNT>::load( node, dir, tt, ft, "field", FieldNames );
	}
};

///@todo remove the need for this hacky crap
inline Field dominantField( const Fields &fields ) {
	Field f = Field::INVALID;
	if (fields.get( Field::LAND )) f = Field::LAND;
	else if (fields.get( Field::AIR )) f = Field::AIR;
	if (fields.get( Field::AMPHIBIOUS )) f = Field::AMPHIBIOUS;
	else if (fields.get( Field::ANY_WATER )) f = Field::ANY_WATER;
	else if (fields.get( Field::DEEP_WATER )) f = Field::DEEP_WATER;
	return f;
}

/** Zones of attack (air, surface, etc.) */
class Zones : public XmlBasedFlags<Zone, Zone::COUNT> {
public:
	void load( const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft ) {
		XmlBasedFlags<Zone, Zone::COUNT>::load( node, dir, tt, ft, "field", ZoneNames );
	}
};

// ==============================================================
// 	enum Property & class UnitProperties
// ==============================================================

/** Properties that can be applied to a unit. */
class UnitProperties : public XmlBasedFlags<Property, Property::COUNT> {
private:
	//static const char *names[pCount];

public:
	void load( const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft ) {
		XmlBasedFlags<Property, Property::COUNT>::load( node, dir, tt, ft, "property", PropertyNames );
	}
};

// ===============================
// 	class UnitStats
// ===============================

/** Base stats for a unit type, upgrade, effect or other enhancement. */
class UnitStats {

	friend class UpgradeType; // hack, needed to support old style Upgrades in a new style world :(

protected:
	int m_maxHp;
	int m_hpRegeneration;
    int m_maxEp;
	int m_epRegeneration;
    int m_sight;
	int m_armor;

	// skill mods
	int m_attackStrength;
	fixed m_effectStrength;
	fixed m_attackPctStolen;
	int m_attackRange;
	int m_moveSpeed;
	int m_attackSpeed;
	int m_prodSpeed;
	int m_repairSpeed;
	int m_harvestSpeed;

	void setMaxHp( int v )             { m_maxHp = v;           }
	void setHpRegeneration( int v )    { m_hpRegeneration = v;  }
	void setMaxEp( int v )             { m_maxEp = v;           }
	void setEpRegeneration( int v )    { m_epRegeneration = v;  }
	void setSight( int v )             { m_sight = v;           }
	void setArmor( int v )             { m_armor = v;           }

	void setAttackStrength( int v )    { m_attackStrength = v;  }
	void setEffectStrength( fixed v )  { m_effectStrength = v;  }
	void setAttackPctStolen( fixed v ) { m_attackPctStolen = v; }
	void setAttackRange( int v )       { m_attackRange = v;     }
	void setMoveSpeed( int v )         { m_moveSpeed = v;       }
	void setAttackSpeed( int v )       { m_attackSpeed = v;     }
	void setProdSpeed( int v )         { m_prodSpeed = v;       }
	void setRepairSpeed( int v )       { m_repairSpeed = v;     }
	void setHarvestSpeed( int v )      { m_harvestSpeed = v;    }

public:
	UnitStats( )          { memset( this, 0, sizeof( *this ) ); }
	virtual ~UnitStats( ) { }

	virtual void doChecksum( Checksum &checksum ) const;

	// ==================== get ====================

	int getMaxHp( ) const                   { return m_maxHp;           }
	int getHpRegeneration( ) const          { return m_hpRegeneration;  }
	int getMaxEp( ) const                   { return m_maxEp;           }
	int getEpRegeneration( ) const          { return m_epRegeneration;  }
	int getSight( ) const                   { return m_sight;           }
	int getArmor( ) const                   { return m_armor;           }

	int getAttackStrength( ) const          { return m_attackStrength;  }
	fixed getEffectStrength( ) const        { return m_effectStrength;  }
	fixed getAttackPctStolen( ) const       { return m_attackPctStolen; }
	int getAttackRange( ) const             { return m_attackRange;     }
	int getMoveSpeed( ) const               { return m_moveSpeed;       }
	int getAttackSpeed( ) const             { return m_attackSpeed;     }
	int getProdSpeed( ) const               { return m_prodSpeed;       }
	int getRepairSpeed( ) const             { return m_repairSpeed;     }
	int getHarvestSpeed( ) const            { return m_harvestSpeed;    }

	// ==================== misc ====================

	/** Resets the values of all fields to zero or other base value. */
	virtual void reset( );

	/** Initialize the object from an XmlNode object. It is important to note
	  * that all xxxSpeed and m_attackRange variables are not initialized by this
	  * function. This is essentially the portions of the load method of the
	  * legacy UnitType class that appeared under the <properties> node and that
	  * is exactly what XmlNode object the UnitType load( ) method supplies to
	  * this method. */
	bool load( const XmlNode *parametersNode, const string &dir, const TechTree *tt, const FactionType *ft );

	virtual void save( XmlNode *node ) const;

	/** Equivilant to an assignment operator; initializes values based on supplied object. */
	void setValues( const UnitStats &us );

	/** Apply all the multipliers in the supplied EnhancementType to the
	  * applicable static value (i.e., addition/subtraction values) in this
	  * object. */
	void applyMultipliers( const EnhancementType &e );

	/** Add all static values (i.e., addition/subtraction values) in to this
	  * object, using the supplied multiplier strength before adding. I.e., stat =
	  * e.stat * strength. */
	void addStatic( const EnhancementType &e, fixed strength = 1 );

	/** re-adjust values for unit entities, enforcing minimum sensible values */
	void sanitiseUnitStats( );
};

// ===============================
// 	class EnhancementType
// ===============================

/** An extension of UnitStats, which contains values suitable for an
  * addition/subtraction alteration to a Unit's stats, that also has a multiplier
  * for each of those stats.  This is the base class for both UpgradeType and
  * EffectType. */
class EnhancementType : public UnitStats {
protected:
	fixed m_maxHpMult;
	fixed m_hpRegenerationMult;
	fixed m_maxEpMult;
	fixed m_epRegenerationMult;
	fixed m_sightMult;
	fixed m_armorMult;

	fixed m_attackStrengthMult;
	fixed m_effectStrengthMult;
	fixed m_attackPctStolenMult;
	fixed m_attackRangeMult;

	fixed m_moveSpeedMult;
	fixed m_attackSpeedMult;
	fixed m_prodSpeedMult;
	fixed m_repairSpeedMult;
	fixed m_harvestSpeedMult;

	int m_hpBoost;
	int m_epBoost;

public:
	EnhancementType( ) {
		reset( );
	}

	fixed getMaxHpMult( ) const             { return m_maxHpMult;           }
	fixed getHpRegenerationMult( ) const    { return m_hpRegenerationMult;  }
	int   getHpBoost( ) const               { return m_hpBoost;             }
	fixed getMaxEpMult( ) const             { return m_maxEpMult;           }
	fixed getEpRegenerationMult( ) const    { return m_epRegenerationMult;  }
	int   getEpBoost( ) const               { return m_epBoost;             }
	fixed getSightMult( ) const             { return m_sightMult;           }
	fixed getArmorMult( ) const             { return m_armorMult;           }
	fixed getAttackStrengthMult( ) const    { return m_attackStrengthMult;  }
	fixed getEffectStrengthMult( ) const    { return m_effectStrengthMult;  }
	fixed getAttackPctStolenMult( ) const   { return m_attackPctStolenMult; }
	fixed getAttackRangeMult( ) const       { return m_attackRangeMult;     }
	fixed getMoveSpeedMult( ) const         { return m_moveSpeedMult;       }
	fixed getAttackSpeedMult( ) const       { return m_attackSpeedMult;     }
	fixed getProdSpeedMult( ) const         { return m_prodSpeedMult;       }
	fixed getRepairSpeedMult( ) const       { return m_repairSpeedMult;     }
	fixed getHarvestSpeedMult( ) const      { return m_harvestSpeedMult;    }

	/**
	 * Adds multipliers, normalizing and adjusting for strength. The formula
	 * used to calculate the new value for each multiplier field is "field +=
	 * (e.field - 1.0f) * strength". This effectively causes the original
	 * multiplier for each field to be adjusted by the deviation from the value
	 * 1.0f of each multiplier in the supplied object e.
	 */
	void addMultipliers( const EnhancementType &e, fixed strength = 1 );

	void clampMultipliers( );

	/**
	 * Resets all multipliers to 1.0f and all base class members to their
	 * appropriate default values (0, NULL, etc.).
	 */
	virtual void reset( );

	/**
	 * Returns a string description of this object, only supplying information
	 * on fields which cause a modification.
	 */
	virtual void getDesc( string &str, const char *prefix ) const;

	/**
	 * Initializes this object from the specified XmlNode object.
	 */
	virtual bool load( const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft );

	virtual void doChecksum( Checksum &checksum ) const;

	virtual void save( XmlNode *node ) const;

	/**
	 * Appends a uniform description of the supplied value, if non-zero.
	 * Essentially, this will contain either a + or - sign followed by the
	 * value, unless the value is zero and is primarily used as an inline
	 * function to keep redundant clutter out of other functions which provide
	 * descriptions.
	 */
	static void describeModifier( string &str, int value ) {
		if (value != 0) {
			if (value > 0) {
				str += "+";
			}
			str += intToStr( value );
		}
	}

	void sum( const EnhancementType *enh ) {
		addStatic( *enh );
		addMultipliers( *enh );
	}

private:
	/** Initialize value from <static-modifiers> node */
	void initStaticModifier( const XmlNode *node, const string &dir );

	/** Initialize value from <multipliers> node */
	void initMultiplier( const XmlNode *node, const string &dir );
};



}}//end namespace

#endif
