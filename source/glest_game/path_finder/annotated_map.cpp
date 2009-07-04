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
#include "pch.h"

#include "annotated_map.h"
#include "graph_search.h"
#include "map.h"
#include "path_finder.h"

namespace Glest { namespace Game { namespace PathFinder {

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
   assert ( val <= AnnotatedMap::maxClearanceValue );
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

const int AnnotatedMap::maxClearanceValue = 3;

AnnotatedMap::AnnotatedMap ( Map *m )
{
   metrics = NULL;
   cMap = m;
   initMapMetrics ( cMap );
}

AnnotatedMap::~AnnotatedMap ()
{
   if ( metrics )
   {
      for ( int i=0; i < metricHeight; ++i ) 
         delete metrics[i];
      delete [] metrics;
   }
   metrics = NULL;
}

void AnnotatedMap::initMapMetrics ( Map *map )
{
   if ( metrics )
   {
      for ( int i=0; i < metricHeight; ++i ) 
         delete [] metrics[i];
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

// pos: location of object added/removed
// size: size of same object
// adding: true if the object has been added, false if it has been removed.
void AnnotatedMap::updateMapMetrics ( const Vec2i &pos, const int size, bool adding, Field field )
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
         for ( VLIt it = LeftList->begin (); it != LeftList->end (); ++it )
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
         for ( VLIt it = AboveList->begin (); it != AboveList->end (); ++it )
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
CellMetrics AnnotatedMap::computeClearances ( const Vec2i &pos )
{
   assert ( sizeof ( CellMetrics ) == 4 );

   CellMetrics clearances;
   Tile *container = cMap->getTile ( cMap->toTileCoords ( pos ) );
 
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
   if ( pos.x == cMap->getW() - 3 ) clearAir = 1;
   else if ( pos.x == cMap->getW() - 4 ) clearAir = 2;
   if ( pos.y == cMap->getH() - 3 ) clearAir = 1;
   else if ( pos.y == cMap->getH() - 4 && clearAir > 2 ) clearAir = 2;
   clearances.set ( mfAir, clearAir );
   
   // Amphibious
   int clearSurf = clearances.get ( mfWalkable );
   if ( clearances.get ( mfAnyWater ) > clearSurf ) clearSurf = clearances.get ( mfAnyWater );
   clearances.set ( mfAmphibious, clearSurf );

   // use previously calculated base fields to calc combinations ??

   return clearances;
}
// as above, make faster...
uint32 AnnotatedMap::computeClearance ( const Vec2i &pos, Field field )
{
   uint32 clearance = 0;
   Tile *container = cMap->getTile ( cMap->toTileCoords ( pos ) );
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

bool AnnotatedMap::canClear ( const Vec2i &pos, int clear, Field field )
{
   if ( clear > maxClearanceValue ) return false;

   // on map ?
   if ( pos.x + clear >= cMap->getW() - 2 || pos.y + clear >= cMap->getH() - 2 ) 
      return false;

   for ( int i=pos.y; i < pos.y + clear; ++i )
   {
      for ( int j=pos.x; j < pos.x + clear; ++j )
      {
         Vec2i checkPos ( j, i );
	      Cell *cell = cMap->getCell ( checkPos );
         Tile *sc = cMap->getTile ( cMap->toTileCoords ( checkPos ) );
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


bool AnnotatedMap::canOccupy ( const Vec2i &pos, int size, Field field ) const
{
   //assert ( pos.x >= 0 && pos.x < map->getW() && pos.y >= 0 && pos.y < map->getH() );
   assert ( cMap->isInside ( pos ) );
   return metrics[pos.y][pos.x].get ( field ) >= size ? true : false;
}

void AnnotatedMap::annotateLocal ( const Vec2i &pos, const int size, const Field field )
{
   const Vec2i *directions = NULL;
   int numDirs;
   if ( size == 1 ) 
   {
      directions = Directions;
      numDirs = 8;
   }
   else if ( size == 2 )
   {
      directions = DirectionsSize2;
      numDirs = 12;
   }
   else
      assert ( false );

   Zone zone = field == mfAir ? fAir : fSurface;
   for ( int i = 0; i < numDirs; ++i )
   {
      Vec2i aPos = pos + directions[i];
      if ( cMap->isInside (aPos) && ! cMap->getCell (aPos)->isFree (zone) )
      {  // remember the metric
         localAnnt[aPos] = metrics[aPos.y][aPos.x].get ( field );
         metrics[aPos.y][aPos.x].set ( field, 0 );
      }
   }
}

void AnnotatedMap::clearLocalAnnotations ( Field field )
{
   for ( map<Vec2i,uint32>::iterator it = localAnnt.begin (); it != localAnnt.end (); ++ it )
   {
      assert ( it->second <= maxClearanceValue );
      assert ( cMap->isInside ( it->first ) );
      metrics [ it->first.y ][ it->first.x ].set ( field, it->second );
   }
   localAnnt.clear ();
}

}}}
