// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "game_camera.h"

#include <cstdlib>

#include "config.h"
#include "game_constants.h"
#include "metrics.h"
#include "logger.h"
#include "xml_parser.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using Shared::Xml::XmlNode;

namespace Glest { namespace Game {

// =====================================================
// 	class GameCamera
// =====================================================

// ================== PUBLIC =====================

const float GameCamera::startingVAng= -60.f;
const float GameCamera::startingHAng= 0.f;
const float GameCamera::vTransitionMult= 0.125f;
const float GameCamera::hTransitionMult= 0.125f;
const float GameCamera::defaultHeight= 20.f;
const float GameCamera::centerOffsetZ= 8.0f;

// ================= Constructor =================

GameCamera::GameCamera() : pos(0.f, defaultHeight, 0.f),
		destPos(0.f, defaultHeight, 0.f), destAng(startingVAng, startingHAng) {
	Config &config = Config::getInstance();
    state= sGame;

	//config
	speed= 15.f / GameConstants::cameraFps;
	clampBounds= !Config::getInstance().getUiPhotoMode();

	vAng= startingVAng;
    hAng= startingHAng;

    rotate=0;

	move= Vec3f(0.f);

	maxRenderDistance = config.getRenderDistanceMax();
	maxHeight = config.getCameraMaxDistance();
	minHeight = config.getCameraMinDistance();
	maxCameraDist = config.getCameraMaxDistance();
	minCameraDist = config.getCameraMinDistance();
	minVAng = -config.getCameraMaxYaw();
	maxVAng = -config.getCameraMinYaw();
	fov = config.getCameraFov();
}

void GameCamera::init(int limitX, int limitY){
	this->limitX= limitX;
	this->limitY= limitY;
}

// ==================== Misc =====================

void GameCamera::setPos(Vec2f pos){
	this->pos= Vec3f(pos.x, this->pos.y, pos.y);
	clampPosXZ(0.0f, (float)limitX, 0.0f, (float)limitY);
	destPos.x = pos.x;
	destPos.z = pos.y;
}

void GameCamera::update(){

	//move XZ
	if(move.z){
        moveForwardH(speed * move.z, 0.9f);
	}
	if(move.x){
        moveSideH(speed * move.x, 0.9f);
	}

	//free state
	if(state==sFree){
		if(fabs(rotate) == 1){
			rotateHV(speed*5*rotate, 0);
		}
		if(move.y>0){
			moveUp(speed * move.y);
			if(clampBounds && pos.y<maxHeight){
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
		if(move.y<0){
			moveUp(speed * move.y);
			if(clampBounds && pos.y>minHeight){
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
	}

	//game state
	if(abs(destAng.x - vAng) > 0.01f) {
		vAng+= (destAng.x - vAng) * hTransitionMult;
	}
	if(abs(destAng.y - hAng) > 0.01f) {
		if(abs(destAng.y - hAng) > 180) {
			if(destAng.y > hAng) {
				hAng+= (destAng.y - hAng - 360) * vTransitionMult;
			} else {
				hAng+= (destAng.y - hAng + 360) * vTransitionMult;
			}
		} else {
			hAng+= (destAng.y - hAng) * vTransitionMult;
		}
	}
	if(abs(destPos.x - pos.x) > 0.01f) {
		pos.x += (destPos.x - pos.x) / 32.0f;
	}
	if(abs(destPos.y - pos.y) > 0.01f) {
		pos.y += (destPos.y - pos.y) / 32.0f;
	}
	if(abs(destPos.z - pos.z) > 0.01f) {
		pos.z += (destPos.z - pos.z) / 32.0f;
	}

	clampAng();

	if(clampBounds){
		clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
	}
}

Quad2i GameCamera::computeVisibleQuad() const{
	float aspectRatio = Metrics::getInstance().getAspectRatio();
	//Vec2i v= Vec2i(static_cast<int>(pos.x), static_cast<int>(pos.z));

	const float nearDist= 20.f;
	// farDist needs to be calculated based on the vertical pitch of the camera
	const float farDist= 90.f;
	const float fov= 65.0f * aspectRatio * 0.5f;
	const float dist= 20.f;
	
	Vec2f v(sin(degToRad(180-hAng)), cos(degToRad(180-hAng)));
	Vec2f v1(sin(degToRad(180-hAng-fov)), cos(degToRad(180-hAng-fov)));
	Vec2f v2(sin(degToRad(180-hAng+fov)), cos(degToRad(180-hAng+fov)));
	v.normalize();
	v1.normalize();
	v2.normalize();

	Vec2f p= Vec2f(pos.x, pos.z)-v*dist;
	Vec2i p1(static_cast<int>(p.x+v1.x*nearDist), static_cast<int>(p.y+v1.y*nearDist));
	Vec2i p2(static_cast<int>(p.x+v1.x*farDist), static_cast<int>(p.y+v1.y*farDist));
	Vec2i p3(static_cast<int>(p.x+v2.x*nearDist), static_cast<int>(p.y+v2.y*nearDist));
	Vec2i p4(static_cast<int>(p.x+v2.x*farDist), static_cast<int>(p.y+v2.y*farDist));

	if(hAng>=135 && hAng<=225){
		return Quad2i(p1, p2, p3, p4);
	}
	if(hAng>=45 && hAng<=135){
		return Quad2i(p3, p1, p4, p2);
	}	
	if(hAng>=225 && hAng<=315) {
		return Quad2i(p2, p4, p1, p3);
	}
	/*
	Quad2i q2i (p4, p3, p2, p1);
	static char buffer[1024];
	sprintf ( buffer, "{(%d,%d),(%d,%d),(%d,%d),(%d,%d)}",
		p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y );
	Logger::getInstance().add ( buffer );
	*/
	return Quad2i(p4, p3, p2, p1);
}

void GameCamera::switchState(){
	if(state==sGame){
		state= sFree;
	}
	else{
		state= sGame;
		destAng.x = startingVAng;
		destAng.y = startingHAng;
		destPos.y = defaultHeight;
	}
}

void GameCamera::centerXZ(float x, float z){
	destPos.x = pos.x= x;
	destPos.z = pos.z= z+centerOffsetZ;
}

void GameCamera::transitionXYZ(float x, float y, float z) {
	destPos.x += x;
	destPos.y += y;
	destPos.z += z;
	clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
}

void GameCamera::transitionVH(float v, float h) {
	destAng.x += v;
	destPos.y -= v * destPos.y / 100.f;
	destAng.y += h;
	clampAng();
}

void GameCamera::zoom(float dist) {
	float flatDist = dist * cosf(degToRad(vAng));
	Vec3f offset(flatDist * sinf(degToRad(hAng)), dist * sinf(degToRad(vAng)), flatDist  * -cosf(degToRad(hAng)));
	float mult = 1.f;
	if(destPos.y + offset.y < minHeight) {
		mult = abs((destPos.y - minHeight) / offset.y);
	} else if(destPos.y + offset.y > maxHeight) {
		mult = abs((maxHeight - destPos.y) / offset.y);
	}
	destPos += offset * mult;
}

void GameCamera::load(const XmlNode *node) {
	destPos = node->getChildVec3fValue("pos");
	destAng = node->getChildVec2fValue("angle");
}

void GameCamera::save(XmlNode *node) const {
	node->addChild("pos", pos);
	node->addChild("angle", Vec2f(vAng, hAng));
}

// ==================== PRIVATE ====================

void GameCamera::clampPosXZ(float x1, float x2, float z1, float z2){
	if(pos.x < x1)		pos.x = x1;
	if(destPos.x < x1)	destPos.x = x1;
	if(pos.z < z1)		pos.z = z1;
	if(destPos.z < z1)	destPos.z = z1;
	if(pos.x > x2)		pos.x = x2;
	if(destPos.x > x2)	destPos.x = x2;
	if(pos.z > z2)		pos.z = z2;
	if(destPos.z > z2)	destPos.z = z2;
}

void GameCamera::clampPosXYZ(float x1, float x2, float y1, float y2, float z1, float z2){
	if(pos.x < x1)		pos.x = x1;
	if(destPos.x < x1)	destPos.x = x1;
	if(pos.y < y1)		pos.y = y1;
	if(destPos.y < y1)	destPos.y = y1;
	if(pos.z < z1)		pos.z = z1;
	if(destPos.z < z1)	destPos.z = z1;
	if(pos.x > x2)		pos.x = x2;
	if(destPos.x > x2)	destPos.x = x2;
	if(pos.y > y2)		pos.y = y2;
	if(destPos.y > y2)	destPos.y = y2;
	if(pos.z > z2)		pos.z = z2;
	if(destPos.z > z2)	destPos.z = z2;
}

void GameCamera::rotateHV(float h, float v){
	destAng.x = vAng += v;
	destAng.y = hAng += h;
	clampAng();
}

void GameCamera::clampAng() {
	if(vAng > maxVAng)		vAng = maxVAng;
	if(destAng.x > maxVAng)	destAng.x = maxVAng;
	if(vAng < minVAng)		vAng = minVAng;
	if(destAng.x < minVAng)	destAng.x = minVAng;
	if(hAng > 360.f)		hAng -= 360.f;
	if(destAng.y > 360.f)	destAng.y -= 360.f;
	if(hAng < 0.f)			hAng += 360.f;
	if(destAng.y < 0.f)		destAng.y = 360.f;
}

//move camera forwad but never change heightFactor
void GameCamera::moveForwardH(float d, float response) {
	Vec3f offset(sinf(degToRad(hAng)) * d, 0.f, -cosf(degToRad(hAng)) * d);
	destPos += offset;
	pos.x += offset.x * response;
	pos.z += offset.z * response;
}

//move camera to a side but never change heightFactor
void GameCamera::moveSideH(float d, float response){
	Vec3f offset(sinf(degToRad(hAng+90)) * d, 0.f, -cosf(degToRad(hAng+90)) * d);
	destPos += offset;
	pos.x += (destPos.x - pos.x) * response;
	pos.z += (destPos.z - pos.z) * response;
}

void GameCamera::moveUp(float d){
//	pos.y+= d;
	destPos.y += d;
}

}}//end namespace
