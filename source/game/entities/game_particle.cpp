// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//                2008-2009 Daniel Santos
//                2009-2010 Titus Tscharntke
//                2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "game_particle.h"
#include "particle_renderer.h"
#include "logger.h"
#include "world.h"
#include "leak_dumper.h"
#include "program.h"
#include "sim_interface.h"
#include "math_util.h"
#include "core_data.h"

namespace Glest { namespace Entities {
using Sim::Tile;
using Graphics::Renderer;
using Graphics::SceneCuller;
using Main::Program;
using namespace Shared;
// ===========================================================================
//  GameParticleSystem
// ===========================================================================

MEMORY_CHECK_IMPLEMENTATION(GameParticleSystem)

void GameParticleSystem::doVisibiltyChecks(ParticleUse use) {
	Vec2i cellPos(int(pos.x), int(pos.z));
	if (g_map.getTile(Map::toTileCoords(cellPos))->isVisible(g_world.getThisTeamIndex())
	&& g_renderer.getCuller().isInside(cellPos)) { // visible
		if (!visible) {
			initArray(use);
			visible = true;
		}
	} else { // not visible
		if (visible) {
			freeArray();
			visible = false;
		}
	}
}

void GameParticleSystem::checkVisibilty(ParticleUse use, bool log) {
	int64 now = Chrono::getCurMillis();
	if (state != sPause && now - lastVisCheck > test_interval) {
		lastVisCheck = now;
		doVisibiltyChecks(use);
	}
}

// ===========================================================================
//  FireParticleSystem
// ===========================================================================

FireParticleSystem::FireParticleSystem(const Unit *unit, bool visible, int particleCount)
		: GameParticleSystem(ParticleUse::FIRE, visible, particleCount) {
	setColorNoEnergy(Vec4f(1.0f, 0.5f, 0.0f, 1.0f));
	setEnergy(50);
	setEnergyVar(10);
	setSpeed(2.5f / float(GameConstants::updateFps));
	setPos(unit->getCurrVector());
	setRadius(unit->getType()->getSize() / 3.f);
	setTexture(0, g_coreData.getFireTexture1());
	setTexture(1, g_coreData.getFireTexture2());
	setSize(unit->getType()->getSize() / 3.f);
	setSizeVar(0.3f);
	setSizeNoEnergyVar(unit->getType()->getSize() / 5.f);
	setSizeNoEnergyVar(0.1f);
	float emit = unit->getType()->getSize() >= 4 ? unit->getType()->getSize() / 5.f * 35.f : unit->getType()->getSize() / 5.f * 30.f;
	emit = clamp(emit, 5.f, 35.f);
	setEmissionRate(emit);
}

void FireParticleSystem::update() {
	ParticleSystem::update();
	checkVisibilty(ParticleUse::FIRE);
}

void FireParticleSystem::initParticle(Particle *p, int particleIndex) {
	float ang = random.randRange(-twopi, twopi);
	float mod = fabsf(random.randRange(-radius, radius));

	float x = sinf(ang) * mod;
	float y = cosf(ang) * mod;

	float radRatio = sqrtf(sqrtf(mod / radius));
	Vec4f halfColorNoEnergy = colorNoEnergy * 0.5f;

	p->energy.current = p->energy.start = int(energy * radRatio) + random.randRange(-energyVar, energyVar);
	p->colour.value = halfColorNoEnergy + halfColorNoEnergy * radRatio;
	p->colour.step = (-p->colour.value) / float(p->energy.current);
	float halfRadius = radius * 0.5f;
	p->pos = Vec3f(pos.x + x, pos.y + random.randRange(-halfRadius, halfRadius), pos.z + y);
	p->lastPos = pos;
	p->size.value = getRandomStartSize();
	float totalStep = getRandomEndSize() - p->size.value;
	p->size.step = totalStep / float(p->energy.current);
	p->speed = Vec3f(speed * random.randRange(-0.125f, 0.125f), speed * random.randRange(0.5f, 1.5f),
			speed * random.randRange(-0.125f, 0.125f));
	p->angle.step = (random.rand() % 2 ? -1.f : +1.f) * random.randRange(Math::pi, Math::twopi) * (1.f / 40.f);
	p->texture = getNumTextures() > 1 ? random.randRange(0, getNumTextures() - 1) : 0;
}

void FireParticleSystem::updateParticle(Particle *p) {
	p->lastPos = p->pos;
	p->pos = p->pos+p->speed;
	p->energy.current--;
	p->colour.value += p->colour.step;
	p->speed.x *= 1.001f; // wind
	p->angle.value = fmodf(p->angle.value + p->angle.step, Math::twopi);
}


// ===========================================================================
//  SmokeParticleSystem
// ===========================================================================

SmokeParticleSystem::SmokeParticleSystem(const Unit *unit, bool visible, int particleCount)
		: GameParticleSystem(ParticleUse::FIRE, visible, particleCount) {
	setEnergy(240);
	setEnergyVar(10);
	setSpeed(1.f / float(GameConstants::updateFps));
	setPos(unit->getCurrVector() + Vec3f(0.f, 3.f, 0.f));
	setRadius(unit->getType()->getSize() / 3.f);
	setTexture(0, g_coreData.getSmokeTexture());
	setSize(unit->getType()->getSize() / 2.f);
	setSizeVar(0.33f);
	setSizeNoEnergyVar(unit->getType()->getSize() / 1.5f);
	setSizeNoEnergyVar(0.2f);
	//float emit = float(unit->getType()->getSize());
	setEmissionRate(0.2f);
	setDestBlendFactor(BlendFactor::ONE_MINUS_SRC_ALPHA);
}

void SmokeParticleSystem::update() {
	ParticleSystem::update();
	checkVisibilty(ParticleUse::FIRE);
}

void SmokeParticleSystem::initParticle(Particle *p, int particleIndex) {
	float ang = random.randRange(-twopi, twopi);
	float mod = fabsf(random.randRange(-radius, radius));

	float x = sinf(ang) * mod;
	float y = cosf(ang) * mod;

	float radRatio = sqrtf(sqrtf(mod / radius));

	p->energy.current = p->energy.start = int(energy * radRatio) + random.randRange(-energyVar, energyVar);
	p->colour.value = Vec4f(0.6f, 0.6f, 0.6f, 0.175f);
	p->colour.step = (Vec4f(0.6f, 0.6f, 0.6f, 0.f) - p->colour.value) / float(p->energy.current);
	float halfRadius = radius * 0.5f;
	p->pos = Vec3f(pos.x + x, pos.y + random.randRange(-halfRadius, halfRadius), pos.z + y);
	p->lastPos = pos;
	p->size.value = getRandomStartSize();
	float totalStep = getRandomEndSize() - p->size.value;
	p->size.step = totalStep / float(p->energy.current);
	p->speed = Vec3f(speed * random.randRange(-0.125f, 0.125f), speed * random.randRange(0.5f, 1.5f),
			speed * random.randRange(-0.125f, 0.125f));
	p->angle.step = (random.rand() % 2 ? -1.f : +1.f) * random.randRange(0.01f, Math::halfpi) * (1.f / 40.f);
	p->texture = getNumTextures() > 1 ? random.randRange(0, getNumTextures() - 1) : 0;
}

void SmokeParticleSystem::updateParticle(Particle *p) {
	p->lastPos = p->pos;
	p->pos = p->pos + p->speed;
	p->energy.current--;
	p->colour.value += p->colour.step;
	p->speed.x *= 1.01f; // wind
	p->angle.value = fmodf(p->angle.value + p->angle.step, Math::twopi);
}

// ===========================================================================
//  DirectedParticleSystem
// ===========================================================================

DirectedParticleSystem::DirectedParticleSystem(ParticleUse use, bool visible, const ParticleSystemBase &protoType, int particleCount)
		: GameParticleSystem(use, visible, protoType, particleCount) 
		, direction(1.0f, 0.0f, 0.0f) {
	fog = g_world.getTileset()->getFog();
}

void DirectedParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	if (active) {
		if (model) {
			pr->renderSingleModel(this, mr, fog);
		}
		switch (primitiveType) {
			case PrimitiveType::QUAD:
				pr->renderSystem(this);
				break;
			case PrimitiveType::LINE:
				pr->renderSystemLine(this);
				break;
			default:
				assert(false);
		}
	}
}

// ===========================================================================
//  FreeProjectile
// ===========================================================================

void FreeProjectile::update() {
	if (state == sPlay) {
		m_lastPos = pos;
		this->pos += m_velocity;
		m_velocity += Vec3f(0.f, -9.8f, 0.f) / 40.f;
	}
}

// ===========================================================================
//  Projectile
// ===========================================================================

Projectile::Projectile(CreateParams params)
		: DirectedParticleSystem(ParticleUse::PROJECTILE, params.visible, params.model, params.particleCount)
		, m_id(-1)
		, nextParticleSystem(0)
		, target(0)
		, trajectory(TrajectoryType::LINEAR)
		, trajectorySpeed(1.f)
		, trajectoryScale(1.f)
		, trajectoryFrequency(1.f)
		, random(Chrono::getCurMicros())
		, callback(0) {
}

Projectile::~Projectile() {
	//if (nextParticleSystem != NULL) {
		//nextParticleSystem->prevParticleSystem = NULL;
	//}
	delete callback;
}

void Projectile::setCallback(ProjectileCallback *cb) {
	assert(!callback);
	callback = cb;
}

void Projectile::link(Splash *particleSystem) {
	nextParticleSystem = particleSystem;
	nextParticleSystem->setState(sPause);
	//nextParticleSystem->prevParticleSystem = this;
}

void Projectile::update() {
	if (state == sPlay) {
		if (target) {
			if (target->isCarried()) { // if target got into another unit, switch target to carrier
				target = g_world.getUnit(target->getCarrier());
				RUNTIME_CHECK(!target->isCarried());
			}
			endPos = target->getCurrVector();
		}
		lastPos = pos;
		assert(startFrame >= 0 && endFrame >= 0 && startFrame < endFrame);
		float t = clamp((g_world.getFrameCount() - startFrame) / float(endFrame - startFrame), 0.f, 1.f);

		if (trajectory == TrajectoryType::RANDOM) {
			Vec3f currentTargetVector = endPos - pos;
			currentTargetVector.normalize();

			float varRotation = random.randRange(0.f, twopi);
			float varAngle = random.randRange(-pi, pi) * trajectoryScale;
			float varPitch = cosf(varRotation) * varAngle;
			float varYaw = sinf(varRotation) * varAngle;

			//float d = Vec2f(currentTargetVector.z, currentTargetVector.x).length();
			float yaw = atan2f(currentTargetVector.x, currentTargetVector.z) + varYaw;
			float pitch = asinf(currentTargetVector.y) + varPitch;
			float pc = cosf(pitch);

			Vec3f newVector(pc * sinf(yaw), sinf(pitch), pc * cosf(yaw));

			float lengthVariance = 1.f;//random.randRange(0.125f, 1.f);
			//currentEmissionRate = (int)roundf(emissionRate * lengthVariance);
			Vec3f flatVector = newVector * (trajectorySpeed * lengthVariance);
			flatPos = startPos + (endPos - startPos) * t + flatVector;
		} else {
			flatPos = startPos + (endPos - startPos) * t;
		}

//		PARTICLE_LOG( "updating particle now @" + Vec3fToStr(flatPos) )

		Vec3f targetVector = endPos - startPos;
		//Vec3f currentVector = flatPos - startPos;

		// ratio
		//float t = clamp(currentVector.length() / targetVector.length(), 0.0f, 1.0f);

		// trajectory
		switch (trajectory) {
			case TrajectoryType::LINEAR:
				pos = flatPos;
				break;

			case TrajectoryType::PARABOLIC: {
					float scaledT = 2.0f * (t - 0.5f);
					float paraboleY = (1.0f - scaledT * scaledT) * trajectoryScale;

					pos = flatPos;
					pos.y += paraboleY;
				}
				break;

			case TrajectoryType::SPIRAL:
				pos = flatPos;
				pos += xVector * cosf(t * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				pos += yVector * sinf(t * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				break;

			case TrajectoryType::RANDOM:
				if (flatPos.dist(endPos) < 0.5f) {
					pos = flatPos;
				} else {
					pos = flatPos;
					//pos += xVector * cos(t*trajectoryFrequency*targetVector.length())*trajectoryScale;
					//pos += yVector * sin(t*trajectoryFrequency*targetVector.length())*trajectoryScale;
				}
				break;
			default:
				throw runtime_error("Unknown projectile trajectory: " + intToStr(trajectory));
		}
	}

	direction = pos - lastPos;
	direction.normalize();

	if (g_world.getFrameCount() == endFrame) {
		state = sFade;
		model = NULL;

		assert(callback);
		callback->projectileArrived(this);

		if (nextParticleSystem) {
			nextParticleSystem->setState(sPlay);
			nextParticleSystem->setPos(endPos);
			//nextParticleSystem->setDirection(direction);
			nextParticleSystem->checkVisibilty(ParticleUse::PROJECTILE, true);
		}
	}
	ParticleSystem::update();
	checkVisibilty(ParticleUse::PROJECTILE);
}

void Projectile::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float t = float(particleIndex) / emissionRate;

	p->pos = pos + (lastPos - pos) * t;
	p->lastPos = lastPos;
	p->speed = Vec3f(random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f)) * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);

