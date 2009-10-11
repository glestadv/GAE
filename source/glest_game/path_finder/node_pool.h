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
// File: node_pool.h
//

#ifndef _GLEST_GAME_PATHFINDER_ASTAR_NODEPOOL_H_
#define _GLEST_GAME_PATHFINDER_ASTAR_NODEPOOL_H_

#include "vec.h"
#include "timer.h"
#include "unit_stats_base.h"

using Shared::Platform::Chrono;

namespace Glest { namespace Game { 

class Unit;
class Map;

namespace Search {

// =====================================================
// struct AStarNode
//
// A node structure for A* and friends
// =====================================================
struct AStarNode {
	
	Vec2i pos;			// 8 bytes
	AStarNode *prev;	// 4|8 bytes

	//uint32 mark			// 4 bytes
	//PackedPos bestToHere, // 3 bytes
	//			nextOpen;	// 3 bytes

	float heuristic;
	float distToHere;
	bool exploredCell; // <-- vis status bound for AnnotatedMap, then remove this
	float est() const { return distToHere + heuristic; }
}; // remove explored then sizeof()==20[|24]

// Comparison function for the heap
class AStarComp {
public:
	bool operator()( const AStarNode * const one, const AStarNode * const two ) const {
		const float diff = ( one->distToHere + one->heuristic ) - ( two->distToHere + two->heuristic );
		if ( diff < 0 ) return false;
		else if ( diff > 0 ) return true;
		// tie, prefer closer to goal...
		if ( one->heuristic < two->heuristic ) return false;
		if ( one->heuristic > two->heuristic ) return true;
		// still tied... prefer nodes 'in line' with goal ???
		// just distinguish them somehow...
		return one < two;
	}
};

// ========================================================
// class NodePool
//
// A Node Manager for A* like algorithms
// ========================================================
class NodePool {
	// =====================================================
	// struct DoubleMarkerArray
	//
	// A Marker Array supporting two mark types, open and closed.
	// =====================================================
	struct DoubleMarkerArray {
		int stride;
		unsigned int counter;
		unsigned int *marker;

		DoubleMarkerArray()		  { counter=1; marker=NULL; }
		~DoubleMarkerArray()	  { if (marker) delete [] marker; }
		void init( int w, int h ) { 
			stride = w; marker = new unsigned int[w*h]; 
			memset( marker, 0, w * h * sizeof(unsigned int) ); 
		}

		inline void newSearch() { counter += 2; }

		inline void setOpen( const Vec2i &pos )		{ marker[pos.y * stride + pos.x] = counter; }
		inline void setClosed( const Vec2i &pos )	{ marker[pos.y * stride + pos.x] = counter + 1; }

		inline bool isOpen( const Vec2i &pos )		{ return marker[pos.y * stride + pos.x] == counter; }
		inline bool isClosed( const Vec2i &pos )	{ return marker[pos.y * stride + pos.x] == counter + 1; }

		// deprecated ?
		inline bool isListed( const Vec2i &pos )	{ return marker[pos.y * stride + pos.x] >= counter; } 
		inline void setNeither( const Vec2i &pos )	{ marker[pos.y * stride + pos.x] = 0; }
	};

	// =====================================================
	// struct PointerArray
	//
	// An array of pointers
	// =====================================================
	struct PointerArray {
		int stride;
		AStarNode **pArray;

		void init ( int w, int h ) { 
			stride = w; 
			pArray = new AStarNode*[w*h]; 
			memset ( pArray, NULL, w * h * sizeof(AStarNode*) ); 
		}

		inline void  set ( const Vec2i &pos, AStarNode *ptr ) { pArray[pos.y * stride + pos.x] = ptr; }
		inline AStarNode* get ( const Vec2i &pos ) { return pArray[pos.y * stride + pos.x]; }
	};

public:
	NodePool ();
	virtual ~NodePool ();

	// NodeStorage template interface
	//
	//void reset();
	void setNodeLimit( int limit )	{ assert( limit > 0 ); setMaxNodes( limit ); }
	
	// same as old...
	//bool isOpen ( const Vec2i &pos )	{ return nodeMap[pos].mark == searchCounter; }
	//bool isClosed ( const Vec2i &pos )	{ return nodeMap[pos].mark == searchCounter + 1; }

	bool setOpen ( const Vec2i &pos, const Vec2i &prev, float h, float d ) {
		//assert( prev.x < 0 || isClosed( prev ) );
		return addToOpen( prev.x < 0 ? NULL : pointerArray.get( prev ), pos, h, d );
	}
	void updateOpen ( const Vec2i &pos, const Vec2i &prev, const float cost ) {
		//assert( isClosed( prev ) );
		updateOpenNode( pos, pointerArray.get( prev ), cost );
	}
	Vec2i getBestCandidate()	{ 
		AStarNode *ptr = getBestCandidateNode();
		return ptr ? ptr->pos : Vec2i(-1); 
	}
	Vec2i getBestSeen()			{return leastH->pos; }

	float getHeuristicAt( const Vec2i &pos )	{ return pointerArray.get( pos )->heuristic;	}
	float getCostTo( const Vec2i &pos )			{ return pointerArray.get( pos )->distToHere;	}
	float getEstimateFor( const Vec2i &pos )	{ return pointerArray.get( pos )->est();		}
	Vec2i getBestTo( const Vec2i &pos )			{ 
		AStarNode *ptr = pointerArray.get( pos );
		//assert ( ptr );
		return ptr->prev ? ptr->prev->pos : Vec2i(-1);	
	}


	// old algorithm interface...

	// sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
	void setMaxNodes ( const int max );
	// reset the node pool for a new search (resets tmpMaxNodes too)
	void reset ();
	bool limitReached () { return numNodes == tmpMaxNodes; }
	// create and add a new node to the open list
	bool addToOpen ( AStarNode *prev, const Vec2i &pos, float h, float d, bool exp = true );
	// add a node to the open list
	void addOpenNode ( AStarNode *node );
	// returns the node with the lowest heuristic (from open and closed)
	AStarNode* getBestHNode () { return leastH; }
	// test if a position is open
	bool isOpen ( const Vec2i &pos ) { return markerArray.isOpen ( pos ); }
	// conditionally update a node (known to be open)
	void updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost );
	// returns the 'best' node from open (and removes it from open, placing it in closed)
	AStarNode* getBestCandidateNode ();
	// test if a position is closed
	bool isClosed ( const Vec2i &pos ) { return markerArray.isClosed ( pos ); } 
	// needed for canPathOut()
	bool isListed ( const Vec2i &pos ) { return markerArray.isListed ( pos ); }

#if DEBUG_SEARCH_TEXTURES
	// interface to support debugging textures
	list<Vec2i>* getOpenNodes ();
	list<Vec2i>* getClosedNodes ();
	list<Vec2i> listedNodes;
#endif

private:
	AStarNode *leastH;
	AStarNode *stock;
	int numNodes;
	int tmpMaxNodes;

	DoubleMarkerArray markerArray;
	PointerArray pointerArray;
	vector<AStarNode*> openHeap;
};

}}}

#endif