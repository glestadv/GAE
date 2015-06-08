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

#ifndef _GLEST_GAME_SKILLTYPE_H_
#define _GLEST_GAME_SKILLTYPE_H_

#include "sound.h"
#include "xml_parser.h"
#include "vec.h"
#include "model.h"
#include "lang.h"
#include "flags.h"
#include "factory.h"
#include <set>
using std::set;

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;
using Shared::Graphics::Model;
using Shared::Util::MultiFactory;

#include "damage_multiplier.h"
#include "effect_type.h"
#include "sound_container.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "entities_enums.h"
#include "particle_type.h"	// forward decs


namespace Glest {

using namespace Sound;
using Global::Lang;
using Sim::CycleInfo;
using Entities::Unit;

namespace ProtoTypes {

// =====================================================
// 	class SkillType
//
///	A basic action that an unit can perform
///@todo revise, these seem to hold data/properties for 
/// actions rather than the actions themselves.
// =====================================================

class SkillType : public NameIdPair {
	friend class DynamicTypeFactory<SkillClass, SkillType>;

public:
	typedef vector<Model *> Animations;
	typedef map<SurfaceType, Model*>         AnimationBySurface;
	typedef pair<SurfaceType, SurfaceType>   SurfacePair;
	typedef map<SurfacePair, Model*>         AnimationBySurfacePair;

	WRAPPED_ENUM( AnimationsStyle,
		SINGLE,
		SEQUENTIAL,
		RANDOM
	)

protected:
	// protected data... should be private...
	///@todo privatise
	int m_epCost;
	int m_speed;
	int m_animSpeed;
	Animations m_animations;
	AnimationsStyle m_animationsStyle;
	AnimationBySurface     m_animBySurfaceMap;
	AnimationBySurfacePair m_animBySurfPairMap;
	SoundContainer m_sounds;
	float m_soundStartTime;
	const char* m_typeName;
	int m_minRange; // Refactor? Push down? Used for anything other than attack?
	int m_maxRange; // ditto?
	bool m_deCloak; // does this skill cause de-cloak

	float m_startTime;

	bool m_projectile;
	ProjectileType* m_projectileParticleSystemType;
	SoundContainer m_projSounds;

	bool m_splash;
	bool m_splashDamageAll;
	int m_splashRadius;
	SplashType* m_splashParticleSystemType;

	EffectTypes m_effectTypes;
	UnitParticleSystemTypes m_eyeCandySystems;
	const UnitType *m_unitType;

public:
	//varios
	SkillType( const char* typeName );
	virtual ~SkillType( );
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ft );
	virtual void doChecksum( Checksum &checksum ) const;
	const UnitType* getUnitType( ) const { return m_unitType; }
	virtual void getDesc( string &str, const Unit *unit ) const = 0;
	void descEffects( string &str, const Unit *unit ) const;
	//void descEffectsRemoved( string &str, const Unit *unit ) const;
	void descSpeed( string &str, const Unit *unit, const char* speedType ) const;
	void descRange( string &str, const Unit *unit, const char* rangeDesc ) const;

	void descEpCost( string &str, const Unit *unit ) const {
		if (m_epCost) {
			str += g_lang.get( "EpCost" ) + ": " + intToStr( m_epCost ) + "\n";
		}
	}

	CycleInfo calculateCycleTime( ) const;

	//get
	virtual SkillClass getClass( ) const = 0;
	const string &getName( ) const                   { return m_name;      }
	int getEpCost( ) const                           { return m_epCost;    }
	int getBaseSpeed( ) const                        { return m_speed;     }
	virtual fixed getSpeed( const Unit *unit) const	 { return m_speed;     }
	int getAnimSpeed( ) const                        { return m_animSpeed; }
	virtual bool isStretchyAnim( ) const             { return false;       }
	const Model *getAnimation( SurfaceType from, SurfaceType to ) const;
	const Model *getAnimation( SurfaceType st ) const;
	const Model *getAnimation( ) const               { return m_animations.front( );     }
	StaticSound *getSound( ) const                   { return m_sounds.getRandSound( );  }
	float getSoundStartTime( ) const                 { return m_soundStartTime;          }
	int getMaxRange( ) const                         { return m_maxRange;                }
	int getMinRange( ) const                         { return m_minRange;                }
	float getStartTime( ) const                      { return m_startTime;               }
	bool causesDeCloak( ) const                      { return m_deCloak;                 }
	unsigned getEyeCandySystemCount( ) const         { return m_eyeCandySystems.size( ); }
	const UnitParticleSystemType* getEyeCandySystem( unsigned i ) const {
		assert( i < m_eyeCandySystems.size( ) );
		return m_eyeCandySystems[i];
	}

