// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//               2009      James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_PATHFINDER_H_
#define _GLEST_GAME_PATHFINDER_H_

#include "vec.h"

#include <vector>
#include <list>
#include <set>
#include "unit_stats_base.h"

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

using std::vector;
using std::list;
using Shared::Graphics::Vec2i;
using Shared::Platform::uint32;
using Shared::Platform::int64;

namespace Glest{ namespace Game{

class Map;
class Unit;
class UnitPath;
//enum Field;

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

// Stats
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


// =====================================================
// 	class PathFinder
//
///	Finds paths for units using a modification of the A* algorithm
// =====================================================

class PathFinder{
public:
	enum TravelState { tsArrived, tsOnTheWay, tsBlocked };

   static PathFinder* getInstance ();
	~PathFinder();
	void init(Map *map);
	TravelState findPath(Unit *unit, const Vec2i &finalPos);

   // Start a 'cascading update' of the Map Metrics from a position and size
   void updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field ); 
   // Initialise the Map Metrics
   void initMapMetrics ();

   // Interface to the clearance metrics, can a unit of size occupy a cell(s) ?
   bool canOccupy ( const Vec2i &pos, int size, Field field ) const;

	static const int maxFreeSearchRadius;
	static const int pathFindRefresh; // now ignored.. but will be used (at higher val) for nodeLimitReached searches
   static const int pathFindNodesMax;
   static const int maxClearanceValue;

   // legal move ?
   bool isLegalMove ( Unit *unit, const Vec2i &pos ) const;

#ifdef PATHFINDER_TREE_TIMING
   char* treeStats ()
   {
      static char buffer[2048];
      sprintf ( buffer, "Tree Look-Up & Insert Stats...\n%s\n%s", nPool.getStats ( true ), nPool.getStats ( false ) );
      return buffer;
   }
#endif
#if defined(PATHFINDER_TIMING) || defined(PATHFINDER_DEBUG_TEXTURES )
   CellMetrics **metrics; // clearance values [y][x]
#else
private:
   CellMetrics **metrics; // clearance values [y][x]
#endif
private:
   static PathFinder *singleton;
	PathFinder();
	PathFinder(Map *map);
   int metricHeight;

   struct AAStarParams
   {
      Vec2i start, dest;
      Field field;
      int size, team;
      AAStarParams ( Unit *u );
   };
   bool aaStar ( AAStarParams params, list<Vec2i> &path, bool ucStart=true );
   //TravelState aaStar ( Unit *unit, const Vec2i &finalPos );

   void getCrossOverPoints ( const list<Vec2i> &one, const list<Vec2i> &two, list<Vec2i> &result );
   void MergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, UnitPath *path );

   // true if any path to at least radius cells away can be found
   bool canPathOut ( const Vec2i &pos, const int radius, Field field );

   void copyToPath ( const list<Vec2i> pathList, UnitPath *path );

   // fills d1 and d2 with the diagonal cells(coords) that need checking for a 
   // unit of size to move from s to d, for diagonal moves.
   // WARNING: ASSUMES ( s.x != d.x && s.y != d.y ) ie. the move IS diagonal
   // if this condition is not true, results are undefined
   inline void getPassthroughDiagonals ( const Vec2i &s, const Vec2i &d, 
                                    const int size, Vec2i &d1, Vec2i &d2 ) const;

	Vec2i computeNearestFreePos (const Unit *unit, const Vec2i &targetPos);
	
   //void smoothPath ( AStarNode *start );
   //AstarNode* smoothSegment ( AStarNode *front,  AStarNode *rear, int dist )

   // should do some experiments, this involves a sqrt and two multiplications
   // we should be able to find something cheaper that gives as good results
   float heuristic(const Vec2i &pos, const Vec2i &finalPos) 
      {return pos.dist(finalPos);}
//#ifdef PATHFINDER_TIMING
//   int newHeuristic (const Vec2i &sp, const Vec2i &fp)
//      {return (abs( sp.x - fp.x ) + abs( sp.y - fp.y ));}
//   static bool heuristicFlipper;
//#endif

   struct AStarNode
   {
		Vec2i pos;
		AStarNode *next;
		AStarNode *prev;
//#ifdef PATHFINDER_TIMING
      // for testing different heuristics
//      union { float f; int i; } heuristic; 
//#else
      float heuristic;
//#endif
      float distToHere;
		bool exploredCell;
   };
   enum Colour { Red, Black };
   struct PosTreeNode
   {
      PosTreeNode *left, *right, *parent;
      Colour colour;
      Vec2i pos;
   };
   class NodePool
   {
      AStarNode *stock;
      AStarNode **lists;
      PosTreeNode *posStock;
      PosTreeNode *posRoot;
      PosTreeNode sentinel;
      int numOpen, numClosed, numTotal, numPos;
      int maxNodes, tmpMaxNodes;

#ifdef PATHFINDER_TREE_TIMING
   public:
      TreeStats setStats, rbStats;
      std::set<Vec2i> posSet; // for comparison
      bool useSet; // the flipper
      int numRuns; // number of times to consecutively run with same structure
      int curRun;

      char *getStats ( bool set )
      {
         if ( set ) return setStats.output ( "std::set" );
         else       return rbStats.output  ( "PosTree:" );
      }
#endif


   public:
      NodePool ();
      ~NodePool ();

      // reset everything, include maxNodes...
      void reset ();
      // will be == PathFinder::pathFindMaxNodes (returns 'normal' max, not temp)
      int getMaxNodes () const { return maxNodes; }
      // sets a temporary maximum number of nodes to use (50 <= max <= pathFindMaxNodes)
      void setMaxNodes ( const int max );
      // Is this pos already listed?
      bool isListed ( const Vec2i &pos )
#ifndef PATHFINDER_TREE_TIMING  
         const
#endif
         ;
      // add this pos, return true on success, false if out of nodes
//#ifdef PATHFINDER_TIMING
//      bool addToOpen ( AStarNode* prev, const Vec2i &pos, void *h, float d, bool exp );
//#else
      bool addToOpen ( AStarNode* prev, const Vec2i &pos, float h, float d, bool exp = true );
//#endif
      // moves 'best' node from open to closed, and returns it, or NULL if open is empty
      AStarNode* getBestCandidate ();

      // used to insert by highest distance rather than lowest heuristic, for PathFinder::CanPathOut()
      bool addToOpenHD ( AStarNode* prev, const Vec2i &pos, float d );

      // should be private, but getCrossOverPoints() also uses this 'functionality'
      void addToTree ( const Vec2i &pos );
      void rebalanceTree ( PosTreeNode *node );
      void rebalance2 ( PosTreeNode *node );
      inline void rotateRight ( PosTreeNode *node );
      inline void rotateLeft ( PosTreeNode *node );

#if defined(DEBUG) || defined(_DEBUG)
      bool assertTreeValidity ();
      void dumpTree ();
#endif
   }; // class PathFinder::NodePool

   NodePool nPool;
   Map *map;

   // for initMetrics () and updateMapMetrics ()
   CellMetrics computeClearances ( const Vec2i & );
   uint32 computeClearance ( const Vec2i &, Field );
   bool canClear ( const Vec2i &pos, int clear, Field field );
#ifdef PATHFINDER_TIMING
public:
   PathFinderStats statOld, statNew;
   void resetCounters () 
   {
      statNew.resetCounters ();
      statOld.resetCounters ();
   };
   // debugging...
   void AssertValidPath ( AStarNode * );
   void DumpPath ( UnitPath *path, const Vec2i &start, const Vec2i &dest );
#endif

}; // class PathFinder

}}//end namespace

#endif
