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
// Low Level Search Routines and additional support structures
//

#ifndef _GLEST_GAME_PATHFINDER_LLSR_H_
#define _GLEST_GAME_PATHFINDER_LLSR_H_

#include "vec.h"
#include "unit_stats_base.h"

// TODO: Figure out why pre-compiled headers are not working with gcc...
#include <vector>
#include <list>
#include <set>

//
// define this if you know your heuristic is admissable,
// or if it isn't but you don't care (you'll get sub-optimal
// paths, but you'll get them faster).
#define LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC

//
// Please select your preferred internal data structure...
// Note: this data structure is only used to test whether
// a position is listed already or not, the actual open/closed
// lists are indeed 'lists' (actually indexed arrays, in the 
// same memory space, closed grows 'up' from the bottom, open 
// grows 'down' from the top).
// This 'dual-list' structure was optimized for the BFS algorithm, 
// and may not be appropriate for more typical search algorithms.
//
// Please define exactly one of these...
#define NODEPOOL_USE_MARKER_ARRAY
//#define NODEPOOL_USE_PACKED_MARKER_ARRAY // but not this one :)
//#define NODEPOOL_USE_REDBLACK_TREE
//#define NODEPOOL_USE_STL_SET

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
//
// The Red-Black tree is my own implementation of a Red-Black tree, plans to extend
// this class exist, to make it a suitable structure for the open list of a regular 
// A* search. As is it provides performance virtualy identical to std::set, which 
// isn't surprising of course, as std::set is almost certainly also a Red-Black Tree.
// look-ups and inserts run in logarithmic time [O(log n)]

using std::list;

namespace Glest { namespace Game {

// Vector list iterators...
typedef list<Vec2i>::iterator VLIt;
typedef list<Vec2i>::const_iterator VLConIt;
typedef list<Vec2i>::reverse_iterator VLRevIt;
typedef list<Vec2i>::const_reverse_iterator VLConRevIt;

class Map;
class AnnotatedMap;

// =====================================================
// namespace LowLevelSearch
//
// static collection of low level search routines
// and a few support functions.
// =====================================================
namespace LowLevelSearch
{

   class BFSNodePool;
   class AStarNodePool;

   struct SearchParams;
   struct BFSNode;
   struct AStarNode;
   //
   // Function pointers, to be supplied/filled in by the implementing application,
   // preferably before searching ;-)
   //

   //
   // The heuristic to use, supply your own or use one of
   // LowLevelSearch::euclideanDistance or 
   // LowLevelSearch::manhattenDistance
   //
   // To create an simple over-estimating heuristic, simply scale a call 
   // to either of the above,
   // eg.
   // float myHeuristic ( const Vec2i &p1, const Vec2i &p2 ) 
   //    { return LowLevelSearch::manhattenDistance (p1,p2) * 1.8; }
   //
   // If you do this, technically you should un-define LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC
   // but if sub-optimal paths are acceptable, don't...
   //
   extern float (*heuristic) ( const Vec2i &p1, const Vec2i &p2 );
   
   //
   // The search function. init() currently sets this to BestFirstPingPong
   //
   // see under 'Search Functions' below ...
   extern bool (*search) ( SearchParams &sp, list<Vec2i> &path );

   //
   // Some variables we need...
   //
   // These need to be supplied externally
   extern AnnotatedMap *aMap; // the annotated map to search on
   extern Map *cMap; // the cell map // REMOVE ME, replace with function pointer to a hasUnit() type function

   // init() will create these.
   extern BFSNodePool *bNodePool;
   extern AStarNodePool *aNodePool;

   void init (); // basic/advanced nodepool ?

   //
   // Search Functions
   //

   // Best First Search, this is called by BestFirstSearch, and twice by BestFirstPingPong
   // the unit checking behaviour needs to be reversed with the PingPong method, hence
   // the funny signature, assign BestFirstSearch to 'search' if you just want a regular
   // best first search
   bool _BestFirstSearch ( SearchParams &params, list<Vec2i> &path, bool ucStart );

   // A Best First Search (BFS)
   bool BestFirstSearch ( SearchParams &params, list<Vec2i> &path );
      
   // 'Best First Search - Ping Pong'
   // BestFirstSearch() run from start to goal, then goal to start and
   // the two paths 'merged' to create a single higher quality path
   bool BestFirstPingPong ( SearchParams &params, list<Vec2i> &path );

   // The Classic A* [Hart, Nilsson, & Raphael, 1968]
   bool AStar ( SearchParams &params, list<Vec2i> &path );

   // Fringe Search ? (a bit of A* with a dash of IDA*) [Björnsson, et al, 2005]
   //static bool FringeSearch ( SearchParams params, list<Vec2i> &path );

