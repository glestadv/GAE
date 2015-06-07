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

#ifndef _GLEST_GAME_PARTICLETYPE_H_
#define _GLEST_GAME_PARTICLETYPE_H_

#include <string>

#include "game_particle.h"

using std::string;
using Shared::Util::Random;
using Shared::Xml::XmlNode;
using Shared::Graphics::ParticleSystemBase;
using Shared::Graphics::ParticleSystem;

using Glest::Entities::UnitParticleSystem;
using Glest::Entities::Projectile;
using Glest::Entities::Splash;

namespace Glest { namespace ProtoTypes {

// ===========================================================
//	class ParticleSystemType
//
///	A type of particle system
// ===========================================================
/// base class for particle system proto-types
class ParticleSystemType : public ParticleSystemBase {
public:
	ParticleSystemType( );
	void load( const XmlNode *particleSystemNode, const string &dir );
	virtual ParticleSystem *create( bool vis ) = 0;
};

// ===========================================================
//	class DirectedParticleSystemType 
//
///	A particle system type that includes direction parameters.
// ===========================================================

class DirectedParticleSystemType : public ParticleSystemType {
private:
	Vec3f m_direction;
	bool  m_relative;
	bool  m_relativeDirection;
	bool  m_fixed;

public:
	DirectedParticleSystemType( ) : ParticleSystemType( ) { }

	void load( const XmlNode *particleSystemNode, const string &path );

	Vec3f getDirection( ) const       { return m_direction;          }
	bool isRelative( ) const          { return m_relative;           }
	bool isRelativeDirection( ) const { return m_relativeDirection;  }
	bool isFixed( ) const             { return m_fixed;              }
};

// ===========================================================
//	class FreeProjectileType
// ===========================================================

class FreeProjectileType : public DirectedParticleSystemType {
private:
	float m_mass;

public:
	void load( const XmlNode *particleSystemNode, const string &dir );

	virtual ParticleSystem *create( bool vis ) override;

	float getMass( ) const  { return m_mass; }
	void setMass( float v ) { m_mass = v;    }
};

// ===========================================================
//	class CompoundParticleSystemType
// ===========================================================

class CompoundParticleSystemType : public ParticleSystemType {
protected:
	struct ProjInfo {
		const FreeProjectileType *m_type;
		float m_startOffset;

		ProjInfo( const FreeProjectileType *t, float offset ) : m_type( t ), m_startOffset( offset ) { }
	};
	typedef vector<ProjInfo> Projectiles;

	Projectiles m_projectiles;

public:
	void load( const XmlNode *particleSystemNode, const string &dir );

	bool hasFreeProjectiles( ) const  { return !m_projectiles.empty( ); }

};

// ===========================================================
//	class ProjectileType
// ===========================================================

class ProjectileType : public ParticleSystemType {
private:
	TrajectoryType m_trajectory;
	float m_trajectorySpeed;
	float m_trajectoryScale;
	float m_trajectoryFrequency;
	ProjectileStart m_start;
	bool m_tracking;

public:
	void load( const string &dir, const string &path );
	virtual ParticleSystem *create( bool vis ) override;
	Projectile *createProjectileParticleSystem( bool vis ) { return (Projectile*)create( vis ); }

	ProjectileStart getStart( ) const { return m_start;    }
	bool isTracking( ) const          { return m_tracking; }
};

// ===========================================================
//	class SplashType
// ===========================================================

class SplashType : public ParticleSystemType {
private:
	float m_emissionRateFade;
	float m_verticalSpreadA;
	float m_verticalSpreadB;
	float m_horizontalSpreadA;
	float m_horizontalSpreadB;

public:
	void load( const string &dir, const string &path );
	virtual ParticleSystem *create( bool vis ) override;
	Splash *createSplashParticleSystem( bool vis ) { return (Splash*)create( vis ); }
};

// ===========================================================
//	class UnitParticleSystemType 
//
///	A particle system type that is attached to units.
// ===========================================================

class UnitParticleSystemType : public DirectedParticleSystemType {
protected:
	int m_maxParticles;

	EmitterPathType m_emitterType;
	Vec3f m_emitterAxis;
	float m_emitterDistance;
	float m_emitterSpeed;

public:
	UnitParticleSystemType( );
	void load( const XmlNode *particleSystemNode, const string &dir );
	void load( const string &dir, const string &path );

	bool hasTeamColorEnergy( ) const         { return teamColorEnergy;     }
	bool hasTeamColorNoEnergy( ) const       { return teamColorNoEnergy;   }
	EmitterPathType getEmitterType( ) const  { return m_emitterType;       }
	Vec3f getEmitterAxis( ) const            { return m_emitterAxis;       }
	float getEmitterDistance( ) const        { return m_emitterDistance;   }
	float getEmitterSpeed( ) const           { return m_emitterSpeed;      }

	UnitParticleSystem* createUnitParticleSystem( bool vis ) const {
		return new UnitParticleSystem( vis, *this, m_maxParticles );
	}
	virtual ParticleSystem* create( bool vis ) override { return createUnitParticleSystem( vis ); }
};
typedef vector<UnitParticleSystemType*> UnitParticleSystemTypes;


}}//end namespace

#endif
