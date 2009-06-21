// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
#include "path_finder_llsr.h"

using std::vector;
using std::list;
using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;
using Shared::Platform::int64;

namespace Glest{ namespace Game{

class Map;
class Unit;
class UnitPath;

// =====================================================
// 	class PathFinder
//
//	Finds paths for units using any one of numerous algorithms
// [see pathfinder_llsr.h]
// =====================================================

class PathFinder{
public:
	enum TravelState { tsArrived, tsOnTheWay, tsBlocked };

   static PathFinder* getInstance ();
	~PathFinder();
	void init(Map *map);
	TravelState findPath(Unit *unit, const Vec2i &finalPos);

   /* Sort these out !!! What stays, what goes... re-use refresh on nodeLimitReached ??? */
	static const int maxFreeSearchRadius;
	static const int pathFindRefresh; // now ignored.. but will be used (at higher val) for nodeLimitReached searches
   static const int pathFindNodesMax;

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
   AnnotatedMap *annotatedMap;

}; // class PathFinder

}}//end namespace

#endif