   // Algorithm C ? [Bagchi & Mahanti, 1983]
   //static bool AlgorithmC ( SearchParams params, list<Vec2i> &path );

   //
   // Support functions
   //

   // Euclidean distance heuristic
   __inline float euclideanHeuristic ( const Vec2i &p1, const Vec2i &p2 );

   // true if any path to at least radius cells away can be found
   // Performs a modified Best First Search where the heuristic is substituted
   // for zero minus the distance to here, the algorithm thus tries to 'escape'
   // (ie, get as far from start as quickly as possible).
   bool canPathOut ( const Vec2i &pos, const int radius, Field field );

   void getCrossOverPoints ( const list<Vec2i> &one, const list<Vec2i> &two, list<Vec2i> &result );
   bool mergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, list<Vec2i> &path );

   void copyToPath ( const list<Vec2i> &pathList, list<Vec2i> &path );
   
   // Direction Vectors...
   extern Vec2i Directions[8];

   // fills d1 and d2 with the diagonal cells(coords) that need checking for a 
   // unit of size to move from s to d, for diagonal moves.
   // WARNING: ASSUMES ( s.x != d.x && s.y != d.y ) ie. the move IS diagonal
   // if this condition is not true, results are undefined
   //
   __inline void getPassthroughDiagonals ( const Vec2i &s, const Vec2i &d, 
                                    const int size, Vec2i &d1, Vec2i &d2 );

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
   float estCost () const { return distToHere + heuristic; }
}; // 25 bytes == 28 in mem ?

#if defined ( NODEPOOL_USE_REDBLACK_TREE )
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
#elif defined ( NODEPOOL_USE_MARKER_ARRAY )
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
   
   DoubleMarkerArray () {counter=0;marker=NULL;};
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
#elif defined ( NODEPOOL_USE_PACKED_MARKER_ARRAY )
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
#endif

// =====================================================
// class BFSNodePool
//
// An interface to the open/closed lists, used mainly to 
// hide away all the nasty conditional compilation to
// support various different underlying data structures
//
// This NodePool supports the BFS algorithm, and operates
// on BFSNode structures. Tests for 'listedness' report 
// if a position is listed or not, there is no mechanism 
// to test whether a listed position is open or closed.
//
// =====================================================
class BFSNodePool
{
   BFSNode *stock;
   BFSNode **lists;
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
   BFSNodePool ();
   ~BFSNodePool ();
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
#elif defined ( NODEPOOL_USE_STL_SET )
      { return posSet.find ( pos ) != posSet.end(); }
#endif

   void listPos ( const Vec2i &pos )
#if defined ( NODEPOOL_USE_REDBLACK_TREE )
      { posTree.add ( pos ); }
#elif defined ( NODEPOOL_USE_MARKER_ARRAY )
      { markerArray.setMark ( pos ); }
#elif defined ( NODEPOOL_USE_STL_SET )
      { posSet.add ( pos ); }
#endif

   bool addToOpen ( BFSNode* prev, const Vec2i &pos, float h, bool exp = true );

   // moves 'best' node from open to closed, and returns it, or NULL if open is empty
   BFSNode* getBestCandidate ();

}; // class BFSNodePool

// =====================================================
// class AStarNodePool
//
// An interface to the open/closed lists, used mainly to 
// hide away all the nasty conditional compilation to
// support various different underlying data structures
//
// =====================================================
class AStarNodePool
{
   DoubleMarkerArray markerArray;
   AStarNode *stock;
   AStarNode **lists;
   int numOpen, numClosed, numTotal, numPos;
   int maxNodes, tmpMaxNodes;

public:
   AStarNodePool ();
   ~AStarNodePool ();
   void init ( Map *map );

   // reset everything, include maxNodes...
   void reset ();
   // will be == PathFinder::pathFindMaxNodes (returns 'normal' max, not temp)
   int getMaxNodes () const { return maxNodes; }
   // sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
   void setMaxNodes ( const int max );
   // Is this pos already listed?
   bool isListed ( const Vec2i &pos ) { return markerArray.isListed ( pos ); }
   bool isOpen ( const Vec2i &pos ) { return markerArray.isOpen ( pos ); }
   //AStarNode* getOpenNode ( const Vec2i &pos );
   void updateOpenNode ( const Vec2i &pos, AStarNode *curr );

#ifndef LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC
   bool isClosed ( const Vec2i &pos ) { return markerArray.isClosed ( pos ); }
   AStarNode* getClosedNode ( const Vec2i &pos );
   void reOpen ( AStarNode *node );
#endif
   bool addToOpen ( AStarNode* prev, const Vec2i &pos, float h, float d, bool exp = true );

   // moves 'best' node from open to closed, and returns it, or NULL if open is empty
   AStarNode* getBestCandidate ();
};

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

}}}; // namespace Glest::Game::LowLevelSearch

#endif
