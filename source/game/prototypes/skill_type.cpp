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

#include "pch.h"
#include "skill_type.h"

#include <cassert>

#include "sound.h"
#include "util.h"
#include "lang.h"
#include "renderer.h"
#include "particle_type.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "map.h"
#include "earthquake_type.h"
//#include "network_message.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class SkillType
// =====================================================

SkillType::SkillType( const char* typeName ) 
		: NameIdPair( )
		, m_effectTypes( )
		, m_epCost( 0 )
		, m_animSpeed( 0 )
		, m_minRange( 0 )
		, m_maxRange( 0 )
		, m_deCloak( false )
		, m_startTime(0.f)
		, m_projectile( false )
		, m_projectileParticleSystemType( nullptr )
		, m_splash( false )
		, m_splashDamageAll( false )
		, m_splashRadius( 0 )
		, m_splashParticleSystemType( nullptr )
		, m_animationsStyle( AnimationsStyle::SINGLE )
		, m_soundStartTime( 0.f )
		, m_typeName( typeName )
		, m_unitType( nullptr ) {
}

SkillType::~SkillType(  ) {
	deleteValues( m_effectTypes );
	deleteValues( m_sounds.getSounds( ) );
	deleteValues( m_eyeCandySystems );
	deleteValues( m_projSounds.getSounds( ) );

	///@todo these should have a factory...
	delete m_projectileParticleSystemType;
	delete m_splashParticleSystemType;
}

void SkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	m_unitType = ut;
	const FactionType *ft = ut->getFactionType( );
	m_name = sn->getChildStringValue( "name" );
	m_epCost = sn->getOptionalIntValue( "ep-cost" );
	m_speed = sn->getChildIntValue( "speed" );
	m_minRange = sn->getOptionalIntValue( "min-range", 1 );
	m_maxRange = sn->getOptionalIntValue( "max-range", 1 );
	m_animSpeed = sn->getChildIntValue( "anim-speed" );

	//model
	ModelFactory &modelFactory = g_world.getModelFactory( );
	const XmlNode *animNode = sn->getChild( "animation" );
	const XmlAttribute *animPathAttrib = animNode->getAttribute( "path", false );
	if (animPathAttrib) { // single animation, lagacy style
		string path = dir + "/" + animPathAttrib->getRestrictedValue( );
		m_animations.push_back( modelFactory.getModel( cleanPath( path ), ut->getSize( ), ut->getHeight( ) ) );
		m_animationsStyle = AnimationsStyle::SINGLE;
	} else { // multi-anim or anim-by-surface-type, new style
		for (int i=0; i < animNode->getChildCount( ); ++i) {
			XmlNodePtr node = animNode->getChild( i );
			string path = node->getAttribute( "path" )->getRestrictedValue( );
			path = dir + "/" + path;
			Model *model = modelFactory.getModel( cleanPath( path ), ut->getSize( ), ut->getHeight( ) );
			if (node->getName( ) == "anim-file") {
				m_animations.push_back( model );
			} else if (node->getName( ) == "surface") {
				string type = node->getAttribute( "type" )->getRestrictedValue( );
				SurfaceType st = SurfaceTypeNames.match( type );
				if (st != SurfaceType::INVALID) {
					m_animBySurfaceMap[st] = model;
				} else {
					throw runtime_error( "SurfaceType '" + type + "' invalid." );
				}
			} else if (node->getName( ) == "surface-change") {
				string fromType = node->getAttribute( "from" )->getRestrictedValue( );
				string toType = node->getAttribute( "to" )->getRestrictedValue( );
				SurfaceType from = SurfaceTypeNames.match( fromType );
				SurfaceType to = SurfaceTypeNames.match( toType );
				if (from != SurfaceType::INVALID && to != SurfaceType::INVALID) {
					m_animBySurfPairMap[make_pair( from, to )] = model;
				} else {
					string msg;
					if (from == SurfaceType::INVALID && to == SurfaceType::INVALID) {
						msg = "SurfaceType '" + fromType + "' and '" + toType + "' both invalid.";
					} else if (from != SurfaceType::INVALID) {
						msg = "SurfaceType '" + fromType + "' invalid.";
					} else {
						msg = "SurfaceType '" + toType + "' invalid.";
					}
					throw runtime_error( msg );
				}
			} else {
				// error ?
				g_logger.logXmlError( path, 
					( "Warning: In 'animation' node, child node '" + node->getName( ) + "' unknown." ).c_str( ) );
			}
		}
		if (m_animations.size( ) > 1 ) {
			m_animationsStyle = AnimationsStyle::RANDOM;
		}
		if (!m_animBySurfaceMap.empty( ) || !m_animBySurfPairMap.empty( )) {
			if (m_animBySurfaceMap.size( ) != SurfaceType::COUNT) {

				// error
			}
		}
	}

	//sound
	XmlNodePtr soundNode = sn->getChild( "sound", 0, false );
	if (soundNode && soundNode->getBoolAttribute( "enabled" )) {
		m_soundStartTime = soundNode->getAttribute( "start-time" )->getFloatValue( );
		for (int i=0; i < soundNode->getChildCount( ); ++i) {
			const XmlNode *soundFileNode = soundNode->getChild( "sound-file", i );
			string path= soundFileNode->getAttribute( "path" )->getRestrictedValue( );
			StaticSound *sound= new StaticSound( );
			sound->load( dir + "/" + path );
			m_sounds.add( sound );
		}
	}
	
	// particle systems
	const XmlNode *particlesNode = sn->getOptionalChild( "particle-systems" );
	if (particlesNode) {
		for (int i=0; i < particlesNode->getChildCount( ); ++i) {
			const XmlNode *particleFileNode = particlesNode->getChild( "particle", i );
			string path = particleFileNode->getAttribute( "path" )->getRestrictedValue( );
			UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType( );
			unitParticleSystemType->load( dir,  dir + "/" + path );
			m_eyeCandySystems.push_back( unitParticleSystemType );
		}
	} else { // megaglest style?
		particlesNode = sn->getOptionalChild( "particles" );
		if (particlesNode) {
			bool particleEnabled = particlesNode->getAttribute( "value" )->getBoolValue( );
			if (particleEnabled) {
				for (int i=0; i < particlesNode->getChildCount( ); ++i) {
					const XmlNode *particleFileNode = particlesNode->getChild( "particle-file", i );
					string path = particleFileNode->getAttribute( "path" )->getRestrictedValue( );
					UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType( );
					unitParticleSystemType->load( dir,  dir + "/" + path );
					m_eyeCandySystems.push_back( unitParticleSystemType );
				}
			}
		}
	}
	
	//effects
	const XmlNode *effectsNode = sn->getChild( "effects", 0, false );
	if (effectsNode) {
		m_effectTypes.resize( effectsNode->getChildCount( ) );
		for (int i=0; i < effectsNode->getChildCount( ); ++i) {
			const XmlNode *effectNode = effectsNode->getChild( "effect", i );
			EffectType *effectType = new EffectType( );
			effectType->load( effectNode, dir, tt, ft );
			m_effectTypes[i] = effectType;
		}
	}

	m_startTime = sn->getOptionalFloatValue( "start-time" );

	//projectile
	const XmlNode *projectileNode = sn->getChild( "projectile", 0, false );
	if (projectileNode && projectileNode->getAttribute( "value" )->getBoolValue( )) {

		//proj particle
		const XmlNode *particleNode = projectileNode->getChild( "particle", 0, false );
		if (particleNode && particleNode->getAttribute( "value" )->getBoolValue( ) ) {
			string path = particleNode->getAttribute( "path" )->getRestrictedValue( );
			m_projectileParticleSystemType = new ProjectileType( );
			m_projectileParticleSystemType->load( dir,  dir + "/" + path );
			m_projectile = true;
		}

		//proj sounds
		const XmlNode *soundNode = projectileNode->getChild( "sound", 0, false );
		if (soundNode && soundNode->getAttribute( "enabled" )->getBoolValue( )) {
			for (int i=0; i < soundNode->getChildCount( ); ++i) {
				const XmlNode *soundFileNode = soundNode->getChild( "sound-file", i );
				string path = soundFileNode->getAttribute( "path" )->getRestrictedValue( );
				StaticSound *sound = new StaticSound( );
				sound->load( dir + "/" + path );
				m_projSounds.add( sound );
			}
		}
	}

	//splash damage and/or special effects
	const XmlNode *splashNode = sn->getChild( "splash", 0, false );
	m_splash = splashNode && splashNode->getBoolValue( );
	if (m_splash) {
		m_splashDamageAll = splashNode->getOptionalBoolValue( "damage-all", true );
		m_splashRadius = splashNode->getChild( "radius" )->getAttribute( "value" )->getIntValue( );

		//splash particle
		const XmlNode *particleNode= splashNode->getChild( "particle", 0, false );
		if (particleNode && particleNode->getAttribute( "value" )->getBoolValue( ) ) {
			string path= particleNode->getAttribute( "path" )->getRestrictedValue( );
			m_splashParticleSystemType = new SplashType( );
			m_splashParticleSystemType->load( dir,  dir + "/" + path );
		}
	}
}

