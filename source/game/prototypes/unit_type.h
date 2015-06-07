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

#ifndef _GLEST_GAME_UNITTYPE_H_
#define _GLEST_GAME_UNITTYPE_H_

#include "cloak_type.h"
#include "command_type.h"
#include "damage_multiplier.h"
#include "sound_container.h"
#include "checksum.h"
#include "particle_type.h"
#include <set>
using std::set;

using Shared::Sound::StaticSound;
using Shared::Util::Checksum;
using Shared::Util::MultiFactory;

namespace Glest { namespace ProtoTypes {
using namespace Search;

// ===============================
// 	class Level
// ===============================

class Level: public EnhancementType, public NameIdPair {
private:
	int m_killsRequired;

public:
	Level( ) : EnhancementType( ) {
		const fixed onePointFive = fixed( 3 ) / 2;
		m_maxHpMult = onePointFive;
		m_maxEpMult = onePointFive;
		m_sightMult = fixed( 6 ) / 5;
		m_armorMult = onePointFive;
		m_effectStrength = fixed( 1 ) / 10;
	}

	virtual bool load( const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft );
	virtual void doChecksum( Checksum &checksum ) const {
		NameIdPair::doChecksum( checksum );
		EnhancementType::doChecksum( checksum );
		checksum.add( m_killsRequired );
	}
	int getKills( ) const			{ return m_killsRequired; }
};

Vec2i rotateCellOffset( const Vec2i &offsetconst, const int unitSize, const CardinalDir facing );


// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================

class UnitType : public ProducibleType, public UnitStats {
private:
	typedef vector<SkillType*>          SkillTypes;
	typedef vector<CommandType*>        CommandTypes;
	typedef vector<ResourceAmount>      StoredResources;
	typedef vector<Level>               Levels;
	typedef vector<ParticleSystemType*> ParticleSystemTypes;

	//typedef vector<PetRule*> PetRules;
	//typedef map<int, const CommandType*> CommandTypeMap;

private:
	//basic
	bool m_multiBuild;
	bool m_multiSelect;

	const ArmourType *m_armourType;

    /** size in cells square (i.e., size x size) */
    int m_size;
    int m_height;

	bool m_light;
    Vec3f m_lightColour;

	CloakType	  *m_cloakType;
	DetectorType  *m_detectorType;

	set<string> m_tags;

	//sounds
	SoundContainer m_selectionSounds;
	SoundContainer m_commandSounds;

	//info
	StoredResources m_storedResources;
	Levels m_levels;
	Emanations m_emanations;

	//meeting point
	bool m_meetingPoint;
	Texture2D *m_meetingPointImage;

	CommandTypes m_commandTypes;
	CommandTypes m_commandTypesByClass[CmdClass::COUNT]; // command types mapped by CmdClass

	SkillTypes m_skillTypes;
	SkillTypes m_skillTypesByClass[SkillClass::COUNT];

	SkillType *m_startSkill;

	fixed m_halfSize;
	fixed m_halfHeight;

	PatchMap<1> *m_cellMap;
	PatchMap<1> *m_colourMap; // 'footprint' on minimap

	UnitProperties m_properties;
	Field m_field;
	Zone m_zone;
	bool m_hasProjectileAttack;

	const FactionType *m_factionType;

public:
	static ProducibleClass typeClass( ) { return ProducibleClass::UNIT; }

public:
	//creation and loading
	UnitType( );
	virtual ~UnitType( );
	void preLoad( const string &dir );
	bool load( const string &dir, const TechTree *techTree, const FactionType *factionType, bool glestimal = false );
	void addBeLoadedCommand( );
	virtual void doChecksum( Checksum &checksum ) const;
	const FactionType* getFactionType( ) const { return m_factionType; }

	ProducibleClass getClass( ) const override { return typeClass( ); }

	CloakClass getCloakClass( ) const {
		return m_cloakType ? m_cloakType->getClass( ) : CloakClass( CloakClass::INVALID );
	}
	const Texture2D* getCloakImage( ) const {
		return m_cloakType ? m_cloakType->getImage( ) : nullptr;
	}
	const CloakType* getCloakType( ) const       { return m_cloakType; }
	const DetectorType* getDetectorType( ) const { return m_detectorType; }
	bool isDetector( ) const                     { return m_detectorType ? true : false; }

	//get
	const Model *getIdleAnimation( ) const	 { return getFirstStOfClass( SkillClass::STOP )->getAnimation( ); }
	bool getMultiSelect( ) const			 { return m_multiSelect; }
	const ArmourType *getArmourType( ) const { return m_armourType; }
	bool getLight( ) const					 { return m_light; }
	Vec3f getLightColour( ) const			 { return m_lightColour; }
	int getSize( ) const					 { return m_size; }
	int getHeight( ) const					 { return m_height; }
	Field getField( ) const					 { return m_field; }
	Zone getZone( ) const					 { return m_zone; }
	bool hasTag( const string &tag ) const	 { return m_tags.find( tag ) != m_tags.end( ); }

