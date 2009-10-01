// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
//
// search_engine.cpp

//#include "pch.h"
#include <limits>
#include <algorithm>

#include "search_engine.h"
#include "path_finder.h"
#include "node_map.h"

namespace Glest { namespace Game { namespace Search {

SearchEngine<AStarNodePool> npSearchEngine;
//SearchEngine<NodeMap> nmSearchEngine;

Vec2i			PosGoal::target( -1 );
Vec2i		  RangeGoal::target( -1 );
float		  RangeGoal::range( 0.f );
Field	   FreeCellGoal::field( FieldWalkable );
const InfluenceMap* 
		  InfluenceGoal::iMap = NULL;
float	  InfluenceGoal::threshold = 0.f;
Vec2i  DiagonalDistance::target( -1 );
Vec2i	   OverEstimate::target( -1 );
float
   InfluenceBuilderGoal::cutOff = 0.f;
InfluenceMap* 
   InfluenceBuilderGoal::iMap = NULL;
const Unit    *MoveCost::unit = NULL;
const AnnotatedMap*
			   MoveCost::map = NULL;



}}}