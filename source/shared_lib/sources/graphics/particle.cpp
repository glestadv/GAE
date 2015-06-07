// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "particle.h"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <exception>

#include "util.h"
#include "particle_renderer.h"
#include "math_util.h"
#include "lang_features.h"

#include "leak_dumper.h"

using std::range_error;
using namespace Shared::Util;

namespace Shared { namespace Graphics {

// =====================================================
//	class Particle
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Particle)

// =====================================================
//	class ParticleSystemBase
// =====================================================

// =====================================================
//	class ParticleSystemBase
// =====================================================

ParticleSystemBase::ParticleSystemBase()
		: random(int(Chrono::getCurMillis()))
		, srcBlendFactor(BlendFactor::SRC_ALPHA)
		, destBlendFactor(BlendFactor::ONE)
		, blendEquationMode(BlendMode::FUNC_ADD)
		, primitiveType(PrimitiveType::QUAD)
		, offset(0.f)
		, color(1.f)
		, color2(1.f)
		, colorNoEnergy(0.f)
		, color2NoEnergy(0.f)
		, teamColorEnergy(false)
		, teamColorNoEnergy(false)
		, size(1.f)
		, sizeVar(0.f)
		, sizeNoEnergy(1.f)
		, sizeNoEnergyVar(0.f)
		, speed(1.f)
		, gravity(0.f)
		, emissionRate(15.f)
		, emissionRateRemainder(0.f)
		, overwriteOld(true)
		, energy(250)
		, energyVar(50)
		, radialVelocity(0.f)
		, radialVelocityVar(0.f)
		, radius(0.f)
		, drawCount(1)
		, numTextures(0)
		, model(0) {
	memset(textures, 0, MAX_PARTICLE_BUFFERS * sizeof(Texture*));
}

ParticleSystemBase::ParticleSystemBase(const ParticleSystemBase &protoType)
		: random(int(Chrono::getCurMillis()))
		, srcBlendFactor(protoType.srcBlendFactor)
		, destBlendFactor(protoType.destBlendFactor)
		, blendEquationMode(protoType.blendEquationMode)
		, primitiveType(protoType.primitiveType)
		, offset(protoType.offset)
		, color(protoType.color)
		, color2(protoType.color2)
		, colorNoEnergy(protoType.colorNoEnergy)
		, color2NoEnergy(protoType.color2NoEnergy)
		, teamColorEnergy(protoType.teamColorEnergy)
		, teamColorNoEnergy(protoType.teamColorNoEnergy)
		, size(protoType.size)
		, sizeVar(protoType.sizeVar)
		, sizeNoEnergy(protoType.sizeNoEnergy)
		, sizeNoEnergyVar(protoType.sizeNoEnergyVar)
		, speed(protoType.speed)
		, gravity(protoType.gravity)
		, emissionRate(protoType.emissionRate)
		, emissionRateRemainder(0.f)
		, overwriteOld(protoType.overwriteOld)
		, energy(protoType.energy)
		, energyVar(protoType.energyVar)
		, radialVelocity(protoType.radialVelocity)
		, radialVelocityVar(protoType.radialVelocityVar)
		, radius(protoType.radius)
		, drawCount(protoType.drawCount)
		, numTextures(protoType.numTextures)
		, model(protoType.model) {
	memcpy(textures, protoType.textures, MAX_PARTICLE_BUFFERS * sizeof(Texture*));
}

void ParticleSystemBase::setTexture(int i, Texture *v) {
	ASSERT_RANGE(i, 4);
	if (i < 0 && i >= 4) {
		throw runtime_error("Too many textures. Particle systems may have at most 4 textures.");
	}
	if (textures[i] == 0) {
		++ numTextures;
	}
	textures[i] = v;
}

// =====================================================
//	class ParticleSystem
// =====================================================

int ParticleSystem::particleCounts[ParticleUse::COUNT] = {
	0, 0, 0, 0, 0
};

void ParticleSystem::addParticleUse(ParticleUse use, int n) {
	particleCounts[use] += n;
}

void ParticleSystem::remParticleUse(ParticleUse use, int n) {
	particleCounts[use] -= n;
}


//int ParticleSystem::idCounter = 0;

ParticleSystem::ParticleSystem(int particleCount)
		: ParticleSystemBase()
		//, id(++idCounter)
		, particleCount(particleCount)
		, particles(0)
		, state(sPlay)
		, active(true)
		, visible(true)
		, aliveParticleCount(0)
		, pos(0.f)
		, windSpeed(0.f)
		, emissionRateRemainder(0.f) {
}

ParticleSystem::ParticleSystem(const ParticleSystemBase &model, int particleCount)
		: ParticleSystemBase(model)
		//, id(++idCounter)
		, particleCount(particleCount)
		, particles(0)
		, state(sPlay)
		, active(true)
		, visible(true)
		, aliveParticleCount(0)
		, pos(0.f)
		, windSpeed(0.f)
		, emissionRateRemainder(0.f) {
}

void ParticleSystem::initArray(ParticleUse use) {
	assert(!particles);
	particles = new Particle[particleCount];
	addParticleUse(use, particleCount);
	this->use = use;
}

void ParticleSystem::freeArray() {
	delete [] particles;
	remParticleUse(use, particleCount);
	particles = 0;
	aliveParticleCount = 0;
}

ParticleSystem::~ParticleSystem() {
	delete [] particles;
	remParticleUse(use, particleCount);
}

// =============== VIRTUAL ======================

// updates all living particles and creates new ones
void ParticleSystem::update() {
	if (visible && state != sPause) {
		for (int i = 0; i < aliveParticleCount; ++i) {
			updateParticle(&particles[i]);
			if (deathTest(&particles[i])) {
				// kill the particle
				killParticle(&particles[i]);

				// mantain alive particles at front of the array
				if (aliveParticleCount > 0) {
					///@todo FIXME: this particle skips an update :(
					particles[i] = particles[aliveParticleCount];
					// --i; ?
				}
			}
		}
		if (state != sFade) {
			float fCount = emissionRateRemainder + emissionRate;
			int count = int(fCount);
			emissionRateRemainder = fCount - count;
			for (int i = 0; i < count; ++i) {
				Particle *p = createParticle();
				if (p) {
					initParticle(p, i);
				} else {
					break;
				}
			}
		}
	}
}

void ParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	if (active) {
		pr->renderSystem(this);
	}
}

