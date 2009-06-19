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
#include "path_finder.h"

#include <algorithm>
#include <cassert>
#include <iostream>

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

inline uint32 CellMetrics::get ( const Field field )
{
   switch ( field )
   {
   case mfWalkable: return field0;
   case mfAir: return field1;
   case mfAnyWater: return field2;
   case mfDeepWater: return field3;
   case mfAmphibious: return field4;
   default: throw new runtime_error ( "Unknown Field passed to CellMetrics::get()" );
   }
   return 0;
}

inline void CellMetrics::set ( const Field field, uint32 val )
{
   assert ( val <= PathFinder::maxClearanceValue );
   switch ( field )
   {
   case mfWalkable: field0 = val; return;
   case mfAir: field1 = val; return;
   case mfAnyWater: field2 = val; return;
   case mfDeepWater: field3 = val; return;
   case mfAmphibious: field4 = val; return;
   default: throw new runtime_error ( "Unknown Field passed to CellMetrics::set()" );
   }

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
         throw new runtime_error ("Invalid Path Generated.");
      node = node->next;
   }
}
#endif
// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ========================

const int PathFinder::maxFreeSearchRadius = 10;
const int PathFinder::maxClearanceValue = 3;

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
   metrics = NULL;
   singleton = this;
}

PathFinder::PathFinder(Map *map)
{
   PathFinder();
	init(map);
}

PathFinder::~PathFinder()
{
   if ( metrics )
   {
      for ( int i=0; i < metricHeight; ++i ) 
         delete metrics[i];
      delete [] metrics;
   }
}

void PathFinder::init(Map *map){
	this->map= map;
   nPool.markerArray.init ( map->getW(), map->getH() );
   // insert code here
   // I inserted it in its own method, initMapMetrics (), so it can be 
   // called after the initial buildings have been placed... 
}
//
// Is called from World::initUnits().
void PathFinder::initMapMetrics ()
{
   if ( metrics )
   {
      for ( int i=0; i < metricHeight; ++i ) 
         delete metrics[i];
      delete [] metrics;
   }
   metrics = new CellMetrics* [map->getH()];
   for ( int i=0; i < map->getH(); ++i )
   {
      metrics[i] = new CellMetrics [map->getW()];
      for ( int j=0; j < map->getW(); ++j )
      {
         Vec2i pos( j, i );
         metrics[i][j] = computeClearances ( pos );
      }
   }
   metricHeight = map->getH();
}

typedef list<Vec2i>::iterator VListIt;
// pos: location of object added/removed
// size: size of same object
// adding: true if the object has been added, false if it has been removed.
void PathFinder::updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field )
{
   // first, re-evaluate the cells occupied (or formerly occupied)
   for ( int i=0; i < size; ++i )
      for ( int j=0; j < size; ++j )
         metrics[pos.y + j][pos.x + i].set ( field,
            computeClearance ( Vec2i (pos.x + i, pos.y + j), field ) );

   // now, look to the left and above those cells, 
   // updating clearances where needed...
   list<Vec2i> *LeftList, *AboveList;
   LeftList = new list<Vec2i> ();
   AboveList = new list<Vec2i> ();
   // the cell to the nothwest...
   Vec2i *corner = NULL;
   if ( pos.x-1 >= 0 && pos.y-1 >= 0 ) corner = &Vec2i(pos.x-1,pos.y-1);

   for ( int i = 0; i < size; ++i )
   {
      // Check if positions are on map, (the '+i' components are 
      // along the sides of the building/object, so we assume 
      // they are ok). If so, list them
      if ( pos.x-1 >= 0 ) LeftList->push_back ( Vec2i (pos.x-1,pos.y+i) );
      if ( pos.y-1 >= 0 ) AboveList->push_back ( Vec2i (pos.x+i,pos.y-1) );
   }
   // This counts how far away from the new/old object we are, and is used 
   // to update the clearances without a costly call to ComputClearance() 
   // (if we're 'adding' that is)
   uint32 shell = 1;
   while ( !LeftList->empty() || !AboveList->empty() || corner )
   {
      // the left and above lists for the next loop iteration
      list<Vec2i> *newLeftList, *newAboveList;
      newLeftList = new list<Vec2i> ();
      newAboveList = new list<Vec2i> ();

      if ( !LeftList->empty() )
      {
         for ( VListIt it = LeftList->begin (); it != LeftList->end (); ++it )
         {
            // if we're adding and the current metric is bigger 
            // than shell, we need to update it
            if ( adding && metrics[it->y][it->x].get(field) > shell )
            {
               // BUG: Does not handle cell maps properly ( shell + metric[y][x+shell].fieldX ??? )
               metrics[it->y][it->x].set ( field, shell );
               if ( it->x - 1 >= 0 ) // if there is a cell to the left, add it to
                  newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
            }
            // if we're removing and the metrics of the cell to the right has a 
            // larger metric than this cell, we _may_ need to update this cell.
            else if ( !adding && metrics[it->y][it->x].get(field) < metrics[it->y][it->x+1].get(field) )
            {
               uint32 old = metrics[it->y][it->x].get(field);
               metrics[it->y][it->x].set ( field, computeClearance ( *it, field ) ); // re-compute
               // if we changed the metric and there is a cell to the left, add it to the 
               if ( metrics[it->y][it->x].get(field) != old && it->x - 1 >= 0 ) // new left list
                  newLeftList->push_back ( Vec2i(it->x-1,it->y) );
               // if we didn't change the metric, no need to keep looking left
            }
            // else we didn't need to update this metric, and cells to the left
            // will therefore also still have correct metrics and we can stop
            // looking along this row (it->y).
         }
      }
      // Do the equivalent for the above list
      if ( !AboveList->empty() )
      {
         for ( VListIt it = AboveList->begin (); it != AboveList->end (); ++it )
         {
            if ( adding && metrics[it->y][it->x].get(field) > shell )
            {
               metrics[it->y][it->x].set ( field,  shell );
               if ( it->y - 1 >= 0 )
                  newAboveList->push_back ( Vec2i(it->x,it->y-1) );
            }
            else if ( !adding && metrics[it->y][it->x].get(field) < metrics[it->y-1][it->x].get(field) )
            {
               uint32 old = metrics[it->y][it->x].get(field);
               metrics[it->y][it->x].set ( field, computeClearance ( *it, field ) );
               if ( metrics[it->y][it->x].get(field) != old && it->y - 1 >= 0 ) 
                  newAboveList->push_back ( Vec2i(it->x,it->y-1) );
            }
         }
      }
      if ( corner )
      {
         // Deal with the corner...
         int x = corner->x, y  = corner->y;
         if ( adding && metrics[y][x].get(field) > shell ) // adding ?
         {
            metrics[y][x].set ( field, shell ); // update
            // add cell left to the new left list, cell above to new above list, and
            // set corner to point at the next corner (x-1,y-1) (if they're on the map.)
            if ( x - 1 >= 0 )
            {
               newLeftList->push_back ( Vec2i(x-1,y) );
               if ( y - 1 >= 0 ) corner = &Vec2i(x-1,y-1);
               else corner = NULL;
            }
            else corner = NULL;
            if ( y - 1 >= 0 ) newAboveList->push_back ( Vec2i(x,y-1) );
         }
         else if ( !adding && metrics[y][x].get(field) < metrics[y+1][x+1].get(field) ) // removing
         {
            uint32 old = metrics[y][x].get(field);
            metrics[y][x].set (field, computeClearance ( *corner, field ) );
            if ( metrics[y][x].get(field) != old ) // did we update ?
            {
               if ( x - 1 >= 0 )
               {
                  newLeftList->push_back ( Vec2i(x-1,y) );
                  if ( y - 1 >= 0 ) corner = &Vec2i(x-1,y-1);
                  else corner = NULL;
               }
               else corner = NULL;
               if ( y - 1 >= 0 ) newAboveList->push_back ( Vec2i(x,y-1) );
            }
            else // no update, stop looking at corners
               corner = NULL;
         }
         else // no update
            corner = NULL;
      }
      delete LeftList; LeftList = newLeftList;
      delete AboveList; AboveList = newAboveList;
      shell++;
   }// end while
   delete LeftList;
   delete AboveList;
}

