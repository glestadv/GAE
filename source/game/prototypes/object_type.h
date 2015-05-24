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
#ifndef _GLEST_GAME_OBJECTTYPE_H_
#define _GLEST_GAME_OBJECTTYPE_H_

#include <vector>

#include "model.h"
#include "vec.h"

using std::vector;

namespace Glest { namespace ProtoTypes {

using Shared::Graphics::Model;
using Shared::Math::Vec3f;

// =====================================================
// 	class ObjectType  
//
///	Each of the possible objects of the map: trees, stones ...
// =====================================================

class MapObjectType {
private:
	typedef vector<Model*> Models;

private:
	static const int tree1 = 0;
	static const int tree2 = 1;
	static const int choppedTree = 2;

private:
	Models m_models;
	Vec3f  m_color;
	int    m_objectClass;
	bool   m_walkable;

public:
	void init( int modelCount, int objectClass, bool walkable );

	void loadModel( const string &path );

	Model *getModel( int i )        { return m_models[i]; }
	int getModelCount( ) const      { return m_models.size( ); }
	const Vec3f &getColor( ) const  { return m_color; } 
	int getClass( ) const           { return m_objectClass; }
	bool getWalkable( ) const       { return m_walkable; }
	bool isATree( ) const           { return m_objectClass == tree1 || m_objectClass == tree2; }
};

}}//end namespace

#endif