	// set
	void setDeCloak( bool v ) { m_deCloak = v; }

	//other
	virtual string toString( ) const { return Lang::getInstance( ).get( m_typeName ); }

	///REFACTOR: push-down
	//get proj
	bool getProjectile( ) const                     { return m_projectile;                    }
	ProjectileType * getProjParticleType( ) const   { return m_projectileParticleSystemType;  }
	StaticSound *getProjSound( ) const              { return m_projSounds.getRandSound( );    }

	//get splash
	bool getSplash( ) const                         { return m_splash;                   }
	bool getSplashDamageAll( ) const                { return m_splashDamageAll;          }
	int getSplashRadius( ) const                    { return m_splashRadius;             }
	SplashType * getSplashParticleType( ) const     { return m_splashParticleSystemType; }
	///END REFACTOR

	// get effects
	const EffectTypes &getEffectTypes( ) const  { return m_effectTypes;             }
	bool hasEffects( ) const                    { return m_effectTypes.size( ) > 0; }

	bool hasSounds( ) const   { return !m_sounds.getSounds( ).empty( ); }
};

// ===============================
// 	class StopSkillType
// ===============================

class StopSkillType : public SkillType {
public:
	StopSkillType( ) : SkillType( "Stop" ) { }
	virtual void getDesc( string &str, const Unit *unit ) const override {
		Lang &lang= Lang::getInstance( );
		str += lang.get( "ReactionSpeed" ) + ": " + intToStr(m_speed) + "\n";
		descEpCost( str, unit );
	}
	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::STOP; }
};

// ===============================
// 	class MoveSkillType
// ===============================

class MoveSkillType : public SkillType {
private:
	bool m_visibleOnly;

public:
	MoveSkillType( ) : SkillType( "Move" ), m_visibleOnly( false ) { }
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ft ) override;
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "WalkSpeed" );
		descEpCost( str, unit );
	}
	virtual void doChecksum( Checksum &checksum ) const override {
		SkillType::doChecksum(checksum);
		checksum.add(m_visibleOnly);
	}
	bool getVisibleOnly( ) const { return m_visibleOnly; }
	virtual fixed getSpeed( const Unit *unit ) const override;
	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::MOVE; }
};
/*
class RangedType {
protected:
	int minRange;
	int maxRange;

public:
	RangedType( );
	int getMaxRange( ) const					{ return maxRange; }
	int getMinRange( ) const					{ return minRange; }
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void getDesc( string &str, const Unit *unit, const char* rangeDesc) const;
};
*/
// ===============================
// 	class TargetBasedSkillType
//
/// Base class for both AttackSkillType and CastSpellSkillType
// ===============================

class TargetBasedSkillType : public SkillType {
protected:
	Zones m_zones;

public:
	TargetBasedSkillType( const char* typeName );
	virtual ~TargetBasedSkillType( );
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void doChecksum( Checksum &checksum ) const override;
	virtual void getDesc( string &str, const Unit *unit ) const override { getDesc( str, unit, "Range" ); }
	virtual void getDesc( string &str, const Unit *unit, const char* rangeDesc ) const;

	Zones getZones( ) const                { return m_zones; }
	bool getZone( const Zone zone ) const  { return m_zones.get( zone ); }
};

// ===============================
// 	class AttackSkillType
// ===============================

class AttackSkillType : public TargetBasedSkillType {
private:
	int m_attackStrength;
	int m_attackVar;
	fixed m_attackPctStolen;
	fixed m_attackPctVar;
	const AttackType *m_attackType;
//	EarthquakeType *earthquakeType;

public:
	AttackSkillType( ) : TargetBasedSkillType( "Attack" ), m_attackStrength( 0 ),
		m_attackVar( 0 ), m_attackType( nullptr )/*, earthquakeType(nullptr)*/ {}
	virtual ~AttackSkillType( );

	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void getDesc( string &str, const Unit *unit ) const override;
	virtual void doChecksum( Checksum &checksum ) const override;