void SkillType::doChecksum( Checksum &checksum ) const {
	checksum.add<SkillClass>( getClass( ) );
	checksum.add( m_name );
	checksum.add( m_epCost );
	checksum.add( m_speed );
	checksum.add( m_animSpeed );
	checksum.add( m_minRange );
	checksum.add( m_maxRange );
	checksum.add( m_startTime );
	checksum.add( m_projectile );
	checksum.add( m_splash );
	checksum.add( m_splashDamageAll );
	checksum.add( m_splashRadius );
}

void SkillType::descEffects( string &str, const Unit *unit ) const {
	for (EffectTypes::const_iterator i = m_effectTypes.begin( ); i != m_effectTypes.end( ); ++i) {
		str += g_lang.get( "Effect" ) + ": ";
		(*i)->getDesc( str );
	}
}

void SkillType::descRange( string &str, const Unit *unit, const char* rangeDesc ) const {
	str += Lang::getInstance( ).get( rangeDesc ) + ": ";
	if (m_minRange > 1) {
		str += intToStr( m_minRange ) + "...";
	}
	str += intToStr( m_maxRange );
	EnhancementType::describeModifier( str, unit->getMaxRange( this ) - m_maxRange );
	str += "\n";
}

void SkillType::descSpeed( string &str, const Unit *unit, const char* speedType ) const {
	str += g_lang.get( speedType ) + ": " + intToStr( m_speed );
	EnhancementType::describeModifier( str, unit->getSpeed( this ) - m_speed );
	str += "\n";
}

const Model* SkillType::getAnimation( SurfaceType st ) const {
	AnimationBySurface::const_iterator it = m_animBySurfaceMap.find( st );
	if (it != m_animBySurfaceMap.end( )) {
		return it->second;
	}
	return getAnimation( );
}

const Model* SkillType::getAnimation( SurfaceType from, SurfaceType to ) const {
	AnimationBySurfacePair::const_iterator it = m_animBySurfPairMap.find( make_pair( from, to ) );
	if (it != m_animBySurfPairMap.end( )) {
		return it->second;
	}
	return getAnimation( to );
}

CycleInfo SkillType::calculateCycleTime( ) const {
	const SkillClass skillClass = getClass( );
	static const float speedModifier = 1.f / GameConstants::speedDivider / float( WORLD_FPS );
	if (skillClass == SkillClass::MOVE) {
		return CycleInfo( -1, -1 );
	}
	float progressSpeed = getBaseSpeed( ) * speedModifier;

	int skillFrames = int( 1.0000001f / progressSpeed );
	int animFrames = 1;
	int soundOffset = -1;
	int systemOffset = -1;

	if (getAnimSpeed( ) != 0) {
		float animProgressSpeed = getAnimSpeed( ) * speedModifier;
		animFrames = int(1.0000001f / animProgressSpeed);
	
		if (skillClass == SkillClass::ATTACK || skillClass == SkillClass::CAST_SPELL) {
			systemOffset = clamp( int( m_startTime / animProgressSpeed ), 1, animFrames - 1 );
			assert( systemOffset > 0 && systemOffset < 256 );
		}
		if (!m_sounds.getSounds( ).empty( )) {
			soundOffset = clamp( int( m_soundStartTime / animProgressSpeed ), 1, animFrames - 1 );
			assert( soundOffset > 0 );
		}
	}
	if (skillFrames < 1 ) {
		stringstream ss;
		ss << "Error: UnitType '" << m_unitType->getName( ) << "', SkillType '" << getName( )
			<< ", skill speed is too fast, cycle calculation clamped to one frame.";
		g_logger.logError( ss.str( ) );
		skillFrames = 1;
	}
	assert( animFrames > 0 );
	return CycleInfo( skillFrames, animFrames, soundOffset, systemOffset );
}

