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

#ifndef _GLEST_PROTOTYPES_EFFECT_DEFINED_
#define _GLEST_PROTOTYPES_EFFECT_DEFINED_

#include "sound.h"
#include "vec.h"
#include "element_type.h"
#include "xml_parser.h"
#include "particle_type.h"
#include "upgrade_type.h"
#include "flags.h"
#include "game_constants.h"

using Shared::Sound::StaticSound;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;

namespace Glest { namespace ProtoTypes {

class TechTree;
class FactionType;

// ==============================================================
// 	enum EffectTypeFlag & class EffectTypeFlags
// ==============================================================

/**
 * Flags for an EffectType object.
 *
 * @see EffectTypeFlag
 */
class EffectTypeFlags : public XmlBasedFlags<EffectTypeFlag, EffectTypeFlag::COUNT>{
public:
	void load( const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft ) {
		XmlBasedFlags<EffectTypeFlag, EffectTypeFlag::COUNT>::load( node, dir, tt, ft, "flag", EffectTypeFlagNames );
	}
};

class EffectType;
typedef vector<EffectType*> EffectTypes;

/**
 * Represents the type for a possibly temporary effect which modifies the
 * stats, regeneration/degeneration and possibly other behaviors &
 * attributes of a Unit.
 */
class EffectType: public EnhancementType, public DisplayableType {
private:
	EffectBias m_bias;
	EffectStacking m_stacking;

	int m_duration;
	fixed m_chance;
	bool m_light;
	Vec3f m_lightColour;
	StaticSound *m_sound;
	float m_soundStartTime;
	bool m_loopSound;
	EffectTypes m_recourse;
	EffectTypeFlags m_flags;
	string m_affectTag;
	const AttackType *m_damageType;
	bool m_display;
	UnitParticleSystemTypes m_particleSystems;
	const FactionType *m_factionType;

public:
	static EffectClass typeClass( ) { return EffectClass::EFFECT; }

public:
	EffectType( );
	virtual ~EffectType( ) { delete m_sound; }
	virtual bool load( const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft );
	virtual void doChecksum( Checksum &checksum ) const;

	const FactionType* getFactionType( ) const  { return m_factionType;       }
	EffectBias getBias( ) const                 { return m_bias;              }
	EffectStacking getStacking( ) const         { return m_stacking;          }
	const EffectTypeFlags &getFlags( ) const    { return m_flags;             }
	bool getFlag( EffectTypeFlag flag ) const   { return m_flags.get( flag ); }
	const AttackType *getDamageType( ) const    { return m_damageType;        }
	const string& getAffectTag( ) const         { return m_affectTag;         }

	bool isEffectsAlly( ) const             { return getFlag( EffectTypeFlag::ALLY );                    }
	bool isEffectsFoe( ) const              { return getFlag( EffectTypeFlag::FOE );                     }
	//bool isEffectsNormalUnits( ) const    { return !getFlag( EffectTypeFlag::NO_NORMAL_UNITS );        }
	//bool isEffectsBuildings( ) const      { return getFlag( EffectTypeFlag::BUILDINGS );               }
	bool isEffectsPetsOnly( ) const         { return getFlag( EffectTypeFlag::PETS_ONLY );               }
	//bool isEffectsNonLiving( ) const      { return getFlag( EffectTypeFlag::NON_LIVING );              }
	bool isScaleSplashStrength( ) const     { return getFlag( EffectTypeFlag::SCALE_SPLASH_STRENGTH );   }
	bool isEndsWhenSourceDies( ) const      { return getFlag( EffectTypeFlag::ENDS_WITH_SOURCE );        }
	bool idRecourseEndsWithRoot( ) const    { return getFlag( EffectTypeFlag::RECOURSE_ENDS_WITH_ROOT ); }
	bool isPermanent( ) const               { return getFlag( EffectTypeFlag::PERMANENT );               }
	bool isTickImmediately( ) const         { return getFlag( EffectTypeFlag::TICK_IMMEDIATELY );        }
	bool isCauseCloak( ) const              { return getFlag( EffectTypeFlag::CAUSES_CLOAK );            }

	int getDuration( ) const                                 { return m_duration;         }
	fixed getChance( ) const                                 { return m_chance;           }
	bool isLight( ) const                                    { return m_light;            }
	bool isDisplay( ) const                                  { return m_display;          }
	Vec3f getLightColour( ) const                            { return m_lightColour;      }
	UnitParticleSystemTypes& getParticleTypes( )             { return m_particleSystems;  }
	const UnitParticleSystemTypes& getParticleTypes( ) const { return m_particleSystems;  }
	StaticSound *getSound( ) const                           { return m_sound;            }
	float getSoundStartTime( ) const                         { return m_soundStartTime;   }
	bool isLoopSound( ) const                                { return m_loopSound;        }
	const EffectTypes &getRecourse( ) const                  { return m_recourse;         }
	void getDesc( string &str ) const;
};

class EmanationType : public EffectType {
private:
	int m_radius;
	UnitParticleSystemTypes m_sourceParticleSystems;
public:
	static EffectClass typeClass( ) { return EffectClass::EMANATION; }

public:
	virtual ~EmanationType( ) { }

	UnitParticleSystemTypes& getSourceParticleTypes( )              { return m_sourceParticleSystems; }
	const UnitParticleSystemTypes& getSourceParticleTypes( ) const  { return m_sourceParticleSystems; }

	virtual bool load( const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft );
	virtual void doChecksum( Checksum &checksum ) const;
	int getRadius( ) const	{ return m_radius; }
};

typedef vector<EmanationType*> Emanations;

}}//end namespace


#endif
