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
// File: pf_np_astar_dia.h
//

#ifndef _GLEST_GAME_ASTAR_NODE_POOL_DIA_H_
#define _GLEST_GAME_ASTAR_NODE_POOL_DIA_H_

#include "vec.h"
#include <vector>
#include <queue>
#include <map>

#include "pf_datastructs.h"
#include "pf_nodepool.h"

#ifdef PATHFINDER_DEBUG_TEXTURES
#include <list>
#endif
#ifdef PATHFINDER_TIMING
#include "timer.h"

using Shared::Platform::Chrono;
#endif

using Shared::Graphics::Vec2i;
using namespace std;

namespace Glest { namespace Game { 

class Map;

namespace PathFinder {

// =====================================================
// class AStarNodePool using Dual-Indexed-Array
//
// =====================================================
class AStarNodePoolDIA  : public AStarNodePool
{
   DoubleMarkerArray markerArray;
   AStarNode **lists;

   int numOpen, numClosed;
   int maxNodes, tmpMaxNodes;

public:
   AStarNodePoolDIA ();
   ~AStarNodePoolDIA ();
   void init ( Map *map );

   // reset everything, include maxNodes...
   void reset ();
   // Is this pos already listed?
   bool isListed ( const Vec2i &pos ) { return markerArray.isListed ( pos ); }
   bool isClosed ( const Vec2i &pos ) { return markerArray.isClosed ( pos ); }
   bool isOpen ( const Vec2i &pos ) { return markerArray.isOpen ( pos ); }
   void updateOpenNode ( const Vec2i &pos, AStarNode *curr, float cost );

   void addOpenNode ( AStarNode *node );

   // moves 'best' node from open to closed, and returns it, or NULL if open is empty
   AStarNode* getBestCandidate ();

#ifdef PATHFINDER_DEBUG_TEXTURES
   list<Vec2i>* AStarNodePoolDIA::getOpenNodes ();
   list<Vec2i>* AStarNodePoolDIA::getClosedNodes ();
#endif
};

}}}

#endif