// this could be made much faster pretty easily... 
// make it so...
// if cell is an obstacle in field, clearance = 0. done.
// else...
//   let clearance = the south-east metric, 
//   if the east metric < clearance let clearance = east metric
//   if the south metric < clearance let clearance = south metric
//   clearance++. done.
CellMetrics PathFinder::computeClearances ( const Vec2i &pos )
{
   assert ( sizeof ( CellMetrics ) == 4 );

   CellMetrics clearances;
   Tile *container = map->getTile ( map->toTileCoords ( pos ) );
 
   if ( container->isFree () ) // Tile is walkable?
   {
      // Walkable
      while ( canClear ( pos, clearances.get ( mfWalkable ) + 1, mfWalkable ) )
         clearances.set ( mfWalkable, clearances.get ( mfWalkable ) + 1 );
      // Any Water
      while ( canClear ( pos, clearances.get ( mfAnyWater ) + 1, mfAnyWater ) )
         clearances.set ( mfAnyWater, clearances.get ( mfAnyWater ) + 1 );
      // Deep Water
      while ( canClear ( pos, clearances.get ( mfDeepWater ) + 1, mfDeepWater ) )
         clearances.set ( mfDeepWater, clearances.get ( mfDeepWater ) + 1 );
   }
   
   // Air
   int clearAir = maxClearanceValue;
   if ( pos.x == map->getW() - 3 ) clearAir = 1;
   else if ( pos.x == map->getW() - 4 ) clearAir = 2;
   if ( pos.y == map->getH() - 3 ) clearAir = 1;
   else if ( pos.y == map->getH() - 4 && clearAir > 2 ) clearAir = 2;
   clearances.set ( mfAir, clearAir );
   
   // Amphibious
   int clearSurf = clearances.get ( mfWalkable );
   if ( clearances.get ( mfAnyWater ) > clearSurf ) clearSurf = clearances.get ( mfAnyWater );
   clearances.set ( mfAmphibious, clearSurf );

   // use previously calculated base fields to calc combinations ??

   return clearances;
}
// as above, make faster...
uint32 PathFinder::computeClearance ( const Vec2i &pos, Field field )
{
   uint32 clearance = 0;
   Tile *container = map->getTile ( map->toTileCoords ( pos ) );
   switch ( field )
   {
   case mfWalkable:
      if ( container->isFree () ) // surface cell is walkable?
         while ( canClear ( pos, clearance + 1, mfWalkable ) )
            clearance++;
      return clearance;
   case mfAir:
      return maxClearanceValue;
   case mfAnyWater:
      if ( container->isFree () )
         while ( canClear ( pos, clearance + 1, mfAnyWater ) )
            clearance++;
      return clearance;
   case mfDeepWater:
      if ( container->isFree () )
         while ( canClear ( pos, clearance + 1, mfDeepWater ) )
            clearance++;
      return clearance;
   case mfAmphibious:
      return maxClearanceValue;
   default:
      throw new runtime_error ( "Illegal Field passed to PathFinder::computeClearance()" );
      return 0;
   }
}