	updateParticle(p);
}

void Projectile::updateParticle(Particle *p) {
	float energyRatio = p->getEnergyRatio();//clamp(float(p->energy) / energy, 0.f, 1.f);

	p->lastPos += p->speed;
	p->pos += p->speed;
	p->speed += p->accel;
	p->colour.value += p->colour.step;
	p->colour2.value += p->colour2.step;
	p->size.value += p->size.step;
	if (hasAngularVelocity()) {
		p->angle.value = fmodf(p->angle.value + p->angle.step, Math::twopi);
	}
	p->energy.current--;
}

void Projectile::setPath(Vec3f startPos, Vec3f endPos, int frames) {
	// compute axis
	zVector = endPos - startPos;
	zVector.normalize();
	yVector = Vec3f(0.0f, 1.0f, 0.0f);
	xVector = zVector.cross(yVector);

	// apply offset
	startPos += xVector * offset.x;
	startPos += yVector * offset.y;
	startPos += zVector * offset.z;

	pos = startPos;
	lastPos = startPos;
	flatPos = startPos;

	// recompute axis
	zVector = endPos - startPos;
	zVector.normalize();
	yVector = Vec3f(0.0f, 1.0f, 0.0f);
	xVector = zVector.cross(yVector);

	direction = zVector;

	// set members
	this->startPos = startPos;
	this->endPos = endPos;

	// calculate arrival (unless being told by server)
	if (frames  == -1) {
		Vec3f flatVector = zVector * trajectorySpeed;
		Vec3f traj = endPos - startPos;
		frames = int((traj.length() - 0.5f) / flatVector.length());
		frames = clamp(frames, 1, 255); // as dictated by Glest::Net::ProjectileUpdate (maybe give more resolution ?)
	}

	startFrame = g_world.getFrameCount();
	endFrame = startFrame + frames;
	// preProcess updates, when will it arrive ??
	PARTICLE_LOG( "Creating projectile @ " + Vec3fToStr(startPos) + " going to " + Vec3fToStr(endPos) )
	PARTICLE_LOG( "Projectile should arrive at " + intToStr(endFrame) )
}