// =====================================================
// 	class MoveSkillType
// =====================================================

void MoveSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ft ) {
	SkillType::load( sn, dir, tt, ft );

	XmlNode *visibleOnlyNode = sn->getOptionalChild( "visible-only" );
	if (visibleOnlyNode) {
		m_visibleOnly = visibleOnlyNode->getAttribute( "value" )->getBoolValue( );
	}
}

fixed MoveSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getMoveSpeedMult( ) + unit->getMoveSpeed( );
}

// =====================================================
// 	class TargetBasedSkillType
// =====================================================

TargetBasedSkillType::TargetBasedSkillType( const char* typeName )
		: SkillType( typeName ) {
	m_startTime = 0.0f;
}

TargetBasedSkillType::~TargetBasedSkillType(  ) {
}

void TargetBasedSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	SkillType::load( sn, dir, tt, ut );
	const FactionType *ft = ut->getFactionType( );

	//fields
	const XmlNode *attackFieldsNode = sn->getChild( "attack-fields", 0, false );
	const XmlNode *fieldsNode = sn->getChild( "fields", 0, false );
	if (attackFieldsNode && fieldsNode) {
		throw runtime_error( "Cannot specify both <attack-fields> and <fields>." );
	}
	if (!attackFieldsNode && !fieldsNode) {
		throw runtime_error( "Must specify either <attack-fields> or <fields>." );
	}
	m_zones.load( fieldsNode ? fieldsNode : attackFieldsNode, dir, tt, ft );
}

void TargetBasedSkillType::doChecksum( Checksum &checksum ) const {
	SkillType::doChecksum( checksum );
	foreach_enum (Zone, z) {
		checksum.add<bool>( m_zones.get( z ) );
	}
}

void TargetBasedSkillType::getDesc( string &str, const Unit *unit, const char* rangeDesc ) const {
	Lang &lang = Lang::getInstance( );
	descEpCost( str, unit );
	//splash radius
	if (m_splashRadius) {
		str += lang.get( "SplashRadius" ) + ": " + intToStr( m_splashRadius ) + "\n";
	}
	descRange( str, unit, rangeDesc );
	//fields
	str += lang.get( "Zones" ) + ": ";
	foreach_enum (Zone, z) {
		if (m_zones.get( z )) {
			str += string( lang.get( ZoneNames[z] ) ) + " ";
		}
	}
}

// =====================================================
// 	class AttackSkillType
// =====================================================

AttackSkillType::~AttackSkillType( ) { 
//	delete earthquakeType; 
}

void AttackSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	TargetBasedSkillType::load( sn, dir, tt, ut );

	//misc
	if (sn->getOptionalChild( "attack-strenght" )) { // support vanilla-glest typo
		m_attackStrength = sn->getChild( "attack-strenght" )->getAttribute( "value" )->getIntValue( );
	} else {
		m_attackStrength = sn->getChild( "attack-strength" )->getAttribute( "value" )->getIntValue( );
	}
	m_attackVar = sn->getChild( "attack-var" )->getAttribute( "value" )->getIntValue( );
	m_maxRange = sn->getOptionalIntValue( "attack-range" );
	string attackTypeName = sn->getChild( "attack-type" )->getAttribute( "value" )->getRestrictedValue( );
	m_attackType = tt->getAttackType( attackTypeName );
	m_startTime = sn->getOptionalFloatValue( "attack-start-time" );

	const XmlNode *attackPctStolenNode = sn->getChild( "attack-percent-stolen", 0, false );
	if (attackPctStolenNode) {
		m_attackPctStolen = attackPctStolenNode->getAttribute( "value" )->getFixedValue( ) / 100;
		m_attackPctVar = attackPctStolenNode->getAttribute( "var" )->getFixedValue( ) / 100;
	} else {
		m_attackPctStolen = 0;
		m_attackPctVar = 0;
	}