bool PathFinder::canClear ( const Vec2i &pos, int clear, Field field )
{
   if ( clear > maxClearanceValue ) return false;

   // on map ?
   if ( pos.x + clear >= map->getW() - 2 || pos.y + clear >= map->getH() - 2 ) 
      return false;

   for ( int i=pos.y; i < pos.y + clear; ++i )
   {
      for ( int j=pos.x; j < pos.x + clear; ++j )
      {
         Vec2i checkPos ( j, i );
	      Cell *cell = map->getCell ( checkPos );
         Tile *sc = map->getTile ( map->toTileCoords ( checkPos ) );
         switch ( field )
         {
         case mfWalkable:
            if ( ( cell->getUnit(fSurface) && !cell->getUnit(fSurface)->isMobile() )
            ||   !sc->isFree () || cell->isDeepSubmerged () ) 
               return false;
            break;
         case mfAnyWater:
            if ( ( cell->getUnit(fSurface) && !cell->getUnit(fSurface)->isMobile() )
            ||   !sc->isFree () || !cell->isSubmerged () )
               return false;
            break;
         case mfDeepWater:
            if ( ( cell->getUnit(fSurface) && !cell->getUnit(fSurface)->isMobile() )
            ||   !sc->isFree () || !cell->isDeepSubmerged () ) 
               return false;
            break;
         }
      }// end for
   }// end for
   return true;
}

bool PathFinder::isLegalMove ( Unit *unit, const Vec2i &pos2 ) const
{
   if ( unit->getPos().dist ( pos2 ) > 1.5 ) return false; // shouldn't really need this....

   const Vec2i &pos1 = unit->getPos ();
   const int &size = unit->getSize ();
   const Field &field = unit->getCurrField ();
   Zone cellField = field == mfAir ? fAir : fSurface;
   Tile *sc = map->getTile ( map->toTileCoords ( pos2 ) );

   if ( ! canOccupy ( pos2, size, field ) )
      return false;
   if ( pos1.x != pos2.x && pos1.y != pos2.y )
   {  // Proposed move is diagonal, check if cells either 'side' are free.
      Vec2i diag1, diag2;
      getPassthroughDiagonals ( pos1, pos2, size, diag1, diag2 );
      if ( ! canOccupy (diag1, 1, field) || ! canOccupy (diag2, 1, field) ) 
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
#ifdef PATHFINDER_TIMING
   int64 time = Chrono::getCurMicros ();
#endif
	const Vec2i targetPos = computeNearestFreePos ( unit, finalPos );
/*
   static char buffer[256];
   int off = sprintf ( buffer, "Start Pos: %d,%d", unit->getPos().x, unit->getPos().y );
   off += sprintf ( buffer + off, " finalPos: %d,%d", finalPos.x, finalPos.y );
   off += sprintf ( buffer + off, " targetPos: %d,%d", targetPos.x, targetPos.y );
   off += sprintf ( buffer + off, " metric at target: %d",
      PathFinder::getInstance()->metrics[targetPos.y][targetPos.x].get ( mfWalkable ) );
   Logger::getInstance ().add ( buffer );
*/
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
      if ( ! canPathOut ( targetPos, radius, mfWalkable ) ) 
      {
         unit->getPath()->incBlockCount ();
         unit->setCurrSkill(scStop);
#ifdef PATHFINDER_TIMING
         statNew.IncReject ();
#endif
         return tsBlocked;
      }
   }
   nPool.reset ();
   // dynamic adjustment of nodeLimit, based on distance to target
   if ( dist < 5 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 8 );      // == 100 nodes
   else if ( dist < 10 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 4 );// == 200 nodes
   else if ( dist < 15 ) nPool.setMaxNodes ( nPool.getMaxNodes () / 2 );// == 400 nodes
   // else a fixed -100, so the backward run has more nodes,
   // and if the forward run hits the nodeLimit, the backward run is
   // more likely to succeed.
   else nPool.setMaxNodes ( nPool.getMaxNodes () - 100 );// == 700 nodes

   list<Vec2i> forward, backward, cross;
   AAStarParams params (unit);
   params.dest = targetPos;
   if ( ! aaStar ( params, forward ) )
   {
		path->incBlockCount();
      return tsBlocked;
   }
   if ( dist > 15 )
   {
      nPool.reset ();// == 800 nodes
      params.dest = params.start;
      params.start = forward.back(); // is not necessarily targetPos, ie. if nodeLimit was hit
      
      aaStar ( params, backward, false );
/*#ifdef PATHFINDER_DEBUG_TEXTURES
      map->ClearPathPos ();
      map->PathStart = forward.front();
      map->PathDest = forward.back();
      for ( VListIt it = forward.begin(); it != forward.end(); ++it )
         if ( *it != forward.front() && *it != forward.back() ) map->SetPathPos ( *it );
      for ( VListIt it = backward.begin(); it != backward.end(); ++it )
         if ( *it != backward.front() && *it != backward.back() ) map->SetPathPos ( *it, true );
#endif*/

      if ( backward.back() == forward.front() ) // aaStar() doesn't guarantee 'symmetrical success'
      {
         getCrossOverPoints ( forward, backward, cross );
         MergePath ( forward, backward, cross, unit->getPath () );
      }
      else
      {
         // Shouldn't happen too often now the backward run is gauranteed at least
         // 100 more nodes than the foward run, but is inevitable in some cases
         Logger::getInstance().add ( "MergePath() Failed." );
         copyToPath ( forward, unit->getPath () );
      }
   }
   else
      copyToPath ( forward, unit->getPath () );

#ifdef PATHFINDER_DEBUG_TEXTURES
   map->ClearPathPos ();
   if ( cross.size() )
   {
      map->PathStart = cross.front();
      map->PathDest = cross.back();
      for ( VListIt it = cross.begin(); it != cross.end(); ++it )
         map->SetPathPos ( *it );
   }
   else
   {
      map->PathStart = forward.front();
      map->PathDest = forward.back();
      for ( VListIt it = forward.begin(); it != forward.end(); ++it )
         map->SetPathPos ( *it );
   }
#endif 
	Vec2i pos= path->pop();//crash point
   if ( ! isLegalMove ( unit, pos ) )
   {
		unit->setCurrSkill(scStop);
      unit->getPath()->incBlockCount ();
		return tsBlocked;
	}
   unit->setNextPos(pos);
#ifdef PATHFINDER_TIMING
      statNew.AddEntry ( Chrono::getCurMicros () - time );
#endif
	return tsOnTheWay;
}