// ===========================================================================
//  Splash
// ===========================================================================

Splash::Splash(bool visible, const ParticleSystemBase &model,  int particleCount)
		: GameParticleSystem(ParticleUse::SPLASH, visible, model, particleCount)
		//, prevParticleSystem(0)
		, emissionRateFade(1.f)
		, verticalSpreadA(1.f)
		, verticalSpreadB(0.f)
		, horizontalSpreadA(1.f)
		, horizontalSpreadB(0.f) {
}

Splash::~Splash() {
	//if (prevParticleSystem != NULL) {
	//	prevParticleSystem->nextParticleSystem = NULL;
	//}
}

void Splash::update() {
	ParticleSystem::update();
	if (state == sPlay) {
		emissionRate -= emissionRateFade;
		if (emissionRate <= 0.f) {
			state = sFade;
		}
	}
	checkVisibilty(ParticleUse::SPLASH);
}

void Splash::initParticle(Particle *p, int particleIndex) {
	p->pos = pos;
	p->lastPos = p->pos;
	p->energy.current = p->energy.start = energy;
	p->size.value = getRandomStartSize();
	float totalStep = getRandomEndSize() - p->size.value;
	p->size.step = totalStep / float(p->energy.current);
	p->colour.value = getColor();
	p->colour2.value = getColor2();
	p->colour.step = (getColorNoEnergy() - getColor()) / float(p->energy.current);
	p->colour2.step = (getColor2NoEnergy() - getColor2()) / float(p->energy.current);
	p->speed = Vec3f(
			horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB,
			verticalSpreadA * random.randRange(-1.0f, 1.0f) + verticalSpreadB,
			horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB);
	p->speed.normalize();
	p->speed = p->speed * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);
	p->texture = getNumTextures() > 1 ? random.randRange(0, getNumTextures() - 1) : 0;
}