	const UnitProperties &getProperties( ) const { return m_properties; }
	bool getProperty( Property property ) const	 { return m_properties.get( property ); }

	const SkillType *getStartSkill( ) const				{ return m_startSkill; }

	int getSkillTypeCount( ) const						{ return m_skillTypes.size( ); }
	const SkillType *getSkillType( int i ) const		{ return m_skillTypes[i]; }
	const MoveSkillType *getFirstMoveSkill( ) const		{
		return m_skillTypesByClass[SkillClass::MOVE].empty( ) ? nullptr 
			: static_cast<const MoveSkillType *>( m_skillTypesByClass[SkillClass::MOVE].front( ) );
	}

	int getCommandTypeCount( ) const						{ return m_commandTypes.size( ); }
	const CommandType *getCommandType( int i ) const		{ return m_commandTypes[i]; }
	const CommandType *getCommandType( const string &name ) const;
	
	template <typename ConcreteType>
	int getCommandTypeCount( ) const {
		return m_commandTypesByClass[ConcreteType::typeClass( )].size( );
	}
	template <typename ConcreteType>
	const ConcreteType* getCommandType( int i ) const {
		return static_cast<const ConcreteType*>( m_commandTypesByClass[ConcreteType::typeClass( )][i] );
	}
	const CommandTypes& getCommandTypes( CmdClass cc ) const {
		return m_commandTypesByClass[cc];
	}
	const CommandType *getFirstCtOfClass( CmdClass cc ) const {
		return m_commandTypesByClass[cc].empty( ) ? nullptr : m_commandTypesByClass[cc].front( );
	}
    const HarvestCommandType *getHarvestCommand( const ResourceType *resourceType ) const;
	const AttackCommandType *getAttackCommand( Zone zone ) const;
	const RepairCommandType *getRepairCommand( const UnitType *repaired ) const;

	const Level *getLevel( int i ) const        { return &m_levels[i];     }
	int getLevelCount( ) const                  { return m_levels.size( ); }
//	const PetRules &getPetRules( ) const        { return petRules;         }
	const Emanations &getEmanations( ) const    { return m_emanations;     }
	bool isMultiBuild( ) const                  { return m_multiBuild;     }
	fixed getHalfSize( ) const                  { return m_halfSize;       }
	fixed getHalfHeight( ) const                { return m_halfHeight;     }
	bool isMobile( ) const {
		const SkillType *st = getFirstStOfClass( SkillClass::MOVE );
		return st && st->getBaseSpeed( ) > 0 ? true: false;
	}

	//cellmap
	bool getCellMapCell( Vec2i pos, CardinalDir facing ) const;
	bool getCellMapCell( int x, int y, CardinalDir facing ) const	{ return getCellMapCell( Vec2i( x, y ), facing ); }

	const PatchMap<1>& getMinimapFootprint( ) const { return *m_colourMap; }

	// resources
	int getStoredResourceCount( ) const { return m_storedResources.size( ); }
	ResourceAmount getStoredResource( int i, const Faction *f ) const;
	int getStore( const ResourceType *rt, const Faction *f ) const;

	// meeting point
	bool hasMeetingPoint( ) const            { return m_meetingPoint; }
	Texture2D *getMeetingPointImage( ) const { return m_meetingPointImage; }

	// sounds
	StaticSound *getSelectionSound( ) const  { return m_selectionSounds.getRandSound( ); }
	StaticSound *getCommandSound( ) const    { return m_commandSounds.getRandSound( ); }

	const SkillType *getSkillType( const string &skillName, SkillClass skillClass = SkillClass::COUNT ) const;
	const SkillType *getFirstStOfClass( SkillClass sc ) const {
		return m_skillTypesByClass[sc].empty( ) ? nullptr : m_skillTypesByClass[sc].front( );
	}

	// has
	bool hasCommandType( const CommandType *ct ) const;
	bool hasCommandClass( CmdClass cc ) const		{ return !m_commandTypesByClass[cc].empty( ); }
	bool hasSkillType( const SkillType *skillType ) const;
	bool hasSkillClass( SkillClass skillClass ) const;
	bool hasCellMap( ) const           { return m_cellMap != nullptr; }
	bool hasProjectileAttack( ) const  { return m_hasProjectileAttack; }

	// is
	bool isOfClass( UnitClass uc ) const;

private:
	void setDeCloakSkills( const vector<string> &names, const vector<SkillClass> &classes );
    void sortSkillTypes( );
    void sortCommandTypes( );
};

}}//end namespace


#endif
