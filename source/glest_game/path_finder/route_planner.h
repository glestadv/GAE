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

#include "game_constants.h"
#include "influence_map.h"
#include "annotated_map.h"
#include "config.h"
#include "profiler.h"

#include <set>
#include <map>

using std::vector;
using std::list;
using std::map;

using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;

namespace Glest { namespace Game { namespace Search {

const int maxFreeSearchRadius = 10;
const int pathFindNodesMax = 2048;

struct SearchResult {
	enum State { Arrived, OnTheWay, Blocked };
};
typedef SearchResult::State TravelState;

//enum TravelState { tsArrived, tsOnTheWay, tsBlocked };

// =====================================================
// 	class RoutePlanner
//
//	Finds paths for units using the SearchEngine
//
//  Performs group path calculations effeciently
//  using a reverse A*.
//  Generally tries to hide the horrible details of the 
//  SearchEngine<>::aStar<>() function.
// 
// =====================================================
class RoutePlanner {	
public:
	static RoutePlanner* getInstance();
	~RoutePlanner();
	void init();

	TravelState findPathToLocation( Unit *unit, const Vec2i &finalPos );
	TravelState findPath( Unit *unit, const Vec2i &finalPos ) { 
		return findPathToLocation( unit, finalPos ); 
	}
	bool repairPath( Unit *unit );
	bool isLegalMove( Unit *unit, const Vec2i &pos ) const;

private:
	static RoutePlanner *singleton;
	RoutePlanner();

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);

#if DEBUG_SEARCH_TEXTURES
public:
	enum { ShowPathOnly, ShowOpenClosedSets, ShowLocalAnnotations } debug_texture_action;
#endif
}; // class RoutePlanner

}}}//end namespace

#endif