void Splash::updateParticle(Particle *p) {
	float energyRatio = p->getEnergyRatio();//clamp(static_cast<float>(p->energy) / energy, 0.f, 1.f);

	p->lastPos = p->pos;
	p->pos = p->pos + p->speed;
	p->speed = p->speed + p->accel;
	p->energy.current--;
	p->colour.value += p->colour.step;
	p->size.value += p->size.step;
	if (hasAngularVelocity()) {
		p->angle.value = fmodf(p->angle.value + p->angle.step, Math::twopi);
	}
}

// ===========================================================================
//  UnitParticleSystem
// ===========================================================================

UnitParticleSystem::UnitParticleSystem(bool visible, const UnitParticleSystemType &protoType, int particleCount)
		: GameParticleSystem(ParticleUse::UNIT, visible, protoType, particleCount) {
	type = &protoType;
	
	rotation = 0.0f;
	cRotation = Vec3f(1.f, 1.f, 1.f);
	fixedAddition = Vec3f(0.f, 0.f, 0.f);

	direction = protoType.getDirection();
	relative = protoType.isRelative();
	relativeDirection = protoType.isRelativeDirection();
	fixed = protoType.isFixed();
	teamColorEnergy = protoType.hasTeamColorEnergy();
	teamColorNoEnergy = protoType.hasTeamColorNoEnergy();

	varParticleEnergy = protoType.getEnergyVar();
	maxParticleEnergy = protoType.getEnergy();
	
	if (type->getEmitterType() == EmitterPathType::ORBIT) {
		theta = random.randRange(0.f, Math::twopi);
		lastThetaDelta = 0.f;
	} else {
		theta = 0.f;
		lastThetaDelta = 0.f;
	}
}

