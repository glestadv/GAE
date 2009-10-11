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

// =====================================================
// struct CellMetrics
// =====================================================
/** Stores clearance metrics for a cell.
  * 3 bits are used per field, allowing for a maximum moveable unit size of 7.
  * The left over bit is used for per team 'foggy' maps, the dirty bit is set when
  * an obstacle has been placed or removed in an area the team cannot currently see.
  * The team's annotated map is thus not updated until that cell becomes visible again.
  */
struct CellMetrics {
	CellMetrics() { memset ( this, 0, sizeof(*this) ); }
	
	uint16 get(const Field field) {
		switch ( field ) {
			case   FieldWalkable: return field0;
			case		FieldAir: return field1;
			case   FieldAnyWater: return field2;
			case  FieldDeepWater: return field3;
			case FieldAmphibious: return field4;
			default: throw runtime_error ( "Unknown Field passed to CellMetrics::get()" );
		}
	}
	void set(const Field field, uint16 val) {
		switch ( field ) {
			case   FieldWalkable: field0 = val; return;
			case		FieldAir: field1 = val; return;
			case   FieldAnyWater: field2 = val; return;
			case  FieldDeepWater: field3 = val; return;
			case FieldAmphibious: field4 = val; return;
			default: throw runtime_error ( "Unknown Field passed to CellMetrics::set()" );
		}
	}	
	void setAll(uint16 val)				{ field0 = field1 = field2 = field3 = field4 = val; }
	bool operator!=(CellMetrics &that)	{ return memcmp( this, &that, sizeof(*this) ); }
	bool isDirty() const				{ return dirty; }
	void setDirty(const bool val)		{ dirty = val; }

private:
	uint16 field0 : 3; // In Use: FieldWalkable = land + shallow water 
	uint16 field1 : 3; // In Use: FieldAir = air
	uint16 field2 : 3; // In Use: FieldAnyWater = shallow + deep water
	uint16 field3 : 3; // In Use: FieldDeepWater = deep water
	uint16 field4 : 3; // In Use: FieldAmphibious = land + shallow + deep water 

	uint16  dirty : 1; // used in 'team' maps as a 'dirty bit' (clearances have changed
					   // but team hasn't seen that change yet).
};

// =====================================================
// class MetricMap
// =====================================================
/** A wrapper class for the array of CellMetrics
  */
class MetricMap {
private:
	CellMetrics *metrics;
	int width,height;

public:
	MetricMap() : width(0), height(0), metrics(NULL) { }
	~MetricMap ()			{ delete [] metrics; }
	void init(int w, int h) { assert ( w > 0 && h > 0); width = w; height = h; metrics = new CellMetrics[w * h]; }
	void zero()				{ memset(metrics, 0, sizeof(CellMetrics) * width * height); }
	
	CellMetrics& operator [] ( const Vec2i &pos ) const { return metrics[pos.y * width + pos.x]; }
};

// =====================================================
// class AnnotatedMap
// =====================================================
/** <p>A compact representation of the map with clearance information for each cell.
  * The clearance values are stored for each cell & field, and represent the 
  * clearance to the south and east of the cell. That is, the maximum size unit 
  * that can be 'positioned' in this cell (with units in Glest always using the 
  * north-west most cell they occupy as their 'position').</p>
  */
//TODO: pretty pictures for the doco...
class AnnotatedMap {
	friend class AbstractMap;
#	if DEBUG_SEARCH_TEXTURES
		list<std::pair<Vec2i,uint32>>* getLocalAnnotations();
		friend class Renderer; // DebugRenderer ?
#	endif

public:
	AnnotatedMap(bool master=true);
	virtual ~AnnotatedMap();

	static const int maxClearanceValue = 7;

	void initMapMetrics();
	void updateMapMetrics( const Vec2i &pos, const int size ); 

	// Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
	bool canOccupy( const Vec2i &pos, int size, Field field ) const {
		assert( theMap.isInside( pos ) );
		return metrics[pos].get( field ) >= size ? true : false;
	}

	bool isDirty( const Vec2i &pos ) const				{ metrics[pos].isDirty(); }
	void setDirty( const Vec2i &pos, const bool val )	{ metrics[pos].setDirty( val ); }

	void annotateLocal( const Unit *unit, const Field field );
	void clearLocalAnnotations( Field field );

private:
	// for initMetrics() and updateMapMetrics ()
	void computeClearances( const Vec2i & );
	uint32 computeClearance( const Vec2i &, Field );

	void cascadingUpdate( const Vec2i &pos, const int size, const Field field = FieldCount );
	void annotateUnit( const Unit *unit, const Field field );

	std::map<Vec2i,uint32> localAnnt;
	MetricMap metrics;
};

}}}

#endif
