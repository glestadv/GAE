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
// search_engine.h

#ifndef _GLEST_GAME_SEARCH_ENGINE_
#define _GLEST_GAME_SEARCH_ENGINE_

#ifdef _SHARED_PCH_H_
//#	error search_engine.h included from file using pre-compiled header
#endif

#define SQRT2 1.41421356f

#include <limits>

#include "vec.h"
#include "unit_stats_base.h"
#include "game_constants.h"
#include "map.h"
#include "annotated_map.h"
#include "node_pool.h"
#include "node_map.h"
#include "influence_map.h"

namespace Glest { namespace Game { 
/** Home of the templated A* algorithm and the primary users of it.*/
namespace Search {

class NodeMap;

/** gets the two 'diagonal' cells to check for obstacles when a unit is moving diagonally
  * @param s start pos
  * @param d destination pos
  * @param size size of unit
  * @return d1 & d2, the two cells to check
  * @warning assumes s & d are indeed diagonal, abs(s.x - d.x) == 1 && abs(s.y - d.y) == 1
  */
__inline void getDiags( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 ) {
#	define _SEARCH_ENGINE_GET_DIAGS_DEFINED_
	assert( abs( s.x - d.x ) == 1 && abs( s.y - d.y ) == 1 );
	if ( size == 1 ) {
		d1.x = s.x; d1.y = d.y;
		d2.x = d.x; d2.y = s.y;
		return;
	}
	if ( d.x > s.x ) {  // travelling east
		if ( d.y > s.y ) {  // se
			d1.x = d.x + size - 1; d1.y = s.y;
			d2.x = s.x; d2.y = d.y + size - 1;
		} else {  // ne
			d1.x = s.x; d1.y = d.y;
			d2.x = d.x + size - 1; d2.y = s.y - size + 1;
		}
	} else {  // travelling west
		if ( d.y > s.y ) {  // sw
			d1.x = d.x; d1.y = s.y;
			d2.x = s.x + size - 1; d2.y = d.y + size - 1;
		} else {  // nw
			d1.x = d.x; d1.y = s.y - size + 1;
			d2.x = s.x + size - 1; d2.y = d.y;
		}
	}
}

// need to have seen getDiags() defined
#include "search_functions.inl"

const int numOffsetsSize1Dist1 = 8;
const Vec2i OffsetsSize1Dist1 [numOffsetsSize1Dist1] = {
	Vec2i (  0, -1 ), // n
	Vec2i (  1, -1 ), // ne
	Vec2i (  0,  1 ), // e
	Vec2i (  1,  1 ), // se
	Vec2i (  1,  0 ), // s
	Vec2i ( -1,  1 ), // sw
	Vec2i ( -1,  0 ), // w
	Vec2i ( -1, -1 )  // nw
};

/*
	// NodeStorage template interface
	//
	template<typename T>
	class NodeStorage {
	public:
		void reset();
		void setMaxNode( int limit );
		
		bool isOpen ( const T &pos );
		bool isClosed ( const T &pos );

		bool setOpen ( const T &pos, const T &prev, float h, float d );
		void updateOpen ( const T &pos, const T &prev, const float cost );
		T getBestCandidate();
		T getBestSeen();

		float getHeuristicAt( const T &pos );
		float getCostTo( const T &pos );
		float getEstimateFor( const T &pos );
		T getBestTo( const T &pos );
	};
*/
/*
	// Domain Interface
	//
	template<typename T>
	class SearchDomain {
	public:
		
	};

*/

// ========================================================
// class SearchEngine
//
// Wrapper for generic (templated) A*, contains all associated
// goal, cost and heuristic functions, and exposes an interface
// using different combinations for different purposes.
// ========================================================
//
// template SearchEngine on 'domain' (Grid with Vec2i 'key' for everything thus far)
//
// Cost, Goal & Heuristic functions need to accept parameters of the domain key
//
//template< typename NodeStorage, typename Domain = CellMapDomain<Vec2i>, typename DomainKey = Vec2i >
/** The home of the templated A* algorithm 
  * @param NodeStorage templated NodeStorage, must conform to implicit interface see elsewhere
  */
template< typename NodeStorage >
class SearchEngine {
private:
	NodeStorage *nodeStorage; /**< NodeStorage for this SearchEngine */

	Vec2i goalPos;		/**< The goal pos (the 'result') from the last A* search */
	int expandLimit,	/**< limit on number of nodes to expand */
		nodeLimit,		/**< limit on number of nodes to use */
		expanded,		/**< number of nodes expanded this/last run */
		spaceWidth,		/**< kludge because domain isn't templated yet, width of search space */
		spaceHeight;	/**< kludge because domain isn't templated yet, height of search space */

public:
	/** construct & initialise NodeStorage */
	SearchEngine() 
			: expandLimit(-1)
			, nodeLimit(-1)
			, expanded(0)
			, spaceWidth(theMap.getW())
			, spaceHeight(theMap.getH()) {
		nodeStorage = new NodeStorage(spaceWidth, spaceHeight);
		spaceWidth = theMap.getW();
		spaceHeight = theMap.getH();
	}
	~SearchEngine() { 
		delete nodeStorage; 
	}

	/** @return a pointer to this engines node storage */
	NodeStorage* getStorage()	{ return nodeStorage; }

