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
// File: pf_nodepool.h
//

#ifndef _GLEST_GAME_PATHFINDER_NODEPOOL_BASE_H_
#define _GLEST_GAME_PATHFINDER_NODEPOOL_BASE_H_

#define LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC

#include "vec.h"
#include "timer.h"
#include "unit_stats_base.h"

using Shared::Platform::Chrono;

namespace Glest { namespace Game { 

class Unit;
class Map;

namespace PathFinder {

// =====================================================
// class SearchParams
//
// Parameters for a single search
// =====================================================
struct SearchParams
{
   Vec2i start, dest;
   Field field;
   int size, team;
   SearchParams ( Unit *u );
};

// =====================================================
// struct BFSNode
//
// A Best First Search Node
// =====================================================
struct BFSNode
{
	Vec2i pos;
	BFSNode *next;
	BFSNode *prev;
   float heuristic;//union { float f; int i; } heuristic;
	bool exploredCell;
}; // 21 bytes == 24 in mem ?

// =====================================================
// struct AStarNode
//
// A node structure for A* and friends
// =====================================================
struct AStarNode
{
	Vec2i pos;
	AStarNode *next;
	AStarNode *prev;
   float heuristic;
   float distToHere;
	bool exploredCell;
   float est () const { return distToHere + heuristic; }
}; // 25 bytes == 28 in mem ?

#ifdef PATHFINDER_TIMING
struct PathFinderStats
{      
   int64 num_searches;
   double search_avg;
   int64 num_searches_last_interval;
   double search_avg_last_interval;
   int64 num_searches_this_interval;
   double search_avg_this_interval;

   int64 worst_search;
   int64 calls_rejected;

   PathFinderStats ( char * name );
   void resetCounters ();
   void AddEntry ( int64 ticks );
   void IncReject () { calls_rejected++; }
   char* GetTotalStats ();
   char* GetStats ();
   char buffer[512];
   char prefix[32];
};
#endif

// =====================================================
// class AStarNodePool
//
// An abstract base class for A* node pools.
// =====================================================
class AStarNodePool
{
public:
   AStarNodePool ();
   virtual ~AStarNodePool ();

   virtual void init ( Map *map ) = 0;
   
   // sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
   void setMaxNodes ( const int max );

   // Always call AStarNodePool::reset() if you override this.
   virtual void reset ();

   // DO NOT OVERRIDE, override addOpenNode ()
   bool addToOpen ( AStarNode* prev, const Vec2i &pos, float h, float d, bool exp = true );
   
   AStarNode* getBestHNode ();

   virtual bool isOpen ( const Vec2i &pos ) = 0;
   virtual void updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost ) = 0;
   virtual AStarNode* getBestCandidate () = 0;

   virtual bool isClosed ( const Vec2i &pos ) = 0;

#ifndef LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC
   virtual void updateClosedNode ( const Vec2i &pos, AStarNode *neighbour, float cost ) = 0;
#endif

#ifdef PATHFINDER_DEBUG_TEXTURES
   virtual list<Vec2i>* getOpenNodes () = 0;
   virtual list<Vec2i>* getClosedNodes () = 0;
#endif
#ifdef PATHFINDER_TIMING
   int64 startTime;
   virtual void startTimer () { startTime = Chrono::getCurMicros (); }
   virtual int64 stopTimer () { return Chrono::getCurMicros () - startTime; }
#endif

protected:
   virtual void addOpenNode ( AStarNode *node ) = 0;

   AStarNode *leastH;
   AStarNode *stock;
   int numNodes;
   int tmpMaxNodes;
};

}}}

#endif