void ParticleSystem::setTeamColour(Vec3f teamColour) {
	if (teamColorEnergy) {
		color = Vec4f(teamColour, color.a);
	}
	if (teamColorNoEnergy) {
		colorNoEnergy = Vec4f(teamColour, colorNoEnergy.a);
	}
}

// =============== MISC =========================

void ParticleSystem::fade() {
	assert(state == sPlay);
	state = sFade;
}

// =============== PROTECTED =========================

// if there is one dead particle it returns it else, return the particle with
// the least energy
Particle *ParticleSystem::createParticle() {
	//if any dead particles
	if (aliveParticleCount < particleCount) {
		++aliveParticleCount;
		return &particles[aliveParticleCount-1];
	}

	if (overwriteOld) {
		int minEnergy = particles[0].energy.current;
		int minEnergyParticle = 0;
		for (int i = 0; i < particleCount; ++i) {
			if (particles[i].energy.current < minEnergy) {
				minEnergy = particles[i].energy.current;
				minEnergyParticle = i;
			}
		}
		return &particles[minEnergyParticle];
	}
	return 0;
}

void ParticleSystem::initParticle(Particle *p, int particleIndex) {
	p->pos = pos;
	p->lastPos = pos;
	p->speed = Vec3f(0.0f);
	p->accel = Vec3f(0.0f);
	p->energy.start = p->energy.current = getRandEnergy();
	p->colour.value = getColor();
	p->colour2.value = getColor2();
	p->colour.step = (getColorNoEnergy() - getColor()) / float(p->energy.current);
	p->colour2.step = (getColor2NoEnergy() - getColor2()) / float(p->energy.current);
	p->size.value = getRandomStartSize();

	float totalStep = getRandomEndSize() - p->size.value;
	p->size.step = totalStep / float(p->energy.current);
	
	if (hasAngularVelocity()) {
		p->angle.value = getRandAngle();
		p->angle.step = getRandAngularVelocity() * (1.f / 40.f);
	} else {
		p->angle.value = p->angle.step = 0.f;
	}
	p->texture = getNumTextures() > 1 ? random.randRange(0, getNumTextures() - 1) : 0;
}