// ==================== PRIVATE ====================
//
// Take a start pos instead of unit, return a UnitPath? Nope, take a reference to it..
// move all post-processing to findPath()
//
// route a unit using an Annotated (Least Heuristic First) A* Algorithm 
bool PathFinder::aaStar ( AAStarParams params, list<Vec2i> &path, bool ucStart )
{
   Vec2i &startPos = params.start;
   Vec2i &finalPos = params.dest;
   int &size = params.size;
   int &team = params.team;
   Field &field = params.field;
   Zone zone = field == mfAir ? fAir : fSurface;
   
   path.clear ();

	//a) push starting pos into openNodes
   nPool.addToOpen ( NULL, startPos, heuristic (startPos, finalPos) );

   //b) loop
	bool pathFound= true;
	bool nodeLimitReached= false;
	AStarNode *minNode= NULL;

   Vec2i ucPos = ucStart ? startPos : finalPos;
   while( ! nodeLimitReached ) 
   {
      minNode = nPool.getBestCandidate ();
      if ( ! minNode ) // open was empty?
         return false;

      if ( minNode->pos == finalPos || ! minNode->exploredCell )
			break;

		for(int i=-1; i<=1 && !nodeLimitReached; ++i)
      {
			for(int j=-1; j<=1 && !nodeLimitReached; ++j)
         {
            if ( ! (i||j) ) // i==j==0 == minNode, minor performance hack, 
               continue;   // wouldn't get 'picked up' until nPool.isListed() otherwise...
            Vec2i sucPos = minNode->pos + Vec2i(i, j);
            if ( ! map->isInside ( sucPos ) ) 
               continue;
            if ( minNode->pos.dist ( ucPos ) < 5.f && !map->getCell( sucPos )->isFree(zone) && sucPos != ucPos )
               continue; // check nearby units
            // CanOccupy () will be cheapest, do it first...
#ifdef PATHFINDER_NODEPOOL_USE_MARKER_ARRAY
            if ( canOccupy (sucPos, size, field ) && ! nPool.isListedMarker ( sucPos ) )
#else
            if ( canOccupy (sucPos, size, field ) && ! nPool.isListedTree ( sucPos ) )
#endif
            {
               if ( minNode->pos.x != sucPos.x && minNode->pos.y != sucPos.y ) 
               {  // if diagonal move and either diag cell is not free...
                  Vec2i diag1, diag2;
                  getPassthroughDiagonals ( minNode->pos, sucPos, size, diag1, diag2 );
                  if ( !canOccupy (diag1, 1, field) || !canOccupy (diag2, 1, field) 
                  ||  (minNode->pos.dist(ucPos) < 5.f  && !map->getCell (diag1)->isFree (zone) && diag1 != ucPos)
                  ||  (minNode->pos.dist(ucPos) < 5.f  && !map->getCell (diag2)->isFree (zone) && diag2 != ucPos) )
                     continue; // not allowed
               }
               // else move is legal.
               bool exp = map->getTile (Map::toTileCoords (sucPos))->isExplored (team);
               if ( ! nPool.addToOpen ( minNode, sucPos, heuristic ( sucPos, finalPos ), exp ) )
                  nodeLimitReached = true;
            }
			} // end for
		} // end for ... inner loop
	} // end while ... outer loop
	AStarNode *lastNode= minNode;
   // if ( nodeLimtReached ) iterate over closed list, testing for a lower h node ...

   //if ( nodeLimitReached ) Logger::getInstance ().add ( "Node Limit Exceeded." );
	// on the way
   // fill in next pointers
	AStarNode *currNode = lastNode;
   int steps = 0;
	while ( currNode->prev )
   {
		currNode->prev->next = currNode;
		currNode = currNode->prev;
      steps++;
	}
   AStarNode *firstNode = currNode;

   //store path
	currNode = firstNode;//->next; // don't store start pos
   while ( currNode ) 
   {
      path.push_back ( currNode->pos );
      currNode = currNode->next;
   }
   return true;
}

void PathFinder::getCrossOverPoints ( const list<Vec2i> &forward, const list<Vec2i> &backward, list<Vec2i> &result )
{
   result.clear ();
   nPool.reset ();
#ifdef PATHFINDER_NODEPOOL_USE_MARKER_ARRAY
   for ( list<Vec2i>::const_reverse_iterator it1 = backward.rbegin(); it1 != backward.rend(); ++it1 )
      nPool.addMarker ( *it1 );
   for ( list<Vec2i>::const_iterator it2 = forward.begin(); it2 != forward.end(); ++it2 )
      if ( nPool.isListedMarker ( *it2 ) )
         result.push_back ( *it2 );
#else
   for ( list<Vec2i>::const_reverse_iterator it1 = backward.rbegin(); it1 != backward.rend(); ++it1 )
      nPool.addToTree ( *it1 );
   for ( list<Vec2i>::const_iterator it2 = forward.begin(); it2 != forward.end(); ++it2 )
      if ( nPool.isListedTree ( *it2 ) )
         result.push_back ( *it2 );
#endif
}

void PathFinder::copyToPath ( const list<Vec2i> pathList, UnitPath *path )
{
   list<Vec2i>::const_iterator it = pathList.begin();
   // skip start pos, store rest
   for ( ++it; it != pathList.end(); ++it )
      path->push ( *it );
}