UnitParticleSystem::~UnitParticleSystem() {
	DEBUG_HOOK();
}

void UnitParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	switch (primitiveType) {
		case PrimitiveType::QUAD:
			pr->renderSystem(this);
			break;
		case PrimitiveType::LINE:
			pr->renderSystemLine(this);
			break;
		default:
			assert(false);
	}
}

}}

namespace Shared { namespace Graphics { namespace Gl {

extern GLMatrix buildRotationMatrix(float angle, Vec3f axis);

}}}

namespace Glest { namespace Entities {

using namespace Shared::Graphics::Gl;

void UnitParticleSystem::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float ang = random.randRange(-2.f * pi, 2.f * pi);
	float mod = fabsf(random.randRange(-radius, radius));

	float x = sinf(ang) * mod;
	float y = cosf(ang) * mod;

	float radRatio = sqrtf(sqrtf(mod / radius));

	p->energy.start = p->energy.current = int(maxParticleEnergy * radRatio) + random.randRange(-varParticleEnergy, varParticleEnergy);

	p->lastPos = pos;
	oldPos = pos;

	p->speed.x = direction.x + direction.x * random.randRange(-0.5f, 0.5f);
	p->speed.y = direction.y + direction.y * random.randRange(-0.5f, 0.5f);
	p->speed.z = direction.z + direction.z * random.randRange(-0.5f, 0.5f);