#ifdef EARTHQUAKE_CODE
	earthquakeType = nullptr;
	XmlNode *earthquakeNode = sn->getChild( "earthquake", 0, false );
	if (earthquakeNode) {
		earthquakeType = new EarthquakeType(float(attackStrength), attackType);
		earthquakeType->load(earthquakeNode, dir, tt, ft);
	}
#endif
}

void AttackSkillType::doChecksum( Checksum &checksum ) const {
	TargetBasedSkillType::doChecksum( checksum );

	checksum.add( m_attackStrength );
	checksum.add( m_attackVar );
	checksum.add( m_attackPctStolen );
	checksum.add( m_attackPctVar );
	m_attackType->doChecksum( checksum );
	
	// earthquakeType ??
}

void AttackSkillType::getDesc( string &str, const Unit *unit ) const {
	Lang &lang = Lang::getInstance( );

	// skill-speed and ep-cost first
	descSpeed( str, unit, "AttackSpeed" );
	descEpCost( str, unit );

	//attack strength
	str += lang.get( "AttackStrength" ) + ": ";
	str += intToStr( m_attackStrength - m_attackVar );
	str += "...";
	str += intToStr( m_attackStrength + m_attackVar );
	EnhancementType::describeModifier( str, unit->getAttackStrength( this ) - m_attackStrength );
	str += " ( " + lang.get( m_attackType->getName( ) ) + " )";
	str += "\n";

	TargetBasedSkillType::getDesc(str, unit, "AttackDistance" );
	descEffects( str, unit );
}

fixed AttackSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getAttackSpeedMult( ) + unit->getAttackSpeed( );
}

// ===============================
// 	class BuildSkillType
// ===============================

fixed BuildSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getRepairSpeedMult( ) + unit->getRepairSpeed( );
}

// ===============================
// 	class HarvestSkillType
// ===============================

fixed HarvestSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getHarvestSpeedMult( ) + unit->getHarvestSpeed( );	
}

// ===============================
// 	class DieSkillType
// ===============================

void DieSkillType::doChecksum( Checksum &checksum ) const {
	SkillType::doChecksum( checksum );
	checksum.add<bool>( m_fade );
}

void DieSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	SkillType::load( sn, dir, tt, ut );

	m_fade= sn->getChild( "fade" )->getAttribute( "value" )->getBoolValue( );
}

// ===============================
// 	class RepairSkillType
// ===============================

RepairSkillType::RepairSkillType( ) : SkillType( "Repair" ) {
	m_amount = 0;
	m_multiplier = 1;
	m_petOnly = false;
	m_selfAllowed = true;
	m_selfOnly = false;
}

void RepairSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	m_minRange = 1;
	m_maxRange = 1;
	SkillType::load( sn, dir, tt, ut );

	XmlNode *n;

	if (n = sn->getChild( "amount", 0, false )) {
		m_amount = n->getAttribute( "value" )->getIntValue( );
	}

	if (n = sn->getChild( "multiplier", 0, false )) {
		m_multiplier = n->getAttribute( "value" )->getFixedValue( );
	}

	if (n = sn->getChild( "pet-only", 0, false )) {
		m_petOnly = n->getAttribute( "value" )->getBoolValue( );
	}

	if (n = sn->getChild( "self-only", 0, false )) {
		m_selfOnly = n->getAttribute( "value" )->getBoolValue( );
		m_selfAllowed = true;
	}

	if (n = sn->getChild( "self-allowed", 0, false )) {
		m_selfAllowed = n->getAttribute( "value" )->getBoolValue( );
	}

	if (m_selfOnly && !m_selfAllowed) {
		throw runtime_error( "Repair skill can't specify self-only as true and self-allowed as false (dork)." );
	}

	if (m_petOnly && m_selfOnly) {
		throw runtime_error( "Repair skill can't specify pet-only with self-only." );
	}

	//splash particle
	const XmlNode *particleNode = sn->getChild( "particle", 0, false );
	if (particleNode && particleNode->getAttribute( "value" )->getBoolValue( ) ) {
		string path = particleNode->getAttribute( "path" )->getRestrictedValue( );
		m_splashParticleSystemType = new SplashType( );
		m_splashParticleSystemType->load( dir,  dir + "/" + path );
	}
}

