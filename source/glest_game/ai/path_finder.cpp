// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//               2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Does not use pre-compiled header because it's optimized in debug build.

// Currently DOES use precompiled header, because it's no longer optimized in debug
// because debugging optimized code is not much fun :-)

// Actually, no need for this one to be optimized anymore, when path finding is stable again, 
// pathfinder_llsr.cpp should get this treatment, that's where the 'hard work' is done now.

#include "pch.h"

#include "path_finder.h"
#include "path_finder_llsr.h"

/*
#include <algorithm>
#include <cassert>
#include <iostream>
*/
#include "config.h"
#include "map.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ========================

const int PathFinder::maxFreeSearchRadius = 10;

// this is limiting the stored path ... why calculate 
// a long path, then only store the first 10 steps!!!
// We can handle this better by simply recalcing when/if blocked
// (which is happening anyway?)
const int PathFinder::pathFindRefresh = 10; // now unused
const int PathFinder::pathFindNodesMax = 800;// = Config::getInstance ().getPathFinderMaxNodes ();

PathFinder* PathFinder::singleton = NULL;

PathFinder* PathFinder::getInstance ()
{
   if ( ! singleton )
      singleton = new PathFinder ();
   return singleton;
}

PathFinder::PathFinder()
{
   LowLevelSearch::init ();
   LowLevelSearch::aMap = NULL;
   LowLevelSearch::cMap = NULL;
   annotatedMap = NULL;
   singleton = this;
}

PathFinder::~PathFinder ()
{
   LowLevelSearch::aMap = NULL;
   LowLevelSearch::cMap = NULL;
   if ( annotatedMap ) delete annotatedMap;
}

void PathFinder::init ( Map *map )
{
	this->map= map;
   if ( annotatedMap ) delete annotatedMap;
   annotatedMap = new AnnotatedMap ( map );
   LowLevelSearch::cMap = map;
   LowLevelSearch::aMap = annotatedMap;
   LowLevelSearch::bNodePool->init ( map );
   LowLevelSearch::aNodePool->init ( map );
}

bool PathFinder::isLegalMove ( Unit *unit, const Vec2i &pos2 ) const
{
   if ( unit->getPos().dist ( pos2 ) > 1.5 ) return false; // shouldn't really need this....

   const Vec2i &pos1 = unit->getPos ();
   const int &size = unit->getSize ();
   const Field &field = unit->getCurrField ();
   Zone cellField = field == mfAir ? fAir : fSurface;
   Tile *sc = map->getTile ( map->toTileCoords ( pos2 ) );

   if ( ! annotatedMap->canOccupy ( pos2, size, field ) )
      return false;
   if ( pos1.x != pos2.x && pos1.y != pos2.y )
   {  // Proposed move is diagonal, check if cells either 'side' are free.
      Vec2i diag1, diag2;
      LowLevelSearch::getPassthroughDiagonals ( pos1, pos2, size, diag1, diag2 );
      if ( ! annotatedMap->canOccupy (diag1, 1, field) 
      ||   ! annotatedMap->canOccupy (diag2, 1, field) ) 
		   return false; // obstruction, can not move to pos2
      if ( ! map->getCell (diag1)->isFree (cellField)
      ||   ! map->getCell (diag2)->isFree (cellField) )
         return false; // other unit in the way
   }
   for ( int i = pos2.x; i < unit->getSize () + pos2.x; ++i )
      for ( int j = pos2.y; j < unit->getSize () + pos2.y; ++j )
         if ( map->getCell (i,j)->getUnit (cellField) != unit
         &&   ! map->isFreeCell (Vec2i(i,j), field) ) //! map->getCell (i,j)->isFree (cellField)
            return false;
   // pos2 is free, and nothing is in the way
	return true;
}