	p->speed = p->speed * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);

	if (!relative) {
		p->pos += Vec3f(x + offset.x, random.randRange(-radius / 2.f, radius / 2.f) + offset.y, y + offset.z);
	} else { // rotate it according to rotation
		const float rad = degToRad(rotation);
		const float sinRad = sinf(rad);
		const float cosRad = cosf(rad);
		p->pos.x += x + offset.z * sinRad + offset.x * cosRad;
		p->pos.y += random.randRange(-radius / 2, radius / 2) + offset.y;
		p->pos.z += y + (offset.z * cosRad - offset.x * sinRad);
		if (relativeDirection) {
			p->speed.x = p->speed.z * sinRad + p->speed.x * cosRad;
			p->speed.z = p->speed.z * cosRad - p->speed.x * sinRad;
		}
	}
	if (type->getEmitterType() == EmitterPathType::ORBIT) {
		p->lastPos = p->pos;
		Vec3f axis = type->getEmitterAxis();
		// spread out from last update theta to current
		float myTheta = (lastThetaDelta != 0.f) ? theta - random.randRange(0.f, lastThetaDelta): theta;
		Vec3f perpendicularAxis; // find perpendicular axis
		if (axis.x == 0.f && axis.y == 0.f) { // special case for <0, 0, 1>
			perpendicularAxis = Vec3f(1.f, 0.f, 0.f);
		} else { // else chose arbitrary perpendicular <y, -x, 0> and normalise
			perpendicularAxis = Vec3f(axis.y, -axis.x, 0.f);
			perpendicularAxis.normalize();
		}
		GLMatrix rotationMatrix = buildRotationMatrix(myTheta, axis);
		// translate d units along perpendicular axis and rotate, add offset to particle pos
		p->pos += rotationMatrix * (perpendicularAxis * type->getEmitterDistance());

		myTheta -= lastThetaDelta;
		rotationMatrix = buildRotationMatrix(myTheta, axis);
		p->lastPos += rotationMatrix * (perpendicularAxis * type->getEmitterDistance());

	}
}

void UnitParticleSystem::update() {
	if (fixed) {
		fixedAddition = Vec3f(pos.x - oldPos.x, pos.y - oldPos.y, pos.z - oldPos.z);
		oldPos = pos;
	}
	ParticleSystem::update();
	checkVisibilty(ParticleUse::UNIT);
	
	//theta
	if (type->getEmitterType() == EmitterPathType::ORBIT) {
		const float dt = 1.f / 40.f;
		lastThetaDelta = type->getEmitterSpeed() * dt;
		theta = fmodf(theta + lastThetaDelta, Math::twopi);
	}
}

void UnitParticleSystem::updateParticle(Particle *p) {
	float energyRatio = p->getEnergyRatio();// clamp(float(p->energy) / maxParticleEnergy, 0.f, 1.f);

	p->lastPos += p->speed;
	p->pos += p->speed;
	if (fixed) {
		p->lastPos += fixedAddition;
		p->pos += fixedAddition;
	}
	p->speed += p->accel;
	p->colour.value += p->colour.step;
	p->colour2.value += p->colour2.step;
	p->size.value += p->size.step;
	if (hasAngularVelocity()) {
		p->angle.value = fmodf(p->angle.value + p->angle.step, Math::twopi);
	}
	--p->energy.current;
}

// ================= SET PARAMS ====================

/*void UnitParticleSystem::setWind(float windAngle, float windSpeed) {
	this->windSpeed.x = sinf(degToRad(windAngle)) * windSpeed;
	this->windSpeed.y = 0.f;
	this->windSpeed.z = cosf(degToRad(windAngle)) * windSpeed;
}*/

}}