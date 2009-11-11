// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009      James McCulloch
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
#include "abstract_map.h"
#include "config.h"
#include "profiler.h"

#include "search_engine.h"

#include <set>
#include <map>

using std::vector;
using std::list;
using std::map;

using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;

namespace Glest { namespace Game { namespace Search {

/** maximum radius to search for a free position from a target position */
const int maxFreeSearchRadius = 10;
/** @deprecated not in use */
const int pathFindNodesMax = 2048;

/** number of NodePool objects to use, max 16, size of pools can be adjusted with NodePool::size
  * @todo this shouldn't be a const, but dynamically set based on number of players 
  * before a game starts */
const int numNodePools = 4;

/** @page pathfinding RoutePlanner Path Finding Strategy
  * <p>A new path request causes an immediate search with a 512 node NodePool.
  * if that search breaches the node limit, a partial path is returned and 
  * the RouteInfo saved on the longSearchQueue. If there are still NodePools 
  * free (to cintinue servicing new requests), the full NodePool is set aside 
  * and referenced in the RouteInfo.</p>
  * <p>If time is of the essence, initial searches will be performed with an over estimating 
  * heuristic, and node limited searches will be put straight onto the hardSearchQueue.</p>
  * <p>When the RoutePlanner's burnCPU() method is called (at the end of a world update)
  * the long search queue is consulted, any searches with attached NodePools are serviced
  * first, a second node pool is attached to the first and the search resumed. Reaching
  * the node limit again will cause either more node pools to be added in an attempt to get 
  * a path, or the search abandoned and the RouteInfo placed on the hardSearchQueue. The
  * exact policy is undecided at this point, and will no doubt require tweaking.</p>
  * <p>Searches deemed too long/hard to complete with node pools are put on the hard search queue.
  * Hard searches are performed using the NodeMap as node storage, and must be co-ordinated with
  * the Cartographer, owner and monopoliser of the NodeMap.  When the RoutePlanner has hard searches
  * to perform, it notifies the Cartographer, it is then up to the Cartographer to call the 
  * RoutePlanner back when the NodeMap is free and can be borrowed.</p>
  */

// =====================================================
// 	class RoutePlanner
// =====================================================
/**	Finds paths for units using the SearchEngine
  * <p>Performs group path calculations effeciently
  * using a reverse A*.</p>
  * <p>Generally tries to hide the horrible details of the 
  * SearchEngine<>::aStar<>() function.</p>
  * 
  * @see @ref pathfinding
  */ 
class RoutePlanner {
	/** A unit collection and a goal position for a reverse A* search 
	  */
	class GroupInfo : protected pair<vector<Unit*>,Vec2i> {
	public:
		GroupInfo(Vec2i &goal)		{ second = goal; }
		GroupInfo(vector<Unit*> &units, Vec2i &goal) { 
			copy(units.begin(),units.end(),first.begin()); 
			second = goal; 
		}

		vector<Unit*>& getUnits()	{ return first;		}
		Vec2i getGoal()	const		{ return second;	}

		void addUnit(Unit *unit)	{ first.push_back(unit); }
		void remUnit(Unit *unit)	{ first.erase(find(first.begin(), first.end(), unit)); }
	};
	/** A unit and goal position pair and an optional (full) NodeStore.
	  * <p>If a search with the default node limit fails, a partial path is returned and a RouteInfo
	  * is constructed and placed on the 'long search' queue. If there are sufficient node pools 
	  * to continue servicing new requests then the 'full' node pool can be saved, and the long search
	  * can then resume the search by attaching a second node pool to the first one.</p>
	  */
	class RouteInfo : protected pair<Unit*,Vec2i> {
	public:
		RouteInfo(Unit *unit, Vec2i goal, NodePool *pool = NULL) 
				: pair<Unit*,Vec2i>(unit,goal), pool(pool) {}

		Unit*		getUnit() const	{ return first;	}
		Vec2i		getGoal() const	{ return second;}
		bool		hasPool() const	{ return pool;	}
		NodePool*	getPool() const { return pool;	}

	private:
		NodePool *pool;
	};

public:
	static RoutePlanner* getInstance();
	~RoutePlanner();
	void init();

	TravelState findPathToLocation(Unit *unit, const Vec2i &finalPos);
	/** @see findPathToLocation() */
	TravelState findPath(Unit *unit, const Vec2i &finalPos) { 
		return findPathToLocation(unit, finalPos); 
	}
	bool repairPath(Unit *unit);
	bool isLegalMove(Unit *unit, const Vec2i &pos) const;

private:
	float quickSearch(const Unit *unit, const Vec2i &dest);
	void openBorders(const Unit *unit, const Vec2i &dest);
	bool findAbstractPath(const Unit *unit, const Vec2i &dest, WaypointPath &waypoints);
	static RoutePlanner *singleton;
	SearchEngine<NodeStore,GridNeighbours>	 *nsgSearchEngine;
	SearchEngine<AbstractNodeStorage,BorderNeighbours,const Border*> *nsbSearchEngine;

	RoutePlanner();

	queue<RouteInfo*> longSearchQueue;  /**< queue for long searches		*/
	queue<RouteInfo*> hardSearchQueue;  /**< queue for hard case searches	*/
	queue<GroupInfo*> groupSearchQueue; /**< queue for group searches		*/

	Vec2i computeNearestFreePos(const Unit *unit, const Vec2i &targetPos);

	NodePool *nodePool;
	AbstractMap *abstractMap;

	bool attemptMove(Unit *unit) const {
		Vec2i pos = unit->getPath()->peek(); 
		if ( isLegalMove(unit, pos) ) {
			unit->setNextPos(pos);
			unit->getPath()->pop();
			return true;
		}
		return false;
	}


#if DEBUG_SEARCH_TEXTURES
public:
	enum { SHOW_PATH_ONLY, SHOW_OPEN_CLOSED_SETS, SHOW_LOCAL_ANNOTATIONS } debug_texture_action;
#endif
}; // class RoutePlanner

class AbstractAssistedHeuristic {
public:
	AbstractAssistedHeuristic(const Vec2i &target, const Vec2i &waypoint, float wpCost) 
			: target(target), waypoint(waypoint), wpCost(wpCost) {}
	/** search target */
	Vec2i target, waypoint;	
	float wpCost;
	/** The heuristic function.
	  * @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target
	  */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - waypoint.x), 
			  dy = (float)abs(pos.y - waypoint.y);
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return  1.4 * diag + straight + wpCost;

	}
};

}}}//end namespace

#endif