void ParticleSystem::updateParticle(Particle *p) {
	p->lastPos = p->pos;
	p->pos = p->pos + p->speed;
	p->speed = p->speed + p->accel;
	p->size.value += p->size.step;
	if (hasAngularVelocity()) {
		p->angle.value = fmodf(p->angle.value + p->angle.step, Math::twopi);
	}
	p->energy.current--;
}

// ===========================================================================
//  RainParticleSystem
// ===========================================================================


RainParticleSystem::RainParticleSystem(int particleCount)
		: ParticleSystem(particleCount) {
	setRadius(20.f);
	setEmissionRate(25.f);
	setSize(3.0f);
	setColor(Vec4f(0.5f, 0.5f, 0.5f, 0.3f));
	setColor2(Vec4f(0.5f, 0.5f, 0.5f, 0.0f));
	setSpeed(0.2f);

	initArray(ParticleUse::WEATHER);
}

void RainParticleSystem::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float x = random.randRange(-radius, radius);
	float y = random.randRange(-radius, radius);

	p->energy.current = p->energy.start = 10000;
	p->colour.value = getColor();
	p->colour2.value = getColor2();
	p->colour.step = (getColorNoEnergy() - getColor()) / float(p->energy.current);
	p->colour2.step = (getColor2NoEnergy() - getColor2()) / float(p->energy.current);
	p->pos = Vec3f(pos.x + x, pos.y, pos.z + y);
	p->lastPos = p->pos;
	p->speed = Vec3f(random.randRange(-speed / 10, speed / 10), -speed,
			random.randRange(-speed / 10, speed / 10)) + windSpeed;
}

void RainParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	pr->renderSystemLine(this);
}

bool RainParticleSystem::deathTest(Particle *p) {
	return p->pos.y < 0;
}

// ===========================================================================
//  SnowParticleSystem
// ===========================================================================

SnowParticleSystem::SnowParticleSystem(int particleCount) : ParticleSystem(particleCount) {
	setRadius(30.0f);
	setEmissionRate(2.f);
	setSize(0.2f);
	setColor(Vec4f(0.8f, 0.8f, 0.8f, 0.8f));
	setSpeed(0.025f);
	initArray(ParticleUse::WEATHER);
}

void SnowParticleSystem::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float x = random.randRange(-radius, radius);
	float y = random.randRange(-radius, radius);

	//p->color = color;
	p->energy.current = p->energy.start = 10000;
	p->pos = Vec3f(pos.x + x, pos.y, pos.z + y);
	p->speed = Vec3f(0.0f, -speed, 0.0f) + windSpeed;
	p->speed.x += random.randRange(-0.005f, 0.005f);
	p->speed.y += random.randRange(-0.005f, 0.005f);
}

bool SnowParticleSystem::deathTest(Particle *p) {
	return p->pos.y < 0;
}

// ===========================================================================
//  ParticleManager
// ===========================================================================

ParticleManager::~ParticleManager(){
	end();
}

void ParticleManager::render(ParticleRenderer *pr, ModelRenderer *mr) const{
	list<ParticleSystem*>::const_iterator it;

	for (it=particleSystems.begin(); it!=particleSystems.end(); ++it){
		if((*it)->getVisible()){
			(*it)->render(pr, mr);
		}
	}
}

void ParticleManager::update(){
	list<ParticleSystem*>::iterator it;

	for (it = particleSystems.begin(); it != particleSystems.end(); ++it) {
		(*it)->update();
		if ((*it)->isFinished()) {
			delete *it;
			*it = NULL;
		}
	}

	particleSystems.remove(NULL);
}

void ParticleManager::manage(ParticleSystem *ps) {
	particleSystems.push_back(ps);
}


}}//end namespace