void RepairSkillType::doChecksum( Checksum &checksum ) const {
	SkillType::doChecksum( checksum );
	checksum.add( m_amount );
	checksum.add( m_multiplier );
	checksum.add( m_petOnly );
	checksum.add( m_selfOnly );
	checksum.add( m_selfAllowed );
}

void RepairSkillType::getDesc( string &str, const Unit *unit ) const {
	Lang &lang= Lang::getInstance( );
	descSpeed( str, unit, "RepairSpeed" );
	descEpCost( str, unit );

	if (m_amount) {
		str += "\n" + lang.get( "HpRestored" ) + ": " +  intToStr( m_amount );
	}
	if (m_maxRange > 1 ) {
		str += "\n" + lang.get( "Range" ) + ": " + intToStr( m_maxRange );
	}
}

fixed RepairSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getRepairSpeedMult( ) + unit->getRepairSpeed( );
}

// =====================================================
// 	class ProduceSkillType
// =====================================================

ProduceSkillType::ProduceSkillType( ) : SkillType( "Produce" ) {
	m_pet = false;
	m_maxPets = 0;
}

void ProduceSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	SkillType::load( sn, dir, tt, ut );

	XmlNode *petNode = sn->getChild( "pet", 0, false );
	if (petNode) {
		m_pet = petNode->getAttribute( "value" )->getBoolValue( );
		m_maxPets = petNode->getAttribute( "max" )->getIntValue( );
	}
}

void ProduceSkillType::doChecksum( Checksum &checksum ) const {
	SkillType::doChecksum( checksum );
	checksum.add<bool>( m_pet );
	checksum.add<int>( m_maxPets );
}

fixed ProduceSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getProdSpeedMult( ) + unit->getProdSpeed( );
}

// =====================================================
// 	class UpgradeSkillType
// =====================================================

fixed UpgradeSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getProdSpeedMult( ) + unit->getProdSpeed( );
}

// =====================================================
// 	class MorphSkillType
// =====================================================

fixed MorphSkillType::getSpeed( const Unit *unit ) const {
	return m_speed * unit->getProdSpeedMult( ) + unit->getProdSpeed( );
}

// =====================================================
// 	class LoadSkillType
// =====================================================

LoadSkillType::LoadSkillType( ) : SkillType( "Load" ) {
}

void LoadSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ut ) {
	SkillType::load( sn, dir, tt, ut );
}

void LoadSkillType::doChecksum( Checksum &checksum ) const {
	SkillType::doChecksum( checksum );
}

// =====================================================
// 	class BeBuiltSkillType
// =====================================================

void BeBuiltSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ft ) {
	SkillType::load( sn, dir, tt, ft );
	const XmlNode *n = sn->getChild( "anim-progress-bound", 0, false );
	if (n) {
		m_stretchy = n->getBoolValue( );
	} else {
		m_stretchy = sn->getOptionalBoolValue( "anim-stretch", false );
	}
}

// =====================================================
// 	class BuildSelfSkillType
// =====================================================

void BuildSelfSkillType::load( const XmlNode *sn, const string &dir, const TechTree *tt, const UnitType *ft ) {
	SkillType::load( sn, dir, tt, ft );
	const XmlNode *n = sn->getChild( "anim-progress-bound", 0, false );
	if (n) {
		m_stretchy = n->getBoolValue( );
	} else {
		m_stretchy = sn->getOptionalBoolValue( "anim-stretch", false );
	}
}

// =====================================================
// 	class ModelFactory
// =====================================================

ModelFactory::ModelFactory( ) { }