	//get
	virtual fixed getSpeed( const Unit *unit ) const override;
	int getAttackStrength( ) const				{ return m_attackStrength; }
	int getAttackVar( ) const					{ return m_attackVar; }
	//fixed getAttackPctStolen( ) const			{ return m_attackPctStolen; }
	//fixed getAttackPctVar( ) const				{ return m_attackPctVar; }
	const AttackType *getAttackType( ) const		{ return m_attackType; }
//	const EarthquakeType *getEarthquakeType( ) const	{ return earthquakeType; }

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::ATTACK; }
};

// ===============================
// 	class BuildSkillType
// ===============================

class BuildSkillType : public SkillType {
public:
	BuildSkillType( ) : SkillType( "Build" ) { }
	void getDesc( string &str, const Unit *unit ) const {
		descSpeed( str, unit, "BuildSpeed" );
		descEpCost( str, unit );
	}

	virtual fixed getSpeed( const Unit *unit ) const override;
	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::BUILD; }
};

// ===============================
// 	class HarvestSkillType
// ===============================

class HarvestSkillType : public SkillType {
public:
	HarvestSkillType( ) : SkillType( "Harvest" ) { }
	virtual void getDesc( string &str, const Unit *unit ) const override {
		// command modifies displayed speed, all handled in HarvestCommandType
	}
	virtual fixed getSpeed( const Unit *unit ) const override;
	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::HARVEST; }
};

// ===============================
// 	class RepairSkillType
// ===============================

class RepairSkillType : public SkillType {
private:
	int m_amount;
	fixed m_multiplier;
	bool m_petOnly;
	bool m_selfOnly;
	bool m_selfAllowed;

public:
	RepairSkillType( );
	virtual ~RepairSkillType( ) { } // { delete splashParticleSystemType; }

	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void doChecksum( Checksum &checksum ) const override;
	virtual void getDesc( string &str, const Unit *unit ) const override;

	virtual fixed getSpeed( const Unit *unit ) const override;
	int getAmount( ) const          { return m_amount; }
	fixed getMultiplier( ) const	{ return m_multiplier; }
	bool isPetOnly( ) const         { return m_petOnly; }
	bool isSelfOnly( ) const        { return m_selfOnly; }
	bool isSelfAllowed( ) const     { return m_selfAllowed; }

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::REPAIR; }
};

// ===============================
// 	class ProduceSkillType
// ===============================

class ProduceSkillType : public SkillType {
private:
	bool m_pet;
	int m_maxPets;

public:
	ProduceSkillType( );
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void doChecksum( Checksum &checksum ) const override;
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "ProductionSpeed" );
		descEpCost( str, unit );
	}
	virtual fixed getSpeed( const Unit *unit) const override;

	bool isPet( ) const		{ return m_pet; }
	int getMaxPets( ) const	{ return m_maxPets; }


	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::PRODUCE; }
};

// ===============================
// 	class UpgradeSkillType
// ===============================

class UpgradeSkillType : public SkillType {
public:
	UpgradeSkillType( ) : SkillType( "Upgrade" ) { }
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "UpgradeSpeed" );
		descEpCost( str, unit );
	}
	virtual fixed getSpeed( const Unit *unit ) const override;

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::UPGRADE; }
};


// ===============================
// 	class BeBuiltSkillType
// ===============================

class BeBuiltSkillType : public SkillType {
private:
	bool  m_stretchy;

public:
	BeBuiltSkillType( ) : SkillType( "Be built" ), m_stretchy(false) { }
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void getDesc( string &str, const Unit *unit ) const override {}
	virtual bool isStretchyAnim( ) const override { return m_stretchy; }
	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::BE_BUILT; }
};

// ===============================
// 	class MorphSkillType
// ===============================

class MorphSkillType : public SkillType {
public:
	MorphSkillType( ) : SkillType( "Morph" ) { }
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "MorphSpeed" );
		descEpCost( str, unit );
	}
	virtual fixed getSpeed( const Unit *unit ) const override;

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::MORPH; }
};

// ===============================
// 	class DieSkillType
// ===============================