bool PathFinder::MergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, UnitPath *path )
{
   list<Vec2i> endPath;
   assert ( co.size () <= fwd.size () );
   if ( fwd.size () == co.size () ) 
   {  // paths never diverge
      copyToPath ( fwd, path );
      return true;
   }
   list<Vec2i>::const_iterator fIt = fwd.begin ();
   list<Vec2i>::const_reverse_iterator bIt = bwd.rbegin ();
   list<Vec2i>::iterator coIt = co.begin ();

   //the 'first' and 'last' nodes on fwd and bwd must be the first and last on co...
   //assert ( *coIt == *fIt && *coIt == *bIt );
   assert ( co.back() == fwd.back() && co.back() == bwd.front() );
   if ( ! ( *coIt == *fIt && *coIt == *bIt ) )
   {
      //throw new runtime_error ( "PathFinder::MergePath() was passed dodgey data..." );
      copyToPath ( fwd, path );
      return false;
   }
   
   ++fIt; ++bIt;
   // Should probably just iterate over co aswell...
   coIt = co.erase ( coIt );

   while ( coIt != co.end () )
   {
      // coIt now points to a pos that is common to both paths, but isn't the start... 
      // skip any more duplicates, putting them onto the path
      while ( *coIt == *fIt )
      {
         path->push ( *coIt );
#ifdef PATHFINDER_DEBUG_TEXTURES
         endPath.push_back ( *coIt );
#endif
         //assert ( *fIt == *bIt );
         if ( *fIt != *bIt )
         {
            //throw new runtime_error ( "PathFinder::MergePath() was passed a dodgey crossover list..." );
            path->clear ();
            copyToPath ( fwd, path );
            return false;
         }
         coIt = co.erase ( coIt );
         if ( coIt == co.end() ) 
#ifdef PATHFINDER_DEBUG_TEXTURES
            goto CopyToList; // done
#else
            return true;
#endif
         ++fIt; ++bIt;
      }
      // coIt now points to the next common pos, 
      // fIt and bIt point to the positions where the path's have just diverged
      int fGap = 0, bGap = 0;
      list<Vec2i>::const_iterator fStart = fIt; // save our spot
      list<Vec2i>::const_reverse_iterator bStart = bIt; // ditto
      while ( *fIt != *coIt ) { fIt ++; fGap ++; }
      while ( *bIt != *coIt ) { bIt ++; bGap ++; }
      if ( bGap < fGap )
      {
         // copy section from bwd
         while ( *bStart != *coIt )
         {
#ifdef PATHFINDER_DEBUG_TEXTURES
            endPath.push_back ( *bStart );
#endif
            path->push ( *bStart );
            bStart ++;
         }
      }
      else
      {
         // copy section from fwd
         while ( *fStart != *coIt )
         {
#ifdef PATHFINDER_DEBUG_TEXTURES
            endPath.push_back ( *fStart );
#endif
            path->push ( *fStart );
            fStart ++;
         }
      }
      // now *fIt == *bIt == *coIt... skip duplicates, etc etc...
   }
#ifdef PATHFINDER_DEBUG_TEXTURES
   // A kludge, to get at the endpath easier for debugging... UnitPath hides everything...
CopyToList:
   for ( list<Vec2i>::iterator it = endPath.begin(); it != endPath.end(); ++it )
   {
      co.push_back ( *it );
   }
#endif
}

inline void PathFinder::getPassthroughDiagonals ( const Vec2i &s, const Vec2i &d, 
                                             const int size, Vec2i &d1, Vec2i &d2 ) const
{
   if ( size == 1 )
   {
      d1.x = s.x; d1.y = d.y;
      d2.x = d.x; d2.y = s.y;
      return;
   }
   if ( d.x > s.x )
   {  // travelling east
      if ( d.y > s.y )
      {  // se
         d1.x = d.x + size - 1; d1.y = s.y;
         d2.x = s.x; d2.y = d.y + size - 1;
      }
      else
      {  // ne
         d1.x = s.x; d1.y = d.y;
         d2.x = d.x + size - 1; d2.y = s.y - size + 1;
      }
   }
   else
   {  // travelling west
      if ( d.y > s.y )
      {  // sw
         d1.x = d.x; d1.y = s.y;
         d2.x = s.x + size - 1; d2.y = d.y + size - 1;
      }
      else
      {  // nw
         d1.x = d.x; d1.y = s.y - size + 1;
         d2.x = s.x + size - 1; d2.y = d.y;
      }
   }
}


bool PathFinder::canOccupy ( const Vec2i &pos, int size, Field field ) const
{
   //assert ( pos.x >= 0 && pos.x < map->getW() && pos.y >= 0 && pos.y < map->getH() );
   assert ( map->isInside ( pos ) );
   return metrics[pos.y][pos.x].get ( field ) >= size ? true : false;
}

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