PathFinder::TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos)
{
   static int flipper = 0;
   //route cache
	UnitPath *path= unit->getPath();
	if(finalPos==unit->getPos())
   {	//if arrived (where we wanted to go)
		unit->setCurrSkill(scStop);
		return tsArrived;
	}
	else if(!path->isEmpty())
   {	//route cache
		Vec2i pos= path->pop();
      if ( isLegalMove ( unit, pos ) )
      {
			unit->setNextPos(pos);
			return tsOnTheWay;
		}
	}
   //route cache miss
	const Vec2i targetPos = computeNearestFreePos ( unit, finalPos );

   //if arrived (as close as we can get to it)
	if ( targetPos == unit->getPos () )
   {
      unit->setCurrSkill(scStop);
		return tsArrived;
   }
   // some tricks to determine if we are probably blocked on a short path, without
   // an exhuastive and expensive search through pathFindNodesMax nodes
   float dist = unit->getPos().dist ( targetPos );
   if ( unit->getCurrField () == mfWalkable 
   &&   map->getTile (Map::toTileCoords ( targetPos ))->isVisible (unit->getTeam ()) )
   {
      int radius;
      if ( dist < 5 ) radius = 2;
      else if ( dist < 10 ) radius = 3;
      else if ( dist < 15 ) radius = 4;
      else radius = 5;
      if ( ! LowLevelSearch::canPathOut ( targetPos, radius, mfWalkable ) ) 
      {
         unit->getPath()->incBlockCount ();
         unit->setCurrSkill(scStop);
         return tsBlocked;
      }
   }

   if ( flipper ++ % 2 )
   {
      LowLevelSearch::BFSNodePool &nPool = *LowLevelSearch::bNodePool;
      LowLevelSearch::search = &LowLevelSearch::BestFirstPingPong;
      nPool.reset ();
      // dynamic adjustment of nodeLimit, based on distance to target
      if ( dist < 5 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 8 );      // == 100 nodes
      else if ( dist < 10 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 4 );// == 200 nodes
      else if ( dist < 15 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 2 );// == 400 nodes
      // else a fixed -100, so the backward run has more nodes, and if the forward run hits 
      // the nodeLimit, the backward run is more likely to succeed.
      else nPool.setMaxNodes ( nPool.getMaxNodes () - 100 );// == 700 nodes
   }
   else
   {
      LowLevelSearch::AStarNodePool &nPool = *LowLevelSearch::aNodePool;
      LowLevelSearch::search = &LowLevelSearch::AStar;
      nPool.reset ();
      // dynamic adjustment of nodeLimit, based on distance to target
      if ( dist < 5 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 8 );      // == 100 nodes
      else if ( dist < 10 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 4 );// == 200 nodes
      else if ( dist < 15 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 2 );// == 400 nodes
   }

   LowLevelSearch::SearchParams params (unit);
   params.dest = targetPos;
   list<Vec2i> pathList;
   
   if ( LowLevelSearch::search ( params, pathList ) )
   {
      copyToPath ( pathList, unit->getPath () );
      if ( LowLevelSearch::search == LowLevelSearch::BestFirstPingPong )
         Logger::getInstance ().add ( "BestFirstPingPong Used, returned true." );
      else
         Logger::getInstance ().add ( "AStar Used, returned true." );
   }
   else
   {
      if ( LowLevelSearch::search == LowLevelSearch::BestFirstPingPong )
         Logger::getInstance ().add ( "BestFirstPingPong Failed." );
      else
         Logger::getInstance ().add ( "AStar Failed." );
      unit->getPath()->incBlockCount ();
      unit->setCurrSkill(scStop);
      return tsBlocked;
   }

	Vec2i pos= path->pop(); //crash point
   if ( ! isLegalMove ( unit, pos ) )
   {
		unit->setCurrSkill(scStop);
      unit->getPath()->incBlockCount ();
		return tsBlocked;
	}
   unit->setNextPos(pos);
	return tsOnTheWay;
}
void PathFinder::copyToPath ( const list<Vec2i> pathList, UnitPath *path )
{
   list<Vec2i>::const_iterator it = pathList.begin();
   // skip start pos, store rest
   for ( ++it; it != pathList.end(); ++it )
      path->push ( *it );
}

// ==================== PRIVATE ====================