Model* ModelFactory::newInstance( const string &path, int size, int height ) {
	assert( m_models.find( path ) == m_models.end( ) );
	Model *model = g_renderer.newModel( ResourceScope::GAME );
	model->load( path, size, height );
	while (mediaErrorLog.hasError( )) {
		MediaErrorLog::ErrorRecord record = mediaErrorLog.popError( );
		g_logger.logMediaError( "", record.path, record.msg.c_str( ) );
	}
	m_models[path] = model;
	return model;
}

Model* ModelFactory::getModel( const string &path, int size, int height ) {
	string cleanedPath = cleanPath( path );
	ModelMap::iterator it = m_models.find( cleanedPath );
	if (it != m_models.end( )) {
		return it->second;
	}
	return newInstance( cleanedPath, size, height );
}

// =====================================================
// 	class AttackSkillTypes & enum AttackSkillPreferenceFlags
// =====================================================

void AttackSkillTypes::init( ) {
	m_maxRange = 0;

	assert( m_types.size( ) == m_associatedPrefs.size( ) );
	for (int i = 0; i < m_types.size( ); ++i) {
		if (m_types[i]->getMaxRange( ) > m_maxRange) {
			m_maxRange = m_types[i]->getMaxRange( );
		}
		m_zones.flags |= m_types[i]->getZones( ).flags;
		m_allPrefs.flags |= m_associatedPrefs[i].flags;
	}
}

void AttackSkillTypes::getDesc( string &str, const Unit *unit ) const {
	if (m_types.size( ) == 1 ) {
		m_types[0]->getDesc( str, unit );
	} else {
		str += g_lang.get( "Attacks" ) + ":";
		for (int i = 0; i < m_types.size( ); ++i) {
			string nstr = g_lang.get( "AttackNum" );
			string::size_type p = nstr.find( "%s" );
			if (p != string::npos) {
				nstr.replace( p, 2, intToStr( i + 1 ) );
			}
			str += "\n" + nstr + "\n";
			m_types[i]->getDesc( str, unit );			
		}
	}
}

const AttackSkillType *AttackSkillTypes::getPreferredAttack(
		const Unit *unit, const Unit *target, int rangeToTarget ) const {
	const AttackSkillType *ast = nullptr;

	if (m_types.size( ) == 1) {
		ast = m_types[0];
		return unit->getMaxRange( ast ) >= rangeToTarget ? ast : nullptr;
	}

	// a skill for use when damaged gets 1st priority.
	if (hasPreference( AttackSkillPreference::WHEN_DAMAGED ) && unit->isDamaged( )) {
		return getSkillForPref( AttackSkillPreference::WHEN_DAMAGED, rangeToTarget );
	}

	//If a skill in this collection is specified as use whenever possible and
	//the target resides in a field that skill can attack, we will only use that
	//skill if in range and return nullptr otherwise.
	if (hasPreference( AttackSkillPreference::WHENEVER_POSSIBLE )) {
		ast = getSkillForPref( AttackSkillPreference::WHENEVER_POSSIBLE, 0 );
		assert( ast );
		if (ast->getZone( target->getCurrZone( ) )) {
			return unit->getMaxRange( ast ) >= rangeToTarget ? ast : nullptr;
		}
		ast = nullptr;
	}

	//if (hasPreference(AttackSkillPreference::ON_BUILDING) && unit->getType( )->isOfClass(UnitClass::BUILDING)) {
	//	ast = getSkillForPref(AttackSkillPreference::ON_BUILDING, rangeToTarget);
	//}

	//if (!ast && hasPreference(AttackSkillPreference::ON_LARGE) && unit->getType( )->getSize( ) > 1 ) {
	//	ast = getSkillForPref(AttackSkillPreference::ON_LARGE, rangeToTarget);
	//}

	//still haven't found an attack skill then use the 1st that's in range
	if (!ast) {
		for (int i = 0; i < m_types.size( ); ++i) {
			if (unit->getMaxRange( m_types[i] ) >= rangeToTarget && m_types[i]->getZone( target->getCurrZone( ) )) {
				ast = m_types[i];
				break;
			}
		}
	}

	return ast;
}

}} //end namespace
