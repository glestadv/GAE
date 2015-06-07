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
#ifndef _GLEST_GAME_OBJECT_H_
#define _GLEST_GAME_OBJECT_H_

#include "model.h"
#include "vec.h"
#include "forward_decs.h"

namespace Glest { namespace Entities {

class MapResource;

using Shared::Graphics::Model;
using Shared::Math::Vec2i;
using Shared::Math::Vec3f;

using ProtoTypes::MapObjectType;
using ProtoTypes::ResourceType;
using Sim::EntityFactory;

// =====================================================
// 	class MapObject
//
///	A map object: tree, stone...
// =====================================================

class MapObject {
	friend class EntityFactory<MapObject>;

private:
	int m_id;
	MapObjectType *m_objectType;
	MapResource *m_resource;
	Vec2i m_tilePos;
	Vec3f m_pos;
	float m_rotation;
	int m_variation;


public:
	struct CreateParams {
		MapObjectType *objectType;
		Vec2i tilePos;
		const Vec3f pos;
		int seed;
		CreateParams(MapObjectType *objectType, const Vec2i &tilePos, const Vec3f &worldPos, int seed) 
			: objectType(objectType), tilePos(tilePos), pos(worldPos), seed(seed) {}
	};

private:
	MapObject(CreateParams params);
	~MapObject();

	void setId(int v) { m_id = v; }

public:
	void setPos(const Vec3f &pos)		{m_pos = pos;}

	int getId() const 					{return m_id;}
	const MapObjectType *getType() const{return m_objectType;}
	MapResource *getResource() const	{return m_resource;}
	Vec2i getTilePos() const            { return m_tilePos; }
	Vec3f getPos() const				{return m_pos;}
	float getRotation()	const			{return m_rotation;}
	const Model *getModel() const;
	bool getWalkable() const;

	void setResource(const ResourceType *resourceType, const Vec2i &pos);
};

typedef vector<MapObject*>          MapObjectVector;
typedef vector<const MapObject*>    ConstMapObjVector;

}}//end namespace

#endif
