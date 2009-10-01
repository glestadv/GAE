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
#include "search_engine.h"
#include "profiler.h"

#include <set>
#include <map>

using std::vector;
using std::list;
using std::map;

using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;

namespace Glest { namespace Game {

namespace Search {

class Cartographer;

// Some 'globals' (oh no!!! run for cover...)

const int maxFreeSearchRadius = 10;
//const int pathFindRefresh = 10; // now unused
const int pathFindNodesMax = 4096;//Config::getInstance().getPathFinderMaxNodes ();

struct SearchResult {
	enum State { Arrived, OnTheWay, Blocked };
};
typedef SearchResult::State TravelState;

//enum TravelState { tsArrived, tsOnTheWay, tsBlocked };

// =====================================================
// 	class PathManager
//
//	Finds paths for units using the SearchEngine class
//
//  Manages annotated maps for each team, and shared node
//  storage.  Performs group path calculations effeciently
//  using a reserved A*.
//  Generally tries to hide the horrible details of the 
//  templated SearchEngine::aStar() function.
// 
// =====================================================
class PathManager {	
public:
	static PathManager* getInstance();
	~PathManager();
	void init();

	static AnnotatedMap *annotatedMap; //MOVE ME
	Cartographer *cartographer;

	TravelState findPathToLocation( Unit *unit, const Vec2i &finalPos );
	TravelState findPath( Unit *unit, const Vec2i &finalPos ) { 
		return findPathToLocation( unit, finalPos ); 
	}

	bool repairPath( Unit *unit );

	// legal move ?
	bool isLegalMove( Unit *unit, const Vec2i &pos ) const;

	// update the annotated map at pos 
	void updateMapMetrics( const Vec2i &pos, const int size, bool adding, Field field ) { 
		PROFILE_START("AnnotatedMap::updateMapMetrics()");
		annotatedMap->updateMapMetrics( pos, size );
		PROFILE_STOP("AnnotatedMap::updateMapMetrics()");
	}

private:
	static PathManager *singleton;
	PathManager();

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);

#ifdef DEBUG_SEARCH_TEXTURES
public:
	enum { ShowPathOnly, ShowOpenClosedSets, ShowLocalAnnotations } debug_texture_action;
#endif
}; // class PathManager

}}}//end namespace

#endif
