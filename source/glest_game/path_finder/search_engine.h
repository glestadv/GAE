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
#	error search_engine.h included from file using pre-compiled header
#endif

#define SQRT2 1.41421356f

#include <limits>

#include "vec.h"
#include "unit_stats_base.h"
#include "game_constants.h"
#include "map.h"
#include "annotated_map.h"
#include "astar_nodepool.h"
#include "node_map.h"
#include "influence_map.h"

namespace Glest { namespace Game { namespace Search {

class NodeMap;

#define _SEARCH_ENGINE_GET_DIAGS_DEFINED_
__inline void getDiags( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 ) {
	assert( s.x != d.x && s.y != d.y );
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

// wrapped enum
#define WRAP_ENUM( Name, ... ) \
	struct Name { \
		enum Enum { __VA_ARGS__ }; \
		Name( Enum val ) : value( val ) {} \
		operator int() { return value; } \
	private: \
		Enum value; \
	};

WRAP_ENUM( AStarResult,
		   Failed, Complete, Partial, InProgress, Count
		 )

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
	void reset();
	void setNodeLimit( int limit );
	
	bool isOpen ( const Vec2i &pos );
	bool isClosed ( const Vec2i &pos );

	bool setOpen ( const Vec2i &pos, const Vec2i &prev, float h, float d );
	void updateOpen ( const Vec2i &pos, const Vec2i &prev, const float cost );
	Vec2i getBestCandidate();
	Vec2i getBestSeen();

	float getHeuristicAt( const Vec2i &pos );
	float getCostTo( const Vec2i &pos );
	float getEstimateFor( const Vec2i &pos );
	Vec2i getBestTo( const Vec2i &pos );
*/


// ========================================================
// class SearchEngine
//
// Wrapper for generic (templated) A*, contains all associated
// goal, cost and heuristic functions, and exposes an interface
// using different combinations for different purposes.
// ========================================================
//
// template SearchEngine on 'domain' (Vec2i for everything thus far)
// and on 'node storage' class (which is itself templated on the 'domain')
//
// Cost, Goal & Heuristic functions need to accept parameters of the domain
//
//TODO: More templating... generalise the node storage
//template< class NodeStorage, class Domain = Vec2i >
template< typename NodeStorage >
class SearchEngine {
private:
	NodeStorage *nodePool;

	// The goal pos (the 'result') from the last A* search
	Vec2i goalPos;
	int expandLimit, nodeLimit;
	int expanded;

public:
	SearchEngine() : expandLimit(-1), nodeLimit(-1), expanded(-1) { nodePool = NULL;}
	void init()  {
		delete nodePool;
		nodePool = new NodeStorage();
		nodeLimit = -1;
		expandLimit = -1;
		expanded = -1;
	}
	void reset() { nodePool->reset(); if ( nodeLimit > 0 ) { nodePool->setMaxNodes( nodeLimit ); } }
	void setOpen( Vec2i &pos, float h ) { 
		nodePool->setOpen( pos, Vec2i(-1), h, 0.f );
		//nodePool->addToOpen( NULL, pos, h, 0.f, true ); 
	}
	
	// reset and add pos to open
	void setStart( Vec2i &pos, float h ) {
		nodePool->reset();
		if ( nodeLimit > 0 ) {
			nodePool->setMaxNodes( nodeLimit );
		}
		nodePool->setOpen( pos, Vec2i(-1), h, 0.f );
		//nodePool->addToOpen( NULL, pos, h, 0.f, true );
	}

//	NODE_STRUCT* getGoal() { return goalNode; }
	Vec2i getGoalPos() { return goalPos; }
	Vec2i getPreviousPos( const Vec2i &pos ) { return nodePool->getBestTo( pos ); }
	
	// limit search to use at mose limit nodes
	void setNodeLimit( int limit ) { nodeLimit = limit; }

	// set an 'expanded nodes' limit, for a resumable serch
	void setTimeLimit( int limit ) { expandLimit = limit; }

	int getExpandedLastRun() { return expanded; }

	int pathToPos( const AnnotatedMap *map, const Unit *unit, const Vec2i &target ) {
		PosGoal::target = target;
		DiagonalDistance::target = target;
		MoveCost::map = map;
		MoveCost::unit = unit;
		return aStar<PosGoal,MoveCost,DiagonalDistance>();
	}

	int pathToInfluence( const AnnotatedMap *map, const Unit *unit, const Vec2i &target, 
			const InfluenceMap *resMap, float threshold ) {
		InfluenceGoal::iMap = iMap;
		InfluenceGoal::threshold = threshold;
		DiagonalDistance::target = target; // a bit hacky... target is needed for heuristic
		MoveCost::map = aMap;
		MoveCost::unit = unit;
		return aStar<InfluenceGoal,MoveCost,DiagonalDistance>();
	}

	void buildDistanceMap( InfluenceMap *iMap, float cutOff ) {
		InfluenceBuilderGoal::cutOff = cutOff;
		InfluenceBuilderGoal::iMap = iMap;
		aStar<InfluenceBuilderGoal,DistanceCost,ZeroHeuristic>();
	}

	// A* Algorithm (Just the loop, does not do any setup or post-processing)
	template< typename GoalFunc, typename CostFunc, typename Heuristic >
	int aStar() {
		expanded = 0;
		
		Vec2i minPos(-1);
		while ( true ) {
			minPos = nodePool->getBestCandidate();
			if ( minPos.x < 0 ) { // failure
				goalPos = Vec2i(-1);
				return AStarResult::Failed; 
			}
			if ( GoalFunc()(minPos, nodePool->getCostTo( minPos )) ) { // success
				goalPos = minPos;
				return AStarResult::Complete;
			}
			for ( int i = 0; i < 8; ++i ) {  // for each neighbour
				Vec2i sucPos = minPos + OffsetsSize1Dist1[i];
				if ( !theMap.isInside( sucPos ) || nodePool->isClosed( sucPos ) ) {
					continue;
				}
				float cost = CostFunc()( minPos, sucPos );
				if ( cost == numeric_limits<float>::infinity() ) {
					continue;
				}
				if ( nodePool->isOpen( sucPos ) ) {
					nodePool->updateOpen( sucPos, minPos, cost );
				} else {
					const float &costToMin = nodePool->getCostTo( minPos );
					if ( ! nodePool->setOpen( sucPos, minPos, Heuristic()( sucPos ), costToMin + cost ) ) {
						goalPos = nodePool->getBestSeen();
						return AStarResult::Partial;
					}
				}
			} 
			expanded++;
			if ( expanded == expandLimit ) {
				goalPos = Vec2i(-1);
				return AStarResult::InProgress;
			}
		} // while node limit not reached
		return -1; // impossible... just keeping the compiler from complaining
	}

	// Reverse Resumable A*, uses currentArray for storage
	// maxExpand: maximum number of nodes to expand this call ( < 0 will return Failed or Complete).
	//template< class GoalFunc, class CostFunc, class Heuristic >
	//int reverseAStar( int maxExpand = -1);
};

extern SearchEngine< AStarNodePool >	npSearchEngine;
extern SearchEngine< NodeMap >	 		nmSearchEngine;

}}}

#endif
