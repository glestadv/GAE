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
// File: pf_datastructs.h
//
// Support structures for low level search
//

#ifndef _GLEST_GAME_PATHFINDER_DATASTRUCTS_H_
#define _GLEST_GAME_PATHFINDER_DATASTRUCTS_H_

#include "vec.h"

using Shared::Graphics::Vec2i;

// Marker Arrays are simply an array, a 'perfect' hashtable for rectangular maps.
//
// The default marker array uses a lazy clearing scheme (ie, the array is never
// cleared) and a single comparison to test a position's 'listedness', being an array
// 'look-ups' and 'inserts' operate in 'constant time' [O(1)]. Fastest, but requires 4 
// (or 8) bytes per map cell.
//
// The packed marker array uses a single bit per map cell, and therefore needs to be 
// cleared before use, and lookups require some bit masking. 'Look-ups' and 'inserts' are
// still constant time ops [O(1)], but is obviously slightly slower than the unpacked version.
// NB: If the map is very large this will exhibit better caching characteristics
// than the lazy cleared version, and will probably out-perform it.

namespace Glest { namespace Game { namespace PathFinder {

// =====================================================
// struct PosMarkerArray
//
// A Marker Array, using sizeof(unsigned int) bytes per 
// cell, and a lazy clearing scheme.
// =====================================================
struct PosMarkerArray
{
   int stride;
   unsigned int counter;
   unsigned int *marker;
   
   PosMarkerArray () {counter=0;marker=NULL;};
   ~PosMarkerArray () {if (marker) delete marker; }

   void init ( int w, int h ) { stride = w; marker = new unsigned int[w*h]; 
            memset ( marker, 0, w * h * sizeof(unsigned int) ); }
   inline void newSearch () { ++counter; }
   inline void setMark ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = counter; }
   inline bool isMarked ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] == counter; }
};

// =====================================================
// struct DoubleMarkerArray
//
// A Marker Array supporting two mark types, open and closed.
// =====================================================
struct DoubleMarkerArray
{
   int stride;
   unsigned int counter;
   unsigned int *marker;
   
   DoubleMarkerArray () {counter=1;marker=NULL;};
   ~DoubleMarkerArray () {if (marker) delete marker; }

   void init ( int w, int h ) { stride = w; marker = new unsigned int[w*h]; 
            memset ( marker, 0, w * h * sizeof(unsigned int) ); }
   inline void newSearch () { counter += 2; }

   inline void setNeither ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = 0; }
   inline void setOpen ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = counter; }
   inline void setClosed ( const Vec2i &pos ) { marker[pos.y * stride + pos.x] = counter + 1; }
   
   inline bool isOpen ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] == counter; }
   inline bool isClosed ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] == counter + 1; }
   inline bool isListed ( const Vec2i &pos ) { return marker[pos.y * stride + pos.x] >= counter; }
   
};

// =====================================================
// struct PackedPosMarkerArray
//
// A Packed Marker Array, using 1 bit per cell, must be 
// cleared before use, and involves some bit masking.
// =====================================================
struct PackedPosMarkerArray
{
   // TODO: Fill me in!
};

// =====================================================
// struct PointerArray
//
// An array of pointers
// =====================================================
struct PointerArray
{
   int stride;
   void **pArray;

   void init ( int w, int h ) { stride = w; pArray = new void*[w*h]; 
            memset ( pArray, NULL, w * h * sizeof(void*) ); }

   inline void  set ( const Vec2i &pos, void *ptr ) { pArray[pos.y * stride + pos.x] = ptr; }
   inline void* get ( const Vec2i &pos ) { return pArray[pos.y * stride + pos.x]; }
};

}}}

#endif
