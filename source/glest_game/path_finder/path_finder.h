// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//               2009      James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_PATHFINDER_H_
#define _GLEST_GAME_PATHFINDER_H_

#include "annotated_map.h"
#include "graph_search.h"

using std::vector;
using std::list;
using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;
using Shared::Platform::int64;

namespace Glest{ namespace Game{

class Map;
class Unit;
class UnitPath;

namespace PathFinder {

// Some 'globals' (oh no!!! run for cover...)

const int maxFreeSearchRadius = 10;
const int pathFindRefresh = 10; // now unused
const int pathFindNodesMax = 2048;// = Config::getInstance ().getPathFinderMaxNodes ();

const Vec2i Directions[8] = 
{
   Vec2i (  0, -1 ), // n
   Vec2i (  1, -1 ), // ne
   Vec2i (  0,  1 ), // e
   Vec2i (  1,  1 ), // se
   Vec2i (  1,  0 ), // s
   Vec2i ( -1,  1 ), // sw
   Vec2i ( -1,  0 ), // w
   Vec2i ( -1, -1 )  // nw
};

const Vec2i DirectionsSize2[12] = 
{
   Vec2i (  0, -1 ), // n1
   Vec2i (  1, -1 ), // n2
   Vec2i (  2, -1 ), // ne
   Vec2i (  2,  0 ), // e1
   Vec2i (  2,  1 ), // e2
   Vec2i (  2,  2 ), // se
   Vec2i (  1,  2 ), // s1
   Vec2i (  0,  2 ), // s2
   Vec2i ( -1,  2 ), // sw
   Vec2i ( -1,  1 ), // w1
   Vec2i ( -1,  0 ), // w2
   Vec2i ( -1, -1 )  // nw
};
   
enum TravelState { tsArrived, tsOnTheWay, tsBlocked };

// =====================================================
// 	class PathFinder
//
//	Finds paths for units using 
// 
// =====================================================
class PathFinder{
public:
   static PathFinder* getInstance ();
	~PathFinder();
	void init(Map *map);
	TravelState findPath(Unit *unit, const Vec2i &finalPos);

   // legal move ?
   bool isLegalMove ( Unit *unit, const Vec2i &pos ) const;

   // update the annotated map at pos 
   void updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field )
   { annotatedMap->updateMapMetrics ( pos, size, adding, field ); }

private:
   static PathFinder *singleton;
	PathFinder();

	Vec2i computeNearestFreePos (const Unit *unit, const Vec2i &targetPos);
	void copyToPath ( const list<Vec2i> pathList, UnitPath *path );
   Map *map;

public: // should be private ... debugging...
   AnnotatedMap *annotatedMap;
   GraphSearch *search;

}; // class PathFinder

}}}//end namespace

#endif
