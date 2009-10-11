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

#ifndef _GLEST_GAME_PATHFINDER_NODE_POOL_H_
#define _GLEST_GAME_PATHFINDER_NODE_POOL_H_

#include "vec.h"
#include "timer.h"
#include "unit_stats_base.h"

using Shared::Platform::Chrono;

namespace Glest { namespace Game { 

class Unit;
class Map;

namespace Search {
// ========================================================
// class NodePool
// ========================================================
/** A NodeStorage class (template interface) for A* */
class NodePool {
	// =====================================================
	// struct AStarNode
	// =====================================================
	/** A node structure for A* with NodePool */
	struct AStarNode {
		/** position this node represents */
		Vec2i pos;			// 8 bytes
		/** Node of best path to here */
		AStarNode *prev;	// 4|8 bytes
		/** estimate of distance to goal */
		float heuristic;
		/** cost to this node */
		float distToHere;
		bool exploredCell; // <-- vis status bound for AnnotatedMap, then remove this
		/** estimate, costToHere + heuristic */
		float est() const { return distToHere + heuristic; }
	}; // remove explored then sizeof()==20[|24]
	// =====================================================
	// class AStarComp
	// =====================================================
	/** Comparison function for the open heap */
	class AStarComp {
	public:
		/** Comparison function
		  * @param one AStarNode for comparison
		  * @param two AStarNode for comparison
		  * @return  true if two is 'better' than one
		  */
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
	// =====================================================
	// struct DoubleMarkerArray
	// =====================================================
	/** A Marker Array supporting two mark types, open and closed. */
	struct DoubleMarkerArray {
		/** stride of array */
		int stride;
		/** the counter */
		unsigned int counter;
		/** the array */
		unsigned int *marker;

		DoubleMarkerArray()		  { counter=1; marker=NULL; }
		~DoubleMarkerArray()	  { if (marker) delete [] marker; }
		/** Initialise the marker array */
		void init( int w, int h ) { 
			stride = w; marker = new unsigned int[w*h]; 
			memset( marker, 0, w * h * sizeof(unsigned int) ); 
		}
		/** reset for a new search */
		inline void newSearch() { counter += 2; }

		/** mark a cell open */
		inline void setOpen( const Vec2i &pos )		{ marker[pos.y * stride + pos.x] = counter; }
		/** mark a cell closed */
		inline void setClosed( const Vec2i &pos )	{ marker[pos.y * stride + pos.x] = counter + 1; }
		/** is a cell open? */
		inline bool isOpen( const Vec2i &pos )		{ return marker[pos.y * stride + pos.x] == counter; }
		/** is a cell closed? */
		inline bool isClosed( const Vec2i &pos )	{ return marker[pos.y * stride + pos.x] == counter + 1; }

		// deprecated ?
		/** @deprecated not needed? */
		inline bool isListed( const Vec2i &pos )	{ return marker[pos.y * stride + pos.x] >= counter; } 
		/** @deprecated not needed? */
		inline void setNeither( const Vec2i &pos )	{ marker[pos.y * stride + pos.x] = 0; }
	};

	// =====================================================
	// struct PointerArray
	// =====================================================
	/** An array of pointers
	  * <p>Must be used in conjunction with marker array, constantly contains junk 
	  * values, use only if mark > counter</p> */
	struct PointerArray {
		/** stride of array */
		int stride;
		/** the array */
		AStarNode **pArray;

		void init ( int w, int h ) { 
			stride = w; 
			pArray = new AStarNode*[w*h]; 
			memset ( pArray, NULL, w * h * sizeof(AStarNode*) ); 
		}
		/** set the pointer for pos to ptr */
		inline void  set ( const Vec2i &pos, AStarNode *ptr ) { pArray[pos.y * stride + pos.x] = ptr; }
		/** get the pointer for pos */
		inline AStarNode* get ( const Vec2i &pos ) { return pArray[pos.y * stride + pos.x]; }
	};

public:
	NodePool ();
	virtual ~NodePool ();

	// NodeStorage template interface
	//
	// same as old...
	//void reset();
	//bool isOpen ( const Vec2i &pos )	{ return nodeMap[pos].mark == searchCounter; }
	//bool isClosed ( const Vec2i &pos )	{ return nodeMap[pos].mark == searchCounter + 1; }

