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
// File: annotated_map.h
//
// Annotated Map, for use in pathfinding.
//

#ifndef _GLEST_GAME_ANNOTATED_MAP_H_
#define _GLEST_GAME_ANNOTATED_MAP_H_

#include "vec.h"
#include "unit_stats_base.h"
#include "astar_nodepool.h"
#include "map.h"
/*
#include <vector>
#include <list>
#include <set>
*/

typedef list<Vec2i>::iterator VLIt;
typedef list<Vec2i>::const_iterator VLConIt;
typedef list<Vec2i>::reverse_iterator VLRevIt;
typedef list<Vec2i>::const_reverse_iterator VLConRevIt;

using Shared::Platform::int64;

namespace Glest { namespace Game {

class Map;

namespace Search {

const Vec2i AboveLeftDist1[3] =  {
	Vec2i ( -1,  0 ),
	Vec2i ( -1, -1 ),
	Vec2i (  0, -1 )
};

const Vec2i AboveLeftDist2[5] = {
	Vec2i ( -2,  0 ),
	Vec2i ( -2, -1 ),
	Vec2i ( -2, -2 ),
	Vec2i ( -1, -2 ),
	Vec2i (  0, -2 )
};

// Adding a new field?
// add the new Field (enum defined in units_stats_base.h)
// add the 'xml' name to the Fields::names array (in units_stats_base.cpp)
// add a comment below, claiming the first unused metric field.
// add a case to the switches in CellMetrics::get() & CellMetrics::set()
// add code to PathFinder::computeClearance(), PathFinder::computeClearances() 
// and PathFinder::canClear() (if not handled fully in computeClearance[s]).
// finaly, add a little bit of code to Map::fieldsCompatable().

// Allows for a maximum moveable unit size of 15. we can 
// (with modifications) path groups in formation using this, 
// maybe this is not enough.. perhaps give some fields more resolution? 
// Will Glest even do size > 2 moveable units without serious movement related issues ???
struct CellMetrics {
	CellMetrics () { memset ( this, 0, sizeof(*this) ); }
	// can't get a reference to a bit field, so we can't overload 
	// the [] operator, and we have to get by with these...
	__inline uint32 get ( const Field field );
	__inline void set ( const Field field, uint32 val );
	__inline void setAll ( uint32 val );

	bool operator != ( CellMetrics &that ) {
		return memcmp ( this, &that, sizeof(*this) );
	}

private:
	uint32 field0 : 4; // In Use: FieldWalkable = land + shallow water 
	uint32 field1 : 4; // In Use: FieldAir = air
	uint32 field2 : 4; // In Use: FieldAnyWater = shallow + deep water
	uint32 field3 : 4; // In Use: FieldDeepWater = deep water
	uint32 field4 : 4; // In Use: FieldAmphibious = land + shallow + deep water 
	uint32 field5 : 4; // Unused: ?
	uint32 field6 : 4; // Unused: ?
	uint32 field7 : 4; // Unused: ?
};

// class MetricMap
// just a nice wrapper for the clearance metrics array
class MetricMap {
private:
	CellMetrics *metrics;
	int stride;

public:
	MetricMap () { stride = 0; metrics = NULL; }
	virtual ~MetricMap () { delete [] metrics; }

	void init ( int w, int h ) { 
		assert (w>0&&h>0); stride = w; metrics = new CellMetrics[w*h]; 
	}
	CellMetrics& operator [] ( const Vec2i &pos ) const { 
		return metrics[pos.y*stride+pos.x]; 
	}
};

// =====================================================
// class SearchParams
//
// Parameters for a single search
// =====================================================
struct SearchParams {
	Vec2i start, dest;
	Field field;
	int size, team;
	bool (*goalFunc)(const Vec2i&);
	SearchParams ( Unit *u );
	SearchParams ();
};

//#define _HEURISTIC_USE_TIE_BREAKER_

__inline float heuristic ( const Vec2i &p1, const Vec2i &p2 ) {
	int dx = abs (p1.x-p2.x), dy = abs (p1.y-p2.y);
	int diagonal = dx < dy ? dx : dy;
	int straight = dx + dy - 2 * diagonal;
#	ifdef _HEURISTIC_USE_TIE_BREAKER_
		return 1.4f * (float)diagonal + 1.f * (float)straight + p1.dist(p2) / 100.f;
#	else
		return 1.4 * diagonal + 1.0 * straight;
#	endif
}

// =====================================================
// class AnnotatedMap
//
// A compact representation of the map with clearance
// information for each cell, and home of the low level
// search function
// =====================================================
class AnnotatedMap {
	friend class AbstractMap;
public:
	AnnotatedMap ( Map *map );
	virtual ~AnnotatedMap ();

	// Initialise the Map Metrics
	void initMapMetrics ( Map *map );

	// Start a 'cascading update' of the Map Metrics from a position and size
	void updateMapMetrics ( const Vec2i &pos, const int size/*, bool adding, Field field */); 

	// Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
	__inline bool canOccupy ( const Vec2i &pos, int size, Field field ) const;

	static const int maxClearanceValue;

	// Temporarily annotate the map for nearby units
	void annotateLocal ( const Unit *unit, const Field field );

	// Clear temporary annotations
	void clearLocalAnnotations ( Field field );

	// The Classic A* [Hart, Nilsson, & Raphael, 1968]
	bool AStarSearch ( SearchParams &params, list<Vec2i> &path );

	// Perform A* search, but don't keep the path, just return it's length
	float AStarPathLength ( SearchParams &params );

	// fills d1 and d2 with the diagonal cells(coords) that need checking for a 
	// unit of size to move from s to d, for diagonal moves.
	// WARNING: ASSUMES ( s.x != d.x && s.y != d.y ) ie. the move IS diagonal
	// if this condition is not true, results are undefined
	static inline void getDiags ( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 );

	int getWidth () { return cMap->getW(); }
	int getHeight () { return cMap->getH(); }
#  ifdef _GAE_DEBUG_EDITION_
	list<std::pair<Vec2i,uint32>>* getLocalAnnotations ();
#  endif

private:
	// for initMetrics () and updateMapMetrics ()
	void computeClearances ( const Vec2i & );
	uint32 computeClearance ( const Vec2i &, Field );
	bool canClear ( const Vec2i &pos, int clear, Field field );

	// update to the left and above a area that may have changed metrics
	// pos: top-left of area changed
	// size: size of area changed
	// field: field to update for a local annotation, or FieldCount to update all fields
	void cascadingUpdate ( const Vec2i &pos, const int size, const Field field = FieldCount );
	void annotateUnit ( const Unit *unit, const Field field );

	// for annotateLocal ()
	//void localAnnotateCells ( const Vec2i &pos, const int size, const Field field, 
	//	const Vec2i *offsets, const int numOffsets );

	int metricHeight;
	std::map<Vec2i,uint32> localAnnt;
	Map *cMap;

	//void copyToPath ( const list<Vec2i> &pathList, list<Vec2i> &path );

	AStarNodePool *aNodePool;
	bool assertValidPath ( list<Vec2i> &path );

#ifdef _GAE_DEBUG_EDITION_
public:
#endif
	MetricMap metrics;
};

}}}

#endif
