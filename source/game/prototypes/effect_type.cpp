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
#include "effect_type.h"
#include "renderer.h"
#include "tech_tree.h"
#include "logger.h"

#include "leak_dumper.h"

using Glest::Util::Logger;

namespace Glest { namespace ProtoTypes {
using namespace Graphics;
// =====================================================
// 	class EffectType
// =====================================================

EffectType::EffectType( ) : m_lightColour( 0.0f ) {
	m_bias = EffectBias::NEUTRAL;
	m_stacking = EffectStacking::STACK;

	m_duration = 0;
	m_chance = 100;
	m_light = false;
	m_sound = nullptr;
	m_soundStartTime = 0.0f;
	m_loopSound = false;
	m_damageType = nullptr;
	m_factionType = nullptr;
	m_display = true;
}

bool EffectType::load( const XmlNode *en, const string &dir, const TechTree *tt, const FactionType *ft ) {
	m_factionType = ft;
	string tmp;
	const XmlAttribute *attr;
	bool loadOk = true;
	
	try { // name
		m_name = en->getAttribute( "name" )->getRestrictedValue( ); 
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	// bigtime hack (REFACTOR: Move to EffectTypeFactory)
	m_id = const_cast<TechTree*>( tt )->addEffectType( this );

	try { // bias
		tmp = en->getAttribute( "bias" )->getRestrictedValue( );
		m_bias = EffectBiasNames.match( tmp.c_str( ) );
		if (m_bias == EffectBias::INVALID) {
			if (tmp == "benificial") { // support old typo/spelling error
				m_bias = EffectBias::BENEFICIAL;
			} else {
				throw runtime_error( "Not a valid value for bias: " + tmp + ": " + dir );
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // stacking
		tmp = en->getAttribute( "stacking" )->getRestrictedValue( );
		m_stacking = EffectStackingNames.match( tmp.c_str( ) );
		if (m_stacking == EffectStacking::INVALID) {
			throw runtime_error( "Not a valid value for stacking: " + tmp + ": " + dir );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // target (default all)
		attr = en->getAttribute( "target", false );
		if (attr) {
			tmp = attr->getRestrictedValue( );
			if (tmp == "ally") {
				m_flags.set( EffectTypeFlag::ALLY, true );
			} else if (tmp == "foe") {
				m_flags.set( EffectTypeFlag::FOE, true );
			} else if (tmp == "pet") {
				m_flags.set( EffectTypeFlag::ALLY, true );
				m_flags.set( EffectTypeFlag::PETS_ONLY, true );
			} else if (tmp == "all") {
				m_flags.set( EffectTypeFlag::ALLY, true );
				m_flags.set( EffectTypeFlag::FOE, true );
			} else {
				throw runtime_error( "Not a valid value for effect target: '" + tmp + "' : " + dir );
			}
		} else { // default value
			m_flags.set( EffectTypeFlag::ALLY, true );
			m_flags.set( EffectTypeFlag::FOE, true );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // chance (default 100%)
		attr = en->getAttribute( "chance", false );
		if (attr) {
			m_chance = attr->getFixedValue( );
		} else {
			m_chance = 100;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // duration
		m_duration = en->getAttribute( "duration" )->getIntValue( );
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // damageType (default nullptr)
		attr = en->getAttribute( "damage-type", false );
		if (attr) {
			m_damageType = tt->getAttackType( attr->getRestrictedValue( ) );
		}
	}
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // display (default true)
		attr = en->getAttribute( "display", false );
		if (attr) {
			m_display = attr->getBoolValue( );
		} else {
			m_display = true;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // image (default nullptr)
		attr = en->getAttribute( "image", false );
		if (attr && !(attr->getRestrictedValue( ) == "")) {
			m_image = g_renderer.getTexture2D( ResourceScope::GAME, dir + "/" + attr->getRestrictedValue( ) );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	const XmlNode *affectNode = en->getChild( "affect", 0, false );
	if (affectNode) {
		m_affectTag = affectNode->getRestrictedValue( );
	} else {
		m_affectTag = "";
	}

	// flags
	const XmlNode *flagsNode = en->getChild( "flags", 0, false );
	if (flagsNode) {
		m_flags.load( flagsNode, dir, tt, ft );
	}

	EnhancementType::load( en, dir, tt, ft );

	try { // light & lightColour
		const XmlNode *lightNode = en->getChild( "light", 0, false );
		if (lightNode) {
			m_light = lightNode->getAttribute( "enabled" )->getBoolValue( );

			if (m_light) {
				m_lightColour.x = lightNode->getAttribute( "red"   )->getFloatValue( 0.f, 1.f );
				m_lightColour.y = lightNode->getAttribute( "green" )->getFloatValue( 0.f, 1.f );
				m_lightColour.z = lightNode->getAttribute( "blue"  )->getFloatValue( 0.f, 1.f );
			}
		} else {
			m_light = false;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // particle systems
		const XmlNode *particlesNode = en->getOptionalChild( "particle-systems" );
		if (particlesNode) {
			for (int i=0; i < particlesNode->getChildCount( ); ++i) {
				const XmlNode *particleFileNode = particlesNode->getChild( "particle-file", i );
				string path = particleFileNode->getAttribute( "path" )->getRestrictedValue( );
				UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType( );
				unitParticleSystemType->load( dir,  dir + "/" + path );
				m_particleSystems.push_back( unitParticleSystemType );
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // sound
		const XmlNode *soundNode = en->getChild( "sound", 0, false );
		if (soundNode && soundNode->getAttribute( "enabled" )->getBoolValue( )) {
			m_soundStartTime = soundNode->getAttribute( "start-time" )->getFloatValue( );
			m_loopSound = soundNode->getAttribute( "loop" )->getBoolValue( );
			string path = soundNode->getAttribute( "path" )->getRestrictedValue( );
			m_sound = new StaticSound( );
			m_sound->load( dir + "/" + path );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { // recourse
		const XmlNode *recourseEffectsNode = en->getChild( "recourse-effects", 0, false );
		if (recourseEffectsNode) {
			m_recourse.resize( recourseEffectsNode->getChildCount( ) );
			for(int i = 0; i < recourseEffectsNode->getChildCount( ); ++i) {
				const XmlNode *recourseEffectNode = recourseEffectsNode->getChild( "effect", i );
				EffectType *effectType = new EffectType( );
				effectType->load( recourseEffectNode, dir, tt, ft );
				m_recourse[i] = effectType;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	if (m_hpRegeneration >= 0 && m_damageType) {
		m_damageType = nullptr;
	}
	return loadOk;
}

void EffectType::doChecksum( Checksum &checksum ) const {
	NameIdPair::doChecksum( checksum );
	EnhancementType::doChecksum( checksum );

	checksum.add( m_bias );
	checksum.add( m_stacking );

	checksum.add( m_duration );
	checksum.add( m_chance );
	checksum.add( m_light );
	checksum.add( m_lightColour );

	checksum.add( m_soundStartTime );
	checksum.add( m_loopSound );
	
	foreach_const (EffectTypes, it, m_recourse) {
		(*it)->doChecksum( checksum );
	}
	foreach_enum (EffectTypeFlag, f) {
		checksum.add( m_flags.get( f ) );
	}
	if (m_damageType) {
		checksum.add( m_damageType->getName( ) );
	}
	checksum.add( m_display );
}

void EffectType::getDesc( string &str ) const {
	if (!m_display) {
		return;
	}

	str += g_lang.getFactionString( getFactionType( )->getName( ), getName( ) );

	// effected units
	if (isEffectsPetsOnly( ) || isEffectsFoe( ) || isEffectsAlly( )) {
		str += "\n   " + g_lang.get( "Affects" ) + ": ";
		if (isEffectsPetsOnly( )) {
			str += g_lang.get( "PetsOnly" );
		} else if (isEffectsAlly( )) {
			str += g_lang.get( "AllyOnly" );
		} else {
			assert(isEffectsFoe( ));
			str += g_lang.get( "FoeOnly" );
		}
	}

	if (m_chance != 100) {
		str += "\n   " + g_lang.get( "Chance" ) + ": " + intToStr( m_chance.intp( ) ) + "%";
	}

	str += "\n   " + g_lang.get( "Duration" ) + ": ";
	if (isPermanent( )) {
		str += g_lang.get( "Permenant" );
	} else {
		str += intToStr( m_duration );
	}

	if (m_damageType) {
		str += "\n   " + g_lang.get( "DamageType" ) + ": " + g_lang.getTechString( m_damageType->getName( ) );
	}

	EnhancementType::getDesc( str, "\n   " );
	str += "\n";
}

// =====================================================
//  class EmanationType
// =====================================================

bool EmanationType::load( const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft ) {
	EffectType::load( n, dir, tt, ft );

	// radius
	try { m_radius = n->getAttribute( "radius" )->getIntValue( ); }
	catch (runtime_error e) {
		g_logger.logError( dir, e.what( ) );
		return false;
	}

	try { // source-particle systems
		const XmlNode *particlesNode = n->getOptionalChild( "source-particle-systems" );
		if (particlesNode) {
			for (int i=0; i < particlesNode->getChildCount( ); ++i) {
				const XmlNode *particleFileNode = particlesNode->getChild( "particle-file", i );
				string path = particleFileNode->getAttribute( "path" )->getRestrictedValue( );
				UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType( );
				unitParticleSystemType->load( dir,  dir + "/" + path );
				m_sourceParticleSystems.push_back( unitParticleSystemType );
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		return false;
	}

	return true;
}

void EmanationType::doChecksum( Checksum &checksum ) const {
	EffectType::doChecksum( checksum );
	checksum.add( m_radius );
}

}}//end namespace