// return finalPos if free, else a nearest free pos within maxFreeSearchRadius
// cells, or unit's current position if none found
Vec2i PathFinder::computeNearestFreePos (const Unit *unit, const Vec2i &finalPos){
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
   Field field = unit->getCurrField();// == mfAir ? fAir : fSurface;
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	
   if(map->areAproxFreeCells(finalPos, size, field, teamIndex)){
		return finalPos;
	}

	//find nearest pos
	Vec2i nearestPos= unitPos;
	float nearestDist= unitPos.dist(finalPos);
	for(int i= -maxFreeSearchRadius; i<=maxFreeSearchRadius; ++i){
		for(int j= -maxFreeSearchRadius; j<=maxFreeSearchRadius; ++j){
			Vec2i currPos= finalPos + Vec2i(i, j);
			if(map->areAproxFreeCells(currPos, size, field, teamIndex)){
				float dist= currPos.dist(finalPos);

				//if nearer from finalPos
				if(dist<nearestDist){
					nearestPos= currPos;
					nearestDist= dist;
				}
				//if the distance is the same compare distance to unit
				else if(dist==nearestDist){
					if(currPos.dist(unitPos)<nearestPos.dist(unitPos)){
						nearestPos= currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}


#ifdef PATHFINDER_TREE_TIMING
TreeStats::TreeStats ()
{
   numTreesBuilt = avgSearchesPerTree = 0;
   avgSearchTime = avgWorstSearchTime = worstSearchEver = 0;

   searchesThisTree = 0;
   avgSearchThisTree = worstSearchThisTree = 0;

   avgInsertsPerTree = avgInsertTime = avgWorstInsertTime = worstInsertEver = 0;
   insertsThisTree = 0;
   avgInsertThisTree = worstInsertThisTree = 0;

}
void TreeStats::reset () 
{
   avgSearchTime = ( avgSearchTime * numTreesBuilt + avgSearchThisTree ) / ( numTreesBuilt + 1 );
   avgWorstSearchTime = ( avgWorstSearchTime * numTreesBuilt + worstSearchThisTree ) / ( numTreesBuilt + 1 );
   avgSearchesPerTree = ( avgSearchesPerTree * numTreesBuilt + searchesThisTree ) / ( numTreesBuilt + 1 );
   if ( worstSearchThisTree > worstSearchEver ) 
      worstSearchEver = worstSearchThisTree;

   avgInsertTime = ( avgInsertTime * numTreesBuilt + avgInsertThisTree ) / ( numTreesBuilt + 1 );
   avgWorstInsertTime = ( avgWorstInsertTime * numTreesBuilt + avgInsertThisTree ) / ( numTreesBuilt + 1 );
   avgInsertsPerTree = ( avgInsertsPerTree * numTreesBuilt + insertsThisTree ) / ( numTreesBuilt + 1 );
   if ( worstInsertThisTree > worstInsertEver )
      worstInsertEver = worstInsertThisTree;

}
void TreeStats::newTree () 
{ 
   if ( searchesThisTree || insertsThisTree ) 
   {
      reset(); 
      numTreesBuilt++; 
   }
   searchesThisTree = 0;
   avgSearchThisTree = 0; 
   worstSearchThisTree = 0;
   insertsThisTree = 0;
   avgInsertThisTree = 0;
   worstInsertThisTree = 0;
}

void TreeStats::addSearch ( int64 time )
{
   if ( searchesThisTree )
      avgSearchThisTree = avgSearchThisTree * searchesThisTree + time;
   else
      avgSearchThisTree = time;
   searchesThisTree ++;
   avgSearchThisTree /= searchesThisTree;
   if ( time > worstSearchThisTree ) worstSearchThisTree = time;
}
void TreeStats::addInsert ( int64 time )
{
   if ( insertsThisTree )
      avgInsertThisTree = avgInsertThisTree * insertsThisTree + time;
   else
      avgInsertThisTree = time;
   insertsThisTree ++;
   avgInsertThisTree /= insertsThisTree;
   if ( time > worstInsertThisTree ) worstInsertThisTree = time;
}
char * TreeStats::output ( char *prefix )
{
   int off = sprintf ( buffer, "%s Avg number of searches on each tree: %d. Avg search time: %d. Avg Worst: %d. Absolute Worst: %d\n", 
      prefix, avgSearchesPerTree, (int)avgSearchTime, (int)avgWorstSearchTime, (int)worstSearchEver );
   sprintf ( buffer + off, "%s Avg number of inserts on each tree: %d. Avg insert time: %d. Avg Worst: %d. Absolute Worst: %d", 
      prefix, avgInsertsPerTree, (int)avgInsertTime, (int)avgWorstInsertTime, (int)worstInsertEver );
   return buffer;
}
#endif


#ifdef DEBUG
// Why is this here?  Doesn't it belong in world.cpp?  It's here because we compile path_finder.cpp
// optimized in debug since it's the only possible way you can really debug and this is a dog slow
// function.
class ValidationMap {
public:
	int h;
	int w;
	char *cells;

	ValidationMap(int h, int w) : h(h), w(w), cells(new char[h * w * fCount]) {
		reset();
	}

	void reset() {
		memset(cells, 0, h * w * fCount);
	}

	void validate(int x, int y, Field field) {
		assert(!getCell(x, y, field));
		getCell(x, y, field) = 1;
	}

	char &getCell(int x, int y, Field field) {
		assert(x >= 0 && x < w);
		assert(y >= 0 && y < h);
		assert(field >= 0 && field < fCount);
		return cells[field * h * w + x * w + y];
	}
};

void World::assertConsistiency() {
	// go through each map cell and make sure each unit in cells are supposed to be there
	// go through each unit and make sure they are in the cells they are supposed to be in
	// iterate through unit references: pets, master, target and make sure they are valid or
	//		null-references
	// make sure alive/dead states of all units is good.
	// whatever else I can think of

	static ValidationMap validationMap(map.getH(), map.getW());
	validationMap.reset();

	// make sure that every unit is in their cells and mark those as validated.
	for(Factions::iterator fi = factions.begin(); fi != factions.end(); ++fi) {
		for(int ui = 0; ui < fi->getUnitCount(); ++ui) {
			Unit *unit = fi->getUnit(ui);
/*
			if(!((unit->getHp() == 0 && unit->isDead()) || (unit->getHp() > 0 && unit->isAlive()))) {
				cerr << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout.flush();
				assert(false);
			}
*/
			if(unit->isDead() && unit->getCurrSkill()->getClass() == scDie) {
				continue;
			}

			const UnitType *ut = unit->getType();
			int size = ut->getSize();
			Field field = unit->getCurrField();
			const Vec2i &pos = unit->getPos();

			for(int x = 0; x < size; ++x) {
				for(int y = 0; y < size; ++y) {
					Vec2i currPos = pos + Vec2i(x, y);
					assert(map.isInside(currPos));

					if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
						Unit *unitInCell = map.getCell(currPos)->getUnit(field);
						if(unitInCell != unit) {
							cerr << "Unit id " << unit->getId()
									<< " from faction " << fi->getIndex()
									<< "(type = " << unit->getType()->getName() << ")"
									<< " not in cells (" << currPos.x << ", " << currPos.y << ", " << field << ")";
							if(unitInCell == NULL && !unit->getHp()) {
								cerr << " but has zero HP and is not executing scDie." << endl;
							} else {
								cerr << endl;
								assert(false);
							}
						}
						validationMap.validate(currPos.x, currPos.y, field);
					}
				}
			}
		}
	}

	// make sure that every cell that was not validated is empty
	for(int x = 0; x < map.getW(); ++x) {
		for(int y = 0; y < map.getH(); ++y ) {
			for(int field = 0; field < fCount; ++field) {
				if(!validationMap.getCell(x, y, (Field)field)) {
					Cell *cell = map.getCell(x, y);
					if(cell->getUnit(field)) {
						cerr << "Cell not empty at " << x << ", " << y << ", " << field << endl;
						cerr << "Cell has pointer to unit object at " << cell->getUnit(field) << endl;

						assert(false);
					}
				}
			}
		}
	}
}
#else
void World::assertConsistiency() {}
#endif

void World::doHackyCleanUp() {
	int h = map.getH();
	int w = map.getW();
	for(int x = 0; x < w; ++x) {
		for(int y = 0; y < h; ++y) {
			Cell *cell = map.getCell(x, y);
			for(Units::const_iterator u = newlydead.begin(); u != newlydead.end(); ++u) {
				for(int f = 0; f < fCount; ++f) {
					if(cell->getUnit((Zone)f) == *u) {
						cell->setUnit((Zone)f, NULL);
					}
				}
			}
		}
	}
	newlydead.clear();
}
#ifdef PATHFINDER_TIMING
PathFinderStats::PathFinderStats ()
{
   astar_calls = astar_avg = total_astar_calls = 
      total_astar_avg = total_path_recalcs = worst_astar = 
      lastsec_calls = lastsec_avg = calls_rejected = 0;
}

void PathFinderStats::resetCounters ()
{
   lastsec_calls = astar_calls;
   lastsec_avg = astar_avg;
   astar_calls = 0;
   astar_avg = 0;
}
char * PathFinderStats::GetStats ()
{
   sprintf ( buffer, "aaStar() Processed last second: %d, Average (micro-seconds): %d", (int)lastsec_calls, (int)lastsec_avg );
   return buffer;
}
char * PathFinderStats::GetTotalStats ()
{
   sprintf ( buffer, "aaStar() Total Calls: Processed: %d, Rejected: %d, Processed Call Average (micro-seconds): %d, Worst: %d.",
      (int)total_astar_calls, (int)calls_rejected, (int)total_astar_avg, (int)worst_astar );
   return buffer;
}
void PathFinderStats::AddEntry ( int64 ticks )
{
   if ( astar_calls )
      astar_avg = ( astar_avg * astar_calls + ticks ) / ( astar_calls + 1 );
   else
      astar_avg = ticks;
   astar_calls++;
   if ( total_astar_calls )
      total_astar_avg = ( total_astar_avg * total_astar_calls + ticks ) / ( total_astar_calls + 1 );
   else
      total_astar_avg = ticks;
   total_astar_calls++;

   if ( ticks > worst_astar ) worst_astar = ticks;
}
void PathFinder::DumpPath ( UnitPath *path, const Vec2i &start, const Vec2i &dest )
{
   static char buffer[1024*1024];
   char *ptr = buffer;
   ptr += sprintf ( buffer, "Start Pos: %d,%d Destination pos: %d,%d.\n",
      start.x, start.y, dest.x, dest.y );
   
   // output section of map
   Vec2i tl ( (start.x <= dest.x ? start.x : dest.x) - 4, 
              (start.y <= dest.y ? start.y : dest.y) - 4 );
   Vec2i br ( (start.x <= dest.x ? dest.x : start.x) + 4, 
              (start.y <= dest.y ? dest.y : start.y) + 4 );
   if ( tl.x < 0 ) tl.x = 0;
   if ( tl.y < 0 ) tl.y = 0;
   if ( br.x > map->getW()-2 ) br.x = map->getW()-2;
   if ( br.y > map->getH()-2 ) br.y = map->getH()-2;
   for ( int y = tl.y; y <= br.y; ++y )
   {
      for ( int x = tl.x; x <= br.x; ++x )
      {
         Vec2i pos (x,y);
         Unit *unit = map->getCell ( pos )->getUnit ( fSurface );
         Object *obj = map->getTile ( Map::toTileCoords (pos) )->getObject ();
         if ( pos == start )
            ptr += sprintf ( ptr, "S" );
         else if ( pos == dest )
            ptr += sprintf ( ptr, "D" );
         else if ( unit && unit->isMobile () ) 
            ptr += sprintf ( ptr, "U" );
         else if ( unit && ! unit->isMobile () ) 
            ptr += sprintf ( ptr, "B" );
         //else if ( obj && obj->getResource () )
         //   ptr += sprintf ( ptr, "R" );
         else if ( obj )//&& ! obj->getResource () )
            ptr += sprintf ( ptr, "X" );
         else
            ptr += sprintf ( ptr, "0" );
         if ( x != br.x ) ptr += sprintf ( ptr, "," );
      }
      ptr += sprintf ( ptr, "\n" );
   }
   Logger::getInstance ().add ( buffer );
   // output path
   if ( path ) Logger::getInstance ().add ( path->Output () );
}
void PathFinder::AssertValidPath ( AStarNode *node )
{
   while ( node->next )
   {
      Vec2i &pos1 = node->pos;
      Vec2i &pos2 = node->next->pos;
      if ( pos1.dist ( pos2 ) > 1.5 || pos1.dist ( pos2 ) < 0.9 )
      {
         Logger::getInstance.add ( "Invalid Path Generated..." );
         assert ( false );
      }
      node = node->next;
   }
}
#endif

}} //end namespace
