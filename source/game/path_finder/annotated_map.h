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
/*
#include <vector>
#include <list>
#include <set>
*/

namespace Game {

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

// Allows for a maximum moveable unit size of 3. we can
// (with modifications) path groups in formation using this,
// maybe this is not enough.. perhaps give some fields more resolution?
// Will Glest even do size > 2 moveable units without serious movement related issues ???
struct CellMetrics {
	CellMetrics () { memset ( this, 0, sizeof(*this) ); }
	// can't get a reference to a bit field, so we can't overload
	// the [] operator, and we have to get by with these...

	__inline uint32 get ( const Field );
	__inline void   set ( const Field, uint32 val );
	__inline void   setAll ( uint32 val );

	bool operator != ( CellMetrics &that ) {
		return memcmp ( this, &that, sizeof(*this) );
	}

	//bool isNearResource ( const Vec2i &pos );
	//bool isNearStore ( const Vec2i &pos );

private:
	uint32 field0 : 3; // In Use: FieldWalkable = land + shallow water
	uint32 field1 : 3; // In Use: FieldAir = air
	uint32 field2 : 3; // In Use: FieldAnyWater = shallow + deep water
	uint32 field3 : 3; // In Use: FieldDeepWater = deep water
	uint32 field4 : 3; // In Use: FieldAmphibious = land + shallow + deep water
	uint32 pad    : 1;
};

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

class AnnotatedMap {
public:
	AnnotatedMap ( Map *map );
	virtual ~AnnotatedMap ();

	// Initialise the Map Metrics
	void initMapMetrics ( Map *map );

	// Start a 'cascading update' of the Map Metrics from a position and size
	void updateMapMetrics ( const Vec2i &pos, const int size );

	// Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
	__inline bool canOccupy ( const Vec2i &pos, int size, Field field ) const {
		return metrics[pos].get ( field ) >= size ? true : false;
	}

	static const int maxClearanceValue;

	// Temporarily annotate the map for nearby units
	void annotateLocal ( const Unit *unit, const Field field );

	// Clear temporary annotations
	void clearLocalAnnotations ( Field field );

#  ifdef _GAE_DEBUG_EDITION_
	list<std::pair<Vec2i,uint32>>* getLocalAnnotations ();
#  endif

private:
	// for initMetrics () and updateMapMetrics ()
	void computeClearances ( const Vec2i & );
	void computeClearance ( const Vec2i &, Field );
	bool canClear ( const Vec2i &pos, int clear, Field field );

	// perform local annotations, mkae unit obstactle in field
	void annotateUnit ( const Unit *unit, const Field field );

	// update to the left and above a area that may have changed metrics
	// pos: top-left of area changed
	// size: size of area changed
	// field: field to update for a local annotation, or FieldCount to update all fields
	void cascadingUpdate ( const Vec2i &pos, const int size, const Field field = FieldCount );

	int metricHeight;
	std::map<Vec2i,uint32> localAnnt;
	Map *cMap;
#ifdef _GAE_DEBUG_EDITION_
public:
#endif
	MetricMap metrics;
};

inline uint32 CellMetrics::get ( const Field field ) {
	switch ( field ) {
		case FieldWalkable: return field0;
		case FieldAir: return field1;
		case FieldAnyWater: return field2;
		case FieldDeepWater: return field3;
		case FieldAmphibious: return field4;
		default: throw runtime_error ( "Unknown Field passed to CellMetrics::get()" );
	}
	return 0;
}

inline void CellMetrics::set ( const Field field, uint32 val ) {
	assert ( val <= AnnotatedMap::maxClearanceValue );
	switch ( field ) {
		case FieldWalkable: field0 = val; return;
		case FieldAir: field1 = val; return;
		case FieldAnyWater: field2 = val; return;
		case FieldDeepWater: field3 = val; return;
		case FieldAmphibious: field4 = val; return;
		default: throw runtime_error ( "Unknown Field passed to CellMetrics::set()" );
	}

}

inline void CellMetrics::setAll ( uint32 val ) {
	assert ( val <= AnnotatedMap::maxClearanceValue );
	field0 = field1 = field2 = field3 = field4 = val;
}

}}

#endif