	/** Reset the node storage */
	void reset() { nodeStorage->reset(); nodeStorage->setMaxNodes(nodeLimit > 0 ? nodeLimit : -1); }
	/** Add a position to the open set with 0 cost to here (a start position)
	  * @param pos position to set as open
	  * @param h heuristc, estimate to goal from pos
	  */
	void setOpen(Vec2i &pos, float h)	{ nodeStorage->setOpen(pos, Vec2i(-1), h, 0.f); }
	
	/** Reset the node storage and add pos to open 
	  * @param pos position to set as open
	  * @param h heuristc, estimate to goal from pos
	  */
	void setStart(Vec2i &pos, float h) {
		nodeStorage->reset();
		if ( nodeLimit > 0 ) {
			nodeStorage->setMaxNodes(nodeLimit);
		}
		nodeStorage->setOpen(pos, Vec2i(-1), h, 0.f);
	}
	/** Retrieve the goal of last search, position of last goal reached */
	Vec2i getGoalPos() { return goalPos; }
	/** Best path to pos is from */
	Vec2i getPreviousPos(const Vec2i &pos) { return nodeStorage->getBestTo(pos); }
	
	/** limit search to use at mose limit nodes */
	void setNodeLimit(int limit) { nodeLimit = limit > 0 ? limit : -1; }
	/** set an 'expanded nodes' limit, for a resumable serch */
	void setTimeLimit(int limit) { expandLimit = limit > 0 ? limit : -1; }

	/** How many nodes were expanded last search */
	int getExpandedLastRun() { return expanded; }

	/** Find a path for unit to target using map */
	int pathToPos(const AnnotatedMap *map, const Unit *unit, const Vec2i &target) {
		PosGoal goalFunc(target);
		MoveCost costFunc (unit, map);
		DiagonalDistance heuristic(target);
		return aStar<PosGoal,MoveCost,DiagonalDistance>(goalFunc,costFunc,heuristic);
	}

	/** Finds a path for unit towards target using map, terminating when a cell with more than
	  * threshold influence on iMap is found */
	int pathToInfluence(const AnnotatedMap *map, const Unit *unit, const Vec2i &target, 
			const TypeMap<float> *iMap, float threshold) {
		InfluenceGoal<float> goalFunc(threshold, iMap);
		MoveCost			 costFunc(unit, aMap);
		DiagonalDistance	 heuristic(target); // a bit hacky... target is needed for heuristic
		return aStar<InfluenceGoal,MoveCost,DiagonalDistance>(goalFunc,costFunc,heuristic);
	}

	/** Runs a Dijkstra search, setting distance data in iMap, until cutOff is reached */
	void buildDistanceMap(TypeMap<float> *iMap, float cutOff) {
		InfluenceBuilderGoal goalFunc(cutOff, iMap);
		aStar<InfluenceBuilderGoal,DistanceCost,ZeroHeuristic>(goalFunc,DistanceCost(),ZeroHeuristic());
	}

	/** Kludge to search on Cellmap or Tilemap... templated search domain will deprecate this */
	void setSearchSpace(SearchSpace s) {
		if ( s == SearchSpace::CELLMAP ) {
			spaceWidth = theMap.getW();
			spaceHeight = theMap.getH();
		} else if ( s == SearchSpace::TILEMAP ) {
			spaceWidth = theMap.getTileW();
			spaceHeight = theMap.getTileH();
		}
	}

	/** A* Algorithm (Just the loop, does not do any setup or post-processing) 
	  * @param GoalFunc <b>template parameter</b> Goal function class
	  * @param CostFunc <b>template parameter</b> Cost function class
	  * @param Heuristic <b>template parameter</b> Heuristic function class
	  * @param goalFunc goal function object
	  * @param costFunc cost function object
	  * @param heuristic heuristic function object
	  */
	template< typename GoalFunc, typename CostFunc, typename Heuristic >
	int aStar(GoalFunc goalFunc, CostFunc costFunc, Heuristic heuristic) {
		expanded = 0;
		
		Vec2i minPos(-1);
		while ( true ) {
			minPos = nodeStorage->getBestCandidate();
			if ( minPos.x < 0 ) { // failure
				goalPos = Vec2i(-1);
				return AStarResult::FAILED; 
			}
			if ( goalFunc(minPos, nodeStorage->getCostTo(minPos)) ) { // success
				goalPos = minPos;
				return AStarResult::COMPLETE;
			}
			for ( int i = 0; i < 8; ++i ) {  // for each neighbour
				Vec2i nPos = minPos + OffsetsSize1Dist1[i];
				if ( nPos.x < 0 || nPos.y < 0 || nPos.x > spaceWidth || nPos.y > spaceHeight
				||	 nodeStorage->isClosed(nPos) ) {
					continue;
				}
				float cost = costFunc(minPos, nPos);
				if ( cost == numeric_limits<float>::infinity() ) {
					continue;
				}
				if ( nodeStorage->isOpen(nPos) ) {
					nodeStorage->updateOpen(nPos, minPos, cost);
				} else {
					const float &costToMin = nodeStorage->getCostTo(minPos);
					if ( ! nodeStorage->setOpen(nPos, minPos, heuristic(nPos), costToMin + cost) ) {
						goalPos = nodeStorage->getBestSeen();
						return AStarResult::PARTIAL;
					}
				}
			} 
			expanded++;
			if ( expanded == expandLimit ) {
				goalPos = Vec2i(-1);
				return AStarResult::INPROGRESS;
			}
		} // while node limit not reached
		return -1; // impossible... just keeping the compiler from complaining
	}
};

//extern SearchEngine<NodeStore>	*nsSearchEngine;
//extern SearchEngine<NodeMap>	*nmSearchEngine;

}}}

#endif