class DieSkillType : public SkillType {
private:
	bool m_fade;

public:
	DieSkillType( ) : SkillType( "Die" ), m_fade( false ) { }
	bool getFade( ) const	{ return m_fade; }

	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void doChecksum( Checksum &checksum ) const override;
	virtual void getDesc( string &str, const Unit *unit ) const override { }

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::DIE; }
};

// ===============================
// 	class LoadSkillType
// ===============================

class LoadSkillType : public SkillType {
public:
	LoadSkillType( );
	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void doChecksum( Checksum &checksum ) const override;
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "LoadSpeed" );
		descEpCost( str, unit );
	}

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::LOAD; }
};

// ===============================
// 	class UnloadSkillType
// ===============================

class UnloadSkillType : public SkillType {
public:
	UnloadSkillType( ) : SkillType( "Unload" ) { }
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "UnloadSpeed" );
		descEpCost( str, unit );
	}

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::UNLOAD; }
};

// ===============================
// 	class CastSpellSkillType
// ===============================

class CastSpellSkillType : public SkillType {
public:
	CastSpellSkillType( ) : SkillType( "CastSpell" ) { }

	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "Speed" ); // "CastSpeed" ?
		descEpCost( str, unit );
		descEffects( str, unit );
	}

	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::CAST_SPELL; }

};

// ===============================
// 	class BuildSelfSkillType
// ===============================

class BuildSelfSkillType : public SkillType {
private:
	bool  m_stretchy;

public:
	BuildSelfSkillType( ) : SkillType("BuildSelf"), m_stretchy(false) { }

	virtual void load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) override;
	virtual void getDesc( string &str, const Unit *unit ) const override {
		descSpeed( str, unit, "Speed" );
		descEpCost( str, unit );
		descEffects( str, unit );
	}
	virtual bool isStretchyAnim( ) const override { return m_stretchy; }
	virtual SkillClass getClass( ) const override { return typeClass( ); }
	static SkillClass typeClass( ) { return SkillClass::BUILD_SELF; }

};


// ===============================
// 	class ModelFactory
// ===============================

class ModelFactory {
private:
	typedef map<string, Model*> ModelMap;

private:
	ModelMap m_models;
	int m_idCounter;

	Model* newInstance( const string &path, int size, int height );

public:
	ModelFactory( );
	Model* getModel( const string &path, int size, int height );
};


// ===============================
// 	class AttackSkillPreferences
// ===============================

class AttackSkillPreferences : public XmlBasedFlags<AttackSkillPreference, AttackSkillPreference::COUNT> {
public:
	void load( const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft ) {
		XmlBasedFlags<AttackSkillPreference, AttackSkillPreference::COUNT>::load( node, dir, tt, ft, "flag", AttackSkillPreferenceNames );
	}
};

// ===============================
// 	class AttackSkillTypes
// ===============================

class AttackSkillTypes {
private:
	vector<const AttackSkillType*> m_types;
	vector<AttackSkillPreferences> m_associatedPrefs;
	int m_maxRange;
	Zones m_zones;
	AttackSkillPreferences m_allPrefs;

public:
	void init( );
	int getMaxRange( ) const { return m_maxRange; }
// const vector<const AttackSkillType*> &getTypes( ) const	{ return types; }
	void getDesc( string &str, const Unit *unit ) const;
	bool getZone( Zone zone ) const	 { return m_zones.get( zone ); }
	bool hasPreference( AttackSkillPreference pref ) const { return m_allPrefs.get( pref ); }
	const AttackSkillType *getPreferredAttack( const Unit *unit, const Unit *target, int rangeToTarget ) const;
	const AttackSkillType *getSkillForPref( AttackSkillPreference pref, int rangeToTarget ) const {
		assert( m_types.size( ) == m_associatedPrefs.size( ) );
		for (int i = 0; i < m_types.size( ); ++i) {
			if (m_associatedPrefs[i].get( pref ) && m_types[i]->getMaxRange( ) >= rangeToTarget) {
				return m_types[i];
			}
		}
		return nullptr;
	}

	void push_back( const AttackSkillType* ast, AttackSkillPreferences pref ) {
		m_types.push_back( ast );
		m_associatedPrefs.push_back( pref );
	}

	void doChecksum( Checksum &checksum ) const {
		for (int i=0; i < m_types.size( ); ++i) {
			checksum.add( m_types[i]->getName( ) );
		}
	}
};


}}//end namespace

#endif
