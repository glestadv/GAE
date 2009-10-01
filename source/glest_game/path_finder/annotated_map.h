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
#include "world.h"

typedef list<Vec2i>::iterator VLIt;
typedef list<Vec2i>::const_iterator VLConIt;
typedef list<Vec2i>::reverse_iterator VLRevIt;
typedef list<Vec2i>::const_reverse_iterator VLConRevIt;

using Shared::Platform::int64;

namespace Glest { namespace Game {
class Map;
namespace Search {

struct CellMetrics {
	CellMetrics() { memset ( this, 0, sizeof(*this) ); }
	
	uint16 get( const Field field ) {
		switch ( field ) {
			case FieldWalkable: return field0;
			case FieldAir: return field1;
			case FieldAnyWater: return field2;
			case FieldDeepWater: return field3;
			case FieldAmphibious: return field4;
			default: throw runtime_error ( "Unknown Field passed to CellMetrics::get()" );
		}
	}
	
	void set( const Field field, uint16 val ) {
		switch ( field ) {
			case FieldWalkable: field0 = val; return;
			case FieldAir: field1 = val; return;
			case FieldAnyWater: field2 = val; return;
			case FieldDeepWater: field3 = val; return;
			case FieldAmphibious: field4 = val; return;
			default: throw runtime_error ( "Unknown Field passed to CellMetrics::set()" );
		}
	}
	
	void setAll( uint16 val ) {
		field0 = field1 = field2 = field3 = field4 = val;
	}

	bool operator!=( CellMetrics &that ) {
		return memcmp( this, &that, sizeof(*this) );
	}

private:
	uint16 field0 : 3; // In Use: FieldWalkable = land + shallow water 
	uint16 field1 : 3; // In Use: FieldAir = air
	uint16 field2 : 3; // In Use: FieldAnyWater = shallow + deep water
	uint16 field3 : 3; // In Use: FieldDeepWater = deep water
	uint16 field4 : 3; // In Use: FieldAmphibious = land + shallow + deep water 

	uint16 _pad_bit : 1;
};

// class MetricMap
// just a nice wrapper for the clearance metrics array
class MetricMap {
private:
	CellMetrics *metrics;
	int stride;

public:
	MetricMap() { stride = 0; metrics = NULL; }
	virtual ~MetricMap () { delete [] metrics; }

	void init( int w, int h ) { 
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
/*struct SearchParams {
	Vec2i start, dest;
	Field field;
	int size, team, player;

	SearchParams( Unit *u );
	SearchParams();
};*/

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
	AnnotatedMap();
	virtual ~AnnotatedMap();

	static const int maxClearanceValue = 7;

	// Initialise the Map Metrics
	void initMapMetrics();

	// Start a 'cascading update' of the Map Metrics from a position and size
	void updateMapMetrics( const Vec2i &pos, const int size ); 

	// Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
	bool canOccupy( const Vec2i &pos, int size, Field field ) const {
		assert( theMap.isInside( pos ) );
		return metrics[pos].get( field ) >= size ? true : false;
	}
	// Temporarily annotate the map for nearby units
	void annotateLocal( const Unit *unit, const Field field );

	// Clear temporary annotations
	void clearLocalAnnotations( Field field );

#  ifdef DEBUG_SEARCH_TEXTURES
	list<std::pair<Vec2i,uint32>>* getLocalAnnotations();
#  endif

private:
	// for initMetrics() and updateMapMetrics ()
	void computeClearances( const Vec2i & );
	uint32 computeClearance( const Vec2i &, Field );
	bool canClear( const Vec2i &pos, int clear, Field field );

	// update to the left and above a area that may have changed metrics
	// pos: top-left of area changed
	// size: size of area changed
	// field: field to update for a local annotation, or FieldCount to update all fields
	void cascadingUpdate( const Vec2i &pos, const int size, const Field field = FieldCount );

	void annotateUnit( const Unit *unit, const Field field );

	int metricHeight;
	std::map<Vec2i,uint32> localAnnt;

#ifdef DEBUG_SEARCH_TEXTURES
public:
#endif
	MetricMap metrics;
};

}}}

#endif
