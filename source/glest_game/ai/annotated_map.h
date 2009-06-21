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

#include <vector>
#include <list>
#include <set>

namespace Glest { namespace Game {

class Map;

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
struct CellMetrics
{
   CellMetrics () { memset ( this, 0, sizeof(this) ); }
   // can't get a reference to a bit field, so we can't overload 
   // the [] operator, and we have to get by with these...
   inline uint32 get ( const Field );
   inline void   set ( const Field, uint32 val );

private:
   uint32 field0 : 2; // In Use: mfWalkable = land + shallow water 
   uint32 field1 : 2; // In Use: mfAir = air
   uint32 field2 : 2; // In Use: mfAnyWater = shallow + deep water
   uint32 field3 : 2; // In Use: mfDeepWater = deep water
   uint32 field4 : 2; // In Use: mfAmphibious = land + shallow + deep water 
   uint32 field5 : 2; // Unused: ?
   uint32 field6 : 2; // Unused: ?
   uint32 field7 : 2; // Unused: ?
   uint32 field8 : 2; // Unused: ?
   uint32 field9 : 2; // Unused: ?
   uint32 fielda : 2; // Unused: ?
   uint32 fieldb : 2; // Unused: ?
   uint32 fieldc : 2; // Unused: ?
   uint32 fieldd : 2; // Unused: ?
   uint32 fielde : 2; // Unused: ?
   uint32 fieldf : 2; // Unused: ?
};

class AnnotatedMap
{
public:
   AnnotatedMap ( Map *map );
   virtual ~AnnotatedMap ();

   // Initialise the Map Metrics
   void initMapMetrics ( Map *map );

   // Start a 'cascading update' of the Map Metrics from a position and size
   void updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field ); 

   // Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
   bool canOccupy ( const Vec2i &pos, int size, Field field ) const;

   static const int maxClearanceValue;

private:
   // for initMetrics () and updateMapMetrics ()
   CellMetrics computeClearances ( const Vec2i & );
   uint32 computeClearance ( const Vec2i &, Field );
   bool canClear ( const Vec2i &pos, int clear, Field field );

   int metricHeight;

   Map *map;
#if defined(PATHFINDER_TIMING) || defined(PATHFINDER_DEBUG_TEXTURES )
public:
#endif
   CellMetrics **metrics; // clearance values [y][x]
};

}}

#endif