bool PathFinder::canPathOut ( const Vec2i &pos, const int radius, Field field  )
{
   assert ( radius > 0 && radius <= 5 );
   nPool.reset ();
   nPool.addToOpen ( NULL, pos, 0 );
	bool pathFound= false;
   AStarNode *maxNode = NULL;

   while( ! pathFound ) 
   {
      maxNode = nPool.getBestCandidate ();
      if ( ! maxNode ) break; // failure
		for ( int i = -1; i <= 1 && ! pathFound; ++i )
      {
			for ( int j = -1; j <= 1 && ! pathFound; ++j )
         {
            if ( ! (i||j) ) continue;
            Vec2i sucPos = maxNode->pos + Vec2i(i, j);
            if ( ! map->isInside ( sucPos ) 
            ||   ! map->getCell( sucPos )->isFree( field == mfAir ? fAir: fSurface ) )
               continue;
            //CanOccupy() will be cheapest, do it first...
#ifdef PATHFINDER_NODEPOOL_USE_MARKER_ARRAY
            if ( canOccupy (sucPos, 1, field) && ! nPool.isListedMarker (sucPos) )
#else
            if ( canOccupy (sucPos, 1, field) && ! nPool.isListedTree (sucPos) )
#endif
            {
               if ( maxNode->pos.x != sucPos.x && maxNode->pos.y != sucPos.y ) // if diagonal move
               {
                  Vec2i diag1 ( maxNode->pos.x, sucPos.y );
                  Vec2i diag2 ( sucPos.x, maxNode->pos.y );
                  // and either diag cell is not free...
                  if ( !canOccupy ( diag1, 1, field ) || !canOccupy ( diag2, 1, field )
                  ||   ! map->getCell( diag1 )->isFree( field == mfAir ? fAir: fSurface ) 
                  ||   ! map->getCell( diag2 )->isFree( field == mfAir ? fAir: fSurface ) )
                     continue; // not allowed
               }
               // Move is legal.
               if ( -(maxNode->heuristic) + 1 >= radius ) 
                  pathFound = true;
               else
                  nPool.addToOpen ( maxNode, sucPos, maxNode->heuristic - 1.f );
				} // end if
			} // end for
		} // end for
	} // end while
   return pathFound;
}
PathFinder::AAStarParams::AAStarParams ( Unit *u ) 
{
   start = u->getPos(); 
   field = u->getCurrField ();
   size = u->getSize (); 
   team = u->getTeam ();
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

PathFinder::NodePool::NodePool ()
{
   maxNodes = PathFinder::pathFindNodesMax;
   stock = new AStarNode[maxNodes];
   lists = new AStarNode*[maxNodes];
   posStock = new PosTreeNode[maxNodes];
   sentinel.colour = Black;
   sentinel.left = sentinel.right = sentinel.parent = NULL;
   sentinel.pos = Vec2i(-1,-1);
   posRoot = NULL;
#ifdef PATHFINDER_TREE_TIMING
   useSet = false;
   numRuns = 4;
   curRun = 0;
#endif
   reset ();
}

PathFinder::NodePool::~NodePool () 
{
   delete stock;
   delete lists;
   delete posStock;
}

// reset the node pool
void PathFinder::NodePool::reset ()
{
#ifndef PATHFINDER_TREE_TIMING
   assert ( assertTreeValidity() ); // DO NOT do this if timing.
   posRoot = NULL;
#else
   if ( useSet && posSet.size () )
   {
      curRun ++;
      posSet.clear ();
      setStats.newTree ();
      if ( curRun % numRuns == 0 ) useSet = !useSet;
   }
   else if ( !useSet && posRoot )
   {
      curRun ++;
      posRoot = NULL;
      rbStats.newTree ();
      if ( curRun % numRuns == 0 ) useSet = !useSet;
   }
#endif
   numOpen = numClosed = numTotal = numPos = 0;
   tmpMaxNodes = maxNodes;
   markerArray.newSearch ();
}

void PathFinder::NodePool::setMaxNodes ( const int max )
{
   //assert ( max >= 50 && max <= maxNodes ); // reasonable number ?
   //if ( numTotal ) // can't do this after we've started using it.
   //   throw new runtime_error ( "setMaxNodes() called while NodePool was in use." );
   tmpMaxNodes = max;
}

//
// A regular tree insert, the new node is coloured explicitly
// if it's the root it's coloured black and we're done, otherwise the new node is coloured
// Red, inserted then passed to rebalance() to fix any Red-Black property violations
//
void PathFinder::NodePool::addToTree ( const Vec2i &pos )
{
#ifdef PATHFINDER_TREE_TIMING
   int64 start = Chrono::getCurTicks ();
   if ( useSet ) 
   {
      posSet.insert ( pos );
      setStats.addInsert ( Chrono::getCurTicks () - start );
      return;
   }
#endif
   posStock[numPos].pos = pos;
   posStock[numPos].left = posStock[numPos].right = &sentinel;
   
   if ( ! numPos )
   {
      posRoot = &posStock[0];
      posStock[0].parent = &sentinel;
      posStock[0].colour = Black;
   }
   else
   {
      PathFinder::PosTreeNode *parent = posRoot;
      posStock[numPos].colour = Red;
      while ( true )
      {
         if ( pos.x < parent->pos.x 
         ||   ( pos.x == parent->pos.x && pos.y < parent->pos.y ) )
         {  // go left
            if ( parent->left == &sentinel )
            {  // insert and break
               parent->left = &posStock[numPos];
               posStock[numPos].parent = parent;
               rebalanceTree ( &posStock[numPos] );
               break;
            }
            else parent = parent->left;
         }
         else if ( pos.x > parent->pos.x 
         ||   ( pos.x == parent->pos.x && pos.y > parent->pos.y ) )
         {  // go right
            if ( parent->right == &sentinel )
            {  // insert and break
               parent->right = &posStock[numPos];
               posStock[numPos].parent = parent;
               rebalanceTree ( &posStock[numPos] );
               break;
            }
            else parent = parent->right;
         }
         else // pos == parent->pos ... Error
            throw new runtime_error ( "Duplicate position added to PathFinder::NodePool." );
      }
   }
   numPos++;
#ifdef PATHFINDER_TREE_TIMING
   rbStats.addInsert ( Chrono::getCurTicks () - start );
#endif
}

#define DAD(x) (x->parent)
#define GRANDPA(x) (x->parent->parent)
#define UNCLE(x) (x->parent->parent?x->parent->parent->left==x->parent\
                  ?x->parent->parent->right:x->parent->parent->left:NULL)
