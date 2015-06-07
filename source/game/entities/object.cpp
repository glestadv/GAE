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
#include "object.h"

#include "faction_type.h"
#include "tech_tree.h"
#include "resource.h"
#include "upgrade.h"
#include "object_type.h"
#include "resource.h"
#include "util.h"
#include "random.h"
#include "user_interface.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Entities {

// =====================================================
// 	class MapObject
// =====================================================

MapObject::MapObject(CreateParams params)//MapObjectType *objType, const Vec3f &p)
		: m_id(-1)
		, m_objectType(params.objectType)
		, m_resource(nullptr) {
	//int seed = int(Chrono::getCurMicros());
	Random random(params.seed);
	const float max_offset = 0.2f;
	m_tilePos = params.tilePos;
	m_pos = params.pos + Vec3f(random.randRange(-max_offset, max_offset), 0.0f, random.randRange(-max_offset, max_offset));
	m_rotation = random.randRange(0.f, 360.f);
	if (m_objectType != NULL) {
		m_variation = random.randRange(0, m_objectType->getModelCount() - 1);
	}
}

MapObject::~MapObject() {
	delete m_resource;
}

const Model *MapObject::getModel() const {
	return m_objectType == nullptr ? m_resource->getType()->getModel() : m_objectType->getModel(m_variation);
}

bool MapObject::getWalkable() const{
	return m_objectType == nullptr ? false : m_objectType->getWalkable();
}

void MapObject::setResource(const ResourceType *resourceType, const Vec2i &pos) {
    assert( !m_resource );
	m_resource = new MapResource();
	m_resource->init(resourceType, pos);
	m_resource->Depleted.connect(&g_userInterface, &Gui::UserInterface::onResourceDepleted);
}

}}//end namespace
