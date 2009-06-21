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
// File: path_finder_llsr.h
//
// Low Level Search Routines, and additional support structures
//

#ifndef _GLEST_GAME_PATHFINDER_LLSR_H_
#define _GLEST_GAME_PATHFINDER_LLSR_H_

#include "vec.h"
#include "unit_stats_base.h"

#include <vector>
#include <list>
#include <set>

//
// Please select your preferred internal data structure...
// Note: this data structure is only used to test whether
// a position is listed already or not, the actual open/closed
// lists are indeed lists (in the same memory space, closed 
// grows 'up' from the bottom, open grows 'down' from the top).
// This structure was optimized for the LHF algorithm, and 
// may not be appropriate for more typical search algorithms.
//
// Please define exactly one of these...
#define NODEPOOL_USE_MARKER_ARRAY
//#define NODEPOOL_USE_PACKED_MARKER_ARRAY // but not this one :)
//#define NODEPOOL_USE_REDBLACK_TREE
//#define NODEPOOL_USE_STL_SET

// Marker Arrays are simply an array, a 'perfect' hashtable for rectangular maps.
//
// The default marker array uses a lazy clearing scheme (ie, the array is never
// cleared) and a single comparison to test a position's 'listedness', it therefore
// runs in 'constant time' [O(1)]. Fastest, but requires 4 (or 8) bytes per map cell.
//
// The packed marker array uses a single bit per map cell, and therefore needs to be 
// cleared before use, and lookups require some bit masking. It still runs in 
// constant time [O(1)], but is obviously slightly slower than the unpacked version.
//
// The Red-Black tree is my own implementation of a Red-Black tree, plans to extend
// this class exist, to make it a suitable structure for the open list of a regular 
// A* search. As is it provides performance virtualy identical to std::set, which 
// isn't surprising of course, as std::set is almost certainly also a Red-Black Tree.
// Runs in logarithmic time [O(log n)]


//#define PATHFINDER_TREE_TIMING

// Time calls to aStar() and aaStar() ? All the below symbols require this one too.
//#define PATHFINDER_TIMING

// dump state if path not found [aaStar()]
//#define PATHFINDER_DEBUGGING_LOG_FAILURES

// dump state on excessively long calls [aaStar()]
//#define PATHFINDER_DEBUGGING_LOG_LONG_CALLS

// what constitutes a excessively long time ?
//#define PATHFINDER_LOGLIMIT 5000

// Intensive timing of aaStar()
// Defining this symbol will log EVERY call to aaStar()
//#define PATHFINDER_INTENSIVE_TIMING

using std::list;

namespace Glest { namespace Game {

// Vector list iterators...
typedef list<Vec2i>::iterator VLIt;
typedef list<Vec2i>::const_iterator VLConIt;
typedef list<Vec2i>::reverse_iterator VLRevIt;
typedef list<Vec2i>::const_reverse_iterator VLConRevIt;

class NodePool;
struct SearchParams;
struct SearchNode;
//struct AStarNode;
class Map;
class AnnotatedMap;

// =====================================================
// class LowLevelSearch
//
// static collection of low level search routines
// and a few support functions.
// =====================================================
class LowLevelSearch
{
public:
   //
   // Function pointers, to be supplied by the implementing application,
   // preferably before searching ;-)
   //
   //coming soon!

   //
   // Some variables we need...
   //
   static AnnotatedMap *aMap; // the annotated map to search on
   static Map *cMap; // the cell map // REMOVE ME, replace with function pointer to a hasUnit() type function
   static NodePool nodePool;

   //
   // Search Functions
   //

   // Least Heuristic First Search
   // Glest's original algorithm, with shiny new custom data structures
   static bool LHF_Search ( SearchParams params, list<Vec2i> &path, bool ucStart=true );

   // 'Least Heuristic First Ping Pong'
   // LHF_Search() run from start to goal, then goal to start and
   // the paths 'merged' to create a single higher quality path
   static bool LHF_PingPong ( SearchParams params, list<Vec2i> &path );

   // The Classic A* [Hart, Nilsson, & Raphael, 1968]
   //static bool AStar ( SearchParams params, list<Vec2i> &path, bool ucStart=true );

   // Fringe Search ? (a bit of A* with a dash of IDA*) [Björnsson, et al, 2005]
   //static bool FringeSearch ( SearchParams params, list<Vec2i> &path, bool ucStart=true );

   // Algorithm C ? [Bagchi & Mahanti, 1983]
   //static bool AlgorithmC ( SearchParams params, list<Vec2i> &path, bool ucStart=true );

   //
   // Support functions
   //

   // Euclidean distance heuristic
   static float heuristic ( const Vec2i &p1, const Vec2i &p2 ) { return p1.dist ( p2 ); }

   // true if any path to at least radius cells away can be found
   // Performs a modified LHF search where the heuristic is substituted
   // for zero minus the distance to here, the algorithm thus tries to 'escape'
   // (ie, get as far from start as quickly as possible).
   static bool canPathOut ( const Vec2i &pos, const int radius, Field field );

   // fills d1 and d2 with the diagonal cells(coords) that need checking for a 
   // unit of size to move from s to d, for diagonal moves.
   // WARNING: ASSUMES ( s.x != d.x && s.y != d.y ) ie. the move IS diagonal
   // if this condition is not true, results are undefined
   //
   // This should be inline (or even __inline perhaps), but that's causing issues with gcc...
   static void getPassthroughDiagonals ( const Vec2i &s, const Vec2i &d, 
                                    const int size, Vec2i &d1, Vec2i &d2 );

   static void getCrossOverPoints ( const list<Vec2i> &one, const list<Vec2i> &two, list<Vec2i> &result );
   static bool mergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, list<Vec2i> &path );