//
// Restore Red-Black properties on PosTree, with 'node' having just been inserted
//
void PathFinder::NodePool::rebalanceTree ( PosTreeNode *node )
{
   while ( DAD(node)->colour == Red )
   {
      if ( UNCLE(node)->colour == Red )
      {
         // case 'Red Uncle', colour flip dad, uncle, and grandpa then restart with grandpa
         DAD(node)->colour = UNCLE(node)->colour = Black;
         GRANDPA(node)->colour = Red;
         assert ( GRANDPA(node) != &sentinel );
         node = GRANDPA(node);
      }
      else // UNCLE(node) is black
      {
         if ( DAD(node) == GRANDPA(node)->left )
         {
            // dad red left child, uncle black
            if ( node == DAD(node)->right )
            {
               node = DAD(node);
               rotateLeft ( node );
            }
            DAD(node)->colour = Black;
            GRANDPA(node)->colour = Red;
            assert ( GRANDPA(node) != &sentinel );
            rotateRight ( GRANDPA(node) ); 
         }
         else // Dad is right child of grandpa
         {
            // dad red right child, uncle black
            PosTreeNode *prevDad = DAD(node), *prevGrandpa = GRANDPA(node);
            if ( node == DAD(node)->left )
            {
               node = DAD(node);
               rotateRight ( node );
            }
            DAD(node)->colour = Black;
            GRANDPA(node)->colour = Red;
            assert ( GRANDPA(node) != &sentinel );
            rotateLeft ( GRANDPA(node) );
         } // end if..else, dad left or right child of grandpa
      } // end if..else, uncle red or black
   } // end while, dad red
   posRoot->colour = Black; // node is new root
}

#if defined(DEBUG) || defined(_DEBUG)
bool PathFinder::NodePool::assertTreeValidity ()
{
   if ( !posRoot ) return true;
   Vec2i low = Vec2i ( -1,-1 );
   // Inorder traversal, ref [Algorithm T, Knuth Vol 1 pg 317]
   // T1 [Initialize]
   list<PosTreeNode*> stack; // stupid VC++ doesn't have stack...
   PosTreeNode *ptr = posRoot;
   while ( true )
   {
      // T2 [if P == NULL goto T4]
      if ( ptr != &sentinel ) 
      {
         // T3 [stack.push(P), P = P->left, goto T2]
         stack.push_back ( ptr );
         ptr = ptr->left;
         continue;
      }
      // T4 [if stack.empty() terminate, else P = stack.pop()]
      if ( stack.empty () ) 
         break;
      ptr = stack.back ();
      stack.pop_back ();
      //
      // T5 [Visit P, P = P->right, goto T2]
      if ( ptr->colour == Red && ( ptr->left->colour == Red || ptr->right->colour == Red ) )
      {
         Logger::getInstance ().add ( "Red-Black Tree Invalid, Red-Black Property violated." );
         dumpTree ();
         return false;
      }
      if ( ptr->pos.x < low.x || ( ptr->pos.x == low.x && ptr->pos.y < low.y ) )  
      {
         Logger::getInstance ().add ( "Search Tree Invalid! Elements out of order." );
         dumpTree ();
         return false;
      }
      low = ptr->pos;
      ptr = ptr->right;
   }
   return true;
}
void PathFinder::NodePool::dumpTree ()
{
   static char buf[1024*4];
   char *ptr = buf;
   if ( !posRoot )
   {
      Logger::getInstance ().add ( "Tree Empty." );
      return;
   }
   list<PosTreeNode*> *thisLevel = new list<PosTreeNode*>();
   list<PosTreeNode*> *nextLevel = NULL;
   thisLevel->push_back ( posRoot );
   while ( ! thisLevel->empty () )
   {
      ptr += sprintf ( ptr, "\n" );
      nextLevel = new list<PosTreeNode*>();
      for ( list<PosTreeNode*>::iterator it = thisLevel->begin(); it != thisLevel->end(); ++it )
      {
         ptr += sprintf ( ptr, "[%d,%d|", (*it)->pos.x, (*it)->pos.y );
         if ( (*it)->left )
         {
            ptr += sprintf ( ptr, "L:%d,%d|", (*it)->left->pos.x, (*it)->left->pos.y );
            nextLevel->push_back ( (*it)->left );
         }
         else ptr += sprintf ( ptr, "L:NIL|" );
         if ( (*it)->right )
         {
            ptr += sprintf ( ptr, "R:%d,%d|", (*it)->right->pos.x, (*it)->right->pos.y );
            nextLevel->push_back ( (*it)->right );
         }
         else ptr += sprintf ( ptr, "R:NIL|" );
         ptr += sprintf ( ptr, "%s] ", (*it)->colour ? "Black" : "Red" );

      }
      delete thisLevel;
      thisLevel = nextLevel;
   }
   delete thisLevel;
   Logger::getInstance ().add ( buf );
}
#endif

// Tree rotations, ref [Cormen, 13.2]
#define ROTATE_ERR_MSG "PathFinder::NodePool::rotateLeft() was called on a node with no right child."
void PathFinder::NodePool::rotateLeft ( PosTreeNode *root )
{
   PosTreeNode *pivot = root->right; // 1
   assert ( pivot != &sentinel );
   //if ( pivot == &sentinel ) 
   //   throw new runtime_error ( ROTATE_ERR_MSG );
   root->right = pivot->left;  // 2
   root->right->parent = root; // 3
   pivot->parent = root->parent; // 4
   root->parent = pivot; // 11
   pivot->left = root; // 10
   if ( pivot->parent != &sentinel ) // 5
   {
      if ( pivot->parent->left == root ) // 7
         pivot->parent->left = pivot; // 8
      else
         pivot->parent->right = pivot; // 9
   }
   else posRoot = pivot; // 6
}
#define ROTATE_ERR_MSG "PathFinder::NodePool::rotateRight() was called on a node with no left child."
void PathFinder::NodePool::rotateRight ( PosTreeNode *root )
{
   PosTreeNode *pivot = root->left;
   assert ( pivot != &sentinel );
   //if ( pivot == &sentinel ) 
   //   throw new runtime_error ( ROTATE_ERR_MSG );
   root->left = pivot->right;
   root->left->parent = root;
   pivot->parent = root->parent;
   root->parent = pivot;
   pivot->right = root;
   if ( pivot->parent != &sentinel)
   {
      if ( pivot->parent->left == root )
         pivot->parent->left = pivot;
      else
         pivot->parent->right = pivot;
   }
   else posRoot = pivot;
}
#undef ROTATE_ERR_MSG

