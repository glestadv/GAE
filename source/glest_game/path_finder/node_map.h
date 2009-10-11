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


#ifndef _GLEST_GAME_ASTAR_NODE_MAP_H_
#define _GLEST_GAME_ASTAR_NODE_MAP_H_

#include "vec.h"
#include "world.h"
#include "game_constants.h"

using Shared::Graphics::Vec2i;
using Shared::Platform::uint16;
using Shared::Platform::uint32;

namespace Glest { namespace Game {
class Map;
namespace Search {

#define MAX_MAP_COORD_BITS 16
#define MAX_MAP_COORD ((1<<MAX_MAP_COORD_BITS)-1)

#pragma pack(1)

struct PackedPos {
	PackedPos() : x(0), y(0) {}
	PackedPos( int x, int y ) : x(x), y(y) {} 
	PackedPos( Vec2i pos ) { *this = pos; }
	PackedPos& operator=( Vec2i pos ) { 
		assert( pos.x <= MAX_MAP_COORD && pos.y <= MAX_MAP_COORD ); 
		if ( pos.x < 0 || pos.y < 0 ) {
			x = MAX_MAP_COORD; y = MAX_MAP_COORD; // invalid
		} else {
			x = pos.x; 
			y = pos.y; 
		}
		return *this; 
	}
	operator Vec2i() { return Vec2i( x, y ); }
	bool operator==( PackedPos &that ) { return x == that.x && y == that.y; }
	bool valid() { return !(( x == MAX_MAP_COORD ) && ( y == MAX_MAP_COORD )); }

	// max == MAX_MAP_COORD,
	// MAX_MAP_COORD,MAX_MAP_COORD is considered 'invalid', this is ok still on a 
	// MAX_MAP_COORD*MAX_MAP_COORD map in Glest, because the far east and south 'tiles' 
	// (2 cells) are not valid either.
	uint16 x : MAX_MAP_COORD_BITS;
	uint16 y : MAX_MAP_COORD_BITS;
};

struct NodeMapCell {
	// node status for this search, 
	// mark <  NodeMap::searchCounter     => unvisited
	// mark == NodeMap::searchCounter     => open
	// mark == NodeMap::searchCounter + 1 => closed
	uint32 mark;

	PackedPos prevNode; // best route to here is from, valid only if this node is closed
	PackedPos nextOpen; // 'next' node in open list, valid only if this node is open
	float heuristic;
	float distToHere;

	NodeMapCell ()	{ memset( this, 0, sizeof(*this) ); }

	float estimate ()	{ return heuristic + distToHere; }
};

#pragma pack()

class NodeMapCellArray {
private:
	NodeMapCell *array;
	int stride;
public:
	NodeMapCellArray()	{ array = new NodeMapCell[theMap.getW() * theMap.getH()]; stride = theMap.getW(); }
	~NodeMapCellArray() { delete [] array; }

	NodeMapCell& operator[] ( const Vec2i &pos )		{ return array[pos.y * stride + pos.x]; }
	NodeMapCell& operator[] ( const PackedPos pos )	{ return array[pos.y * stride + pos.x]; }
};

class NodeMap {
public:
	NodeMap();

	// NodeStorage template interface
	//
	void reset();
	void setMaxNodes( int limit )	{ nodeLimit = limit > 0 ? limit : -1; }
	
	bool isOpen ( const Vec2i &pos )	{ return nodeMap[pos].mark == searchCounter; }
	bool isClosed ( const Vec2i &pos )	{ return nodeMap[pos].mark == searchCounter + 1; }

	bool setOpen ( const Vec2i &pos, const Vec2i &prev, float h, float d );
	void updateOpen ( const Vec2i &pos, const Vec2i &prev, const float cost );
	Vec2i getBestCandidate();
	Vec2i getBestSeen()		{ return bestH.valid() ? bestH : Vec2i(-1); }

	float getHeuristicAt( const Vec2i &pos )	{ return nodeMap[pos].heuristic;	}
	float getCostTo( const Vec2i &pos )			{ return nodeMap[pos].distToHere;	}
	float getEstimateFor( const Vec2i &pos )	{ return nodeMap[pos].estimate();	}
	Vec2i getBestTo( const Vec2i &pos )			{ return nodeMap[pos].prevNode;		}

private:
	NodeMapCellArray nodeMap;
	int stride;
	int nodeLimit;
	uint32 searchCounter, nodeCount;
	bool assertOpen ();
	PackedPos invalidPos;
	PackedPos bestH;
	Vec2i openTop;

	// debug
	bool assertValidPath ( list<Vec2i> &path );
	void logOpen ();

#ifdef DEBUG_PATHFINDER_TEXTURES
	virtual list<Vec2i>* getOpenNodes ();
	virtual list<Vec2i>* getClosedNodes ();
	list<Vec2i> listedNodes;
#endif

};

}}}

#endif