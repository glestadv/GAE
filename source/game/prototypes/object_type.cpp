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
#include "object_type.h"

#include "renderer.h"

#include "world.h" // to get ModelFactory

#include "leak_dumper.h"

using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class ObjectType
// =====================================================

void ObjectType::init(int modelCount, int objectClass, bool walkable){
	models.reserve(modelCount);
	this->objectClass= objectClass;
	this->walkable= walkable;
}

void ObjectType::loadModel(const string &path){
	Model *model = g_world.getModelFactory().getModel(path, GameConstants::cellScale, 2);
	color = Vec3f(0.f);
	if (model->getMeshCount() > 0 && model->getMesh(0)->getTexture(0) != NULL) {
		const Pixmap2D *p = model->getMesh(0)->getTexture(0)->getPixmap();
		color = p->getPixel3f(p->getW()/2, p->getH()/2);
	}
	models.push_back(model);
}

}}//end namespace