// is pos already listed?
bool PathFinder::NodePool::isListedTree ( const Vec2i &pos ) 
#ifndef PATHFINDER_TREE_TIMING  
const
#endif
{
#ifdef PATHFINDER_TREE_TIMING
   int64 start = Chrono::getCurTicks ();
   if ( useSet )
   {
      set<Vec2i>::iterator it = posSet.find ( pos );
      setStats.addSearch ( Chrono::getCurTicks () - start );
      if ( it == posSet.end() ) return false;
      return true;
   }
#endif
   PathFinder::PosTreeNode *ptr = posRoot;
   while ( ptr != &sentinel)
   {
      if ( pos.x < ptr->pos.x )
         ptr = ptr->left;
      else if ( pos.x > ptr->pos.x )
         ptr = ptr->right;
      else //  pos.x == ptr->pos.x 
      {
         if ( pos.y < ptr->pos.y )
            ptr = ptr->left;
         else if ( pos.y > ptr->pos.y ) 
            ptr = ptr->right;
         else // pos == ptr->pos
#ifdef PATHFINDER_TREE_TIMING  
         {
            rbStats.addSearch ( Chrono::getCurTicks () - start );
            return true;
         }
#else
            return true;
#endif
      }
   }
#ifdef PATHFINDER_TREE_TIMING
   rbStats.addSearch ( Chrono::getCurTicks () - start );
#endif
   return false;
}

// Add pos to the open list
//#ifdef PATHFINDER_TIMING
//bool PathFinder::NodePool::addToOpen ( AStarNode* prev, const Vec2i &pos, void *h, float d, bool exp )
//#else
bool PathFinder::NodePool::addToOpen ( AStarNode* prev, const Vec2i &pos, float h, bool exp )
//#endif
{
   if ( numTotal == tmpMaxNodes ) 
      return false;
   stock[numTotal].next = NULL;
   stock[numTotal].prev = prev;
   stock[numTotal].pos = pos;/*
#ifdef PATHFINDER_TIMING
   if ( heuristicFlipper )
      stock[numTotal].heuristic.i = *(int*)(h);
   else
      stock[numTotal].heuristic.f = *(float*)(h);
#else*/
   stock[numTotal].heuristic = h;
//#endif
   stock[numTotal].exploredCell = exp;
   const int top = tmpMaxNodes - 1;
   if ( !numOpen ) lists[top] = &stock[numTotal];
   else
   {  // find insert index
      // due to the nature of the modified A*, new nodes are likely to have lower heuristics
      // than the majority already in open, so we start checking from the low end.
      const int openStart = tmpMaxNodes - numOpen - 1;
      int offset = openStart;
      /*
#ifdef PATHFINDER_TIMING
      if ( heuristicFlipper )
         while ( offset < top && lists[offset+1]->heuristic.i < stock[numTotal].heuristic.i ) 
            offset ++;
      else
         while ( offset < top && lists[offset+1]->heuristic.f < stock[numTotal].heuristic.f ) 
            offset ++;
#else
            */
      while ( offset < top && lists[offset+1]->heuristic < stock[numTotal].heuristic ) 
         offset ++;
//#endif
      if ( offset > openStart ) // shift lower nodes down...
      {
         int moveNdx = openStart;
         while ( moveNdx <= offset )
         {
            lists[moveNdx-1] = lists[moveNdx];
            moveNdx ++;
         }
      }
      // insert newbie in sorted pos.
      lists[offset] = &stock[numTotal];
   }
#ifdef PATHFINDER_NODEPOOL_USE_MARKER_ARRAY
   addMarker ( pos );
#else
   addToTree ( pos );
#endif
   numTotal ++;
   numOpen ++;
   return true;
}

// Moves the lowest heuristic node from open to closed and returns a 
// pointer to it, or NULL if there are no open nodes.
PathFinder::AStarNode* PathFinder::NodePool::getBestCandidate ()
{
   if ( !numOpen ) return NULL;
   lists[numClosed] = lists[tmpMaxNodes - numOpen];
   numOpen --;
   numClosed ++;
   return lists[numClosed-1];
}
/*
bool PathFinder::NodePool::addToOpenHD ( AStarNode* prev, const Vec2i &pos, float d )
{
   if ( numTotal == tmpMaxNodes ) 
      return false;
   stock[numTotal].next = NULL;
   stock[numTotal].prev = prev;
   stock[numTotal].pos = pos;
   stock[numTotal].distToHere = d;
   stock[numTotal].exploredCell = true;
   const int top = tmpMaxNodes - 1;
   if ( !numOpen ) lists[top] = &stock[numTotal];
   else
   {
      const int openStart = tmpMaxNodes - numOpen - 1;
      int offset = openStart;
      while ( offset < top && lists[offset+1]->distToHere > stock[numTotal].distToHere ) offset ++;

      if ( offset > openStart ) // shift higher distance nodes down...
      {
         int moveNdx = openStart;
         while ( moveNdx <= offset )
         {
            lists[moveNdx] = lists[moveNdx+1];
            moveNdx ++;
         }
      }
     // insert newbie in sorted pos.
      lists[offset] = &stock[numTotal];
   }
   addToTree ( pos );
   numTotal ++;
   numOpen ++;
   return true;
}*/
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

}} //end namespace