	/** marks an unvisited position as open
	  * @param pos the position to open
	  * @param prev the best known path to pos is from
	  * @param h the heuristic for pos
	  * @param d the costSoFar for pos
	  * @return true if added, false if node limit reached
	  */
	bool setOpen ( const Vec2i &pos, const Vec2i &prev, float h, float d ) {
		//assert( prev.x < 0 || isClosed( prev ) );
		return addToOpen( prev.x < 0 ? NULL : pointerArray.get( prev ), pos, h, d );
	}
	/** conditionally update a node on the open list. Tests if a path through a new nieghbour
	  * is better than the existing known best path to pos, updates if so.
	  * @param pos the open postion to test
	  * @param prev the new path from
	  * @param d the distance to here through prev
	  */
	void updateOpen ( const Vec2i &pos, const Vec2i &prev, const float cost ) {
		//assert( isClosed( prev ) );
		updateOpenNode( pos, pointerArray.get( prev ), cost );
	}
	/** get the best candidate from the open list, and close it.
	  * @return the lowest estimate node from the open list, or -1,-1 if open list empty
	  */
	Vec2i getBestCandidate()	{ 
		AStarNode *ptr = getBestCandidateNode();
		return ptr ? ptr->pos : Vec2i(-1); 
	}
	/** get the best heuristic node seen this search */
	Vec2i getBestSeen()			{return leastH->pos; }
	/** get the heuristic of the node at pos [known to be visited] */
	float getHeuristicAt( const Vec2i &pos )	{ return pointerArray.get( pos )->heuristic;	}
	/** get the cost to the node at pos [known to be visited] */
	float getCostTo( const Vec2i &pos )			{ return pointerArray.get( pos )->distToHere;	}
	/** get the estimate for the node at pos [known to be visited] */
	float getEstimateFor( const Vec2i &pos )	{ return pointerArray.get( pos )->est();		}
	/** get the best path to the node at pos [known to be visited] */
	Vec2i getBestTo( const Vec2i &pos )			{ 
		AStarNode *ptr = pointerArray.get( pos );
		//assert ( ptr );
		return ptr->prev ? ptr->prev->pos : Vec2i(-1);	
	}


	// old algorithm interface...

	/** set a maximum number of nodes to expand */
	void setMaxNodes ( const int max );
	/** reset the node pool for a new search (resets tmpMaxNodes too) */
	void reset ();
	/** @deprecated ? do not use ? */
	bool limitReached () { return numNodes == tmpMaxNodes; }
	/** @deprecated use setOpen() */
	bool addToOpen ( AStarNode *prev, const Vec2i &pos, float h, float d, bool exp = true );
	/** @deprecated do not use*/
	void addOpenNode ( AStarNode *node );
	/** @deprecated use getBestSeen() */
	AStarNode* getBestHNode () { return leastH; }
	/** test if a position is open */
	bool isOpen ( const Vec2i &pos ) { return markerArray.isOpen ( pos ); }
	/** @deprecated use updateOpen() */
	void updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost );
	/** @deprecated use getBestCandidate() */
	AStarNode* getBestCandidateNode ();
	/** test if a position is closed */
	bool isClosed ( const Vec2i &pos ) { return markerArray.isClosed ( pos ); } 
	/** @deprecated needed for canPathOut() */
	bool isListed ( const Vec2i &pos ) { return markerArray.isListed ( pos ); }

#if DEBUG_SEARCH_TEXTURES
	// interface to support debugging textures
	list<Vec2i>* getOpenNodes ();
	list<Vec2i>* getClosedNodes ();
	list<Vec2i> listedNodes;
#endif

private:
	/** The 'best' node seen so far this search */
	AStarNode *leastH;
	/** The stock of nodes */
	AStarNode *stock;
	/** number of nodes used so far this search */
	int numNodes;
	/** a temporary maximum number of nodes to use */
	int tmpMaxNodes;

	/** A double marker array the size of the map, indicating node status (unvisited, open, closed) */
	DoubleMarkerArray markerArray;
	/** An array the size of the map, containing pointers to Nodes, valid is position marked */
	PointerArray pointerArray;
	/** the open list, binary heap, maintained with std algorithms */
	vector<AStarNode*> openHeap;
};

}}}

#endif