   static void copyToPath ( const list<Vec2i> &pathList, list<Vec2i> &path );
   
};

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
// struct SearchNode
//
// A basic search node, has no 'distance to here' field
// =====================================================
struct SearchNode
{
	Vec2i pos;
	SearchNode *next;
	SearchNode *prev;
   float heuristic;//union { float f; int i; } heuristic;
	bool exploredCell;
};

// =====================================================
// struct AStarNode
//
// A node structure for A* and friends
// =====================================================
//struct AStarNode : public SearchNode
//{
//   union { float f; int i; } distToHere;
//};

// =====================================================
// struct PosTreeNode
//
// A Red-Black Tree node.
// =====================================================
struct PosTreeNode
{
   enum Colour { Red, Black };
   PosTreeNode *left, *right, *parent;
   Colour colour;
   Vec2i pos;
};

// =====================================================
// struct PosTree
//
// A Red-Black Tree structure, that stores 'positions'
// Size limited to PathFinder::pathFindNodesMax
// =====================================================
struct PosTree
{
   PosTreeNode *posStock;
   PosTreeNode *posRoot;
   PosTreeNode sentinel;

   PosTree ();
   virtual ~PosTree ();

   void clear () { posRoot = NULL; numPos = 0; }
   void add ( const Vec2i &pos );
   bool isIn ( const Vec2i &pos ) const;

#if defined(DEBUG) || defined(_DEBUG)
   bool assertValidity ();
   void dump ();
#endif

private:
   int numPos;

   void rebalance ( PosTreeNode *node );
   inline void rotateRight ( PosTreeNode *node );
   inline void rotateLeft ( PosTreeNode *node );
};

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
// struct PackedPosMarkerArray
//
// A Packed Marker Array, using 1 bit per cell, must be 
// cleared before use, and involves much bit masking.
// =====================================================
struct PackedPosMarkerArray
{
   // TODO: Fill me in!
};

// =====================================================
// class NodePool
//
// An interface to the open/closed lists, used mainly to 
// hide away all the nasty conditional compilation to
// support various different underlying data structures
//
// =====================================================
class NodePool
{
   SearchNode *stock;
   SearchNode **lists;
   int numOpen, numClosed, numTotal, numPos;
   int maxNodes, tmpMaxNodes;

#if defined ( NODEPOOL_USE_REDBLACK_TREE )
   PosTree posTree;
#elif defined ( NODEPOOL_USE_MARKER_ARRAY )
   PosMarkerArray markerArray;
#elif defined ( NODEPOOL_USE_PACKED_MARKER_ARRAY )
   PackedPosMarkerArray markerArray;
#elif defined ( NODEPOOL_USE_STL_SET )
   set<Vec2i> posSet;
#endif
public:
   NodePool ();
   ~NodePool ();
   void init ( Map *map );

   // reset everything, include maxNodes...
   void reset ();
   // will be == PathFinder::pathFindMaxNodes (returns 'normal' max, not temp)
   int getMaxNodes () const { return maxNodes; }
   // sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
   void setMaxNodes ( const int max );
   // Is this pos already listed?
   bool isListed ( const Vec2i &pos )
#if defined ( NODEPOOL_USE_REDBLACK_TREE )
      { return posTree.isIn ( pos ); }
#elif defined ( NODEPOOL_USE_MARKER_ARRAY )
      { return markerArray.isMarked ( pos ); }
#endif

   void listPos ( const Vec2i &pos )
#if defined ( NODEPOOL_USE_REDBLACK_TREE )
      { posTree.add ( pos ); }
#elif defined ( NODEPOOL_USE_MARKER_ARRAY )
      { markerArray.setMark ( pos ); }
#endif

   bool addToOpen ( SearchNode* prev, const Vec2i &pos, float h, bool exp = true );

   // moves 'best' node from open to closed, and returns it, or NULL if open is empty
   SearchNode* getBestCandidate ();

   // used to insert by highest distance rather than lowest heuristic, for PathFinder::CanPathOut()
   //bool addToOpenHD ( AStarNode* prev, const Vec2i &pos, float d );

#ifdef NODEPOOL_USE_REDBLACK_TREE
#endif
}; // class PathFinder::NodePool


// =====================================================
// Debugging Structures 
//
//	structs to collect various statistics.
// =====================================================

#ifdef PATHFINDER_TIMING
struct PathFinderStats
{
   int64 astar_calls;
   int64 astar_avg;
   int64 lastsec_calls;
   int64 lastsec_avg;
   int64 total_astar_calls;
   int64 total_astar_avg;
   int64 total_path_recalcs;

   int64 worst_astar;
   int64 calls_rejected;

   PathFinderStats ();
   void resetCounters ();
   void AddEntry ( int64 ticks );
   void IncReject () { calls_rejected++; }
   char* GetTotalStats ();
   char* GetStats ();
   char buffer[512];
};
#endif
#ifdef PATHFINDER_TREE_TIMING
struct TreeStats
{
   int numTreesBuilt;
   
   int avgSearchesPerTree;
   int64 avgSearchTime;
   int64 avgWorstSearchTime;
   int64 worstSearchEver;

   int searchesThisTree;
   int64 avgSearchThisTree;
   int64 worstSearchThisTree;

   int avgInsertsPerTree;
   int64 avgInsertTime;
   int64 avgWorstInsertTime;
   int64 worstInsertEver;

   int insertsThisTree;
   int64 avgInsertThisTree;
   int64 worstInsertThisTree;
   
   char buffer[512];
   TreeStats ();
   void reset ();
   void newTree ();
   void addSearch ( int64 time );
   void addInsert ( int64 time );
   char * output ( char *prefix = "");
};
#endif

}}

#endif
