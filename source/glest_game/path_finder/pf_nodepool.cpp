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
// File: pf_nodepool.cpp
//
// Low Level Search Routines and additional support structures
//
#include "pch.h"

#include "path_finder.h"
#include "pf_nodepool.h"
#include "map.h"
#include "unit.h"

namespace Glest { namespace Game { namespace PathFinder {

   
SearchParams::SearchParams ( Unit *u ) 
{
   start = u->getPos(); 
   field = u->getCurrField ();
   size = u->getSize (); 
   team = u->getTeam ();
}

AStarNodePool::AStarNodePool ()
{
   Logger::getInstance ().add ( "AStarNodePool::AStarNodePool ()" );
   stock = new AStarNode[pathFindNodesMax];
   numNodes = 0;
   tmpMaxNodes = pathFindNodesMax;
}

AStarNodePool::~AStarNodePool ()
{
   delete [] stock;
}

void AStarNodePool::reset ()
{
   numNodes = 0;
   tmpMaxNodes = pathFindNodesMax;
}

void AStarNodePool::setMaxNodes ( const int max )
{
   assert ( max >= 50 && max <= pathFindNodesMax ); // reasonable number ?
   assert ( !numNodes ); // can't do this after we've started using it.
   tmpMaxNodes = max;
}

bool AStarNodePool::addToOpen ( AStarNode* prev, const Vec2i &pos, float h, float d, bool exp )
{
   if ( numNodes == tmpMaxNodes ) 
      return false;
   stock[numNodes].next = NULL;
   stock[numNodes].prev = prev;
   stock[numNodes].pos = pos;
   stock[numNodes].distToHere = d;
   stock[numNodes].heuristic = h;
   stock[numNodes].exploredCell = exp;
   this->addOpenNode ( &stock[numNodes] );
   numNodes++;
   return true;
}

#ifdef PATHFINDER_TIMING

PathFinderStats::PathFinderStats ( char *name )
{
   assert ( strlen ( name ) < 31 );
   if ( name ) strcpy ( prefix, name );
   else strcpy ( prefix, "??? : " );
   num_searches = num_searches_last_interval = 
      worst_search = calls_rejected = num_searches_this_interval = 0;
   search_avg = search_avg_last_interval = search_avg_this_interval = 0.0;
}

void PathFinderStats::resetCounters ()
{
   num_searches_last_interval = num_searches_this_interval;
   search_avg_last_interval = search_avg_this_interval;
   num_searches_this_interval = 0;
   search_avg_this_interval = 0.0;
}
char * PathFinderStats::GetStats ()
{
   sprintf ( buffer, "%s Processed last interval: %d, Average (micro-seconds): %g", prefix,
      (int)num_searches_last_interval, search_avg_last_interval );
   return buffer;
}
char * PathFinderStats::GetTotalStats ()
{
   sprintf ( buffer, "%s Total Searches: %d, Search Average (micro-seconds): %g, Worst: %d.", prefix,
      (int)num_searches, search_avg, (int)worst_search );
   return buffer;
}
void PathFinderStats::AddEntry ( int64 ticks )
{
   if ( num_searches_this_interval )
      search_avg_this_interval = ( search_avg_this_interval * (double)num_searches_this_interval + (double)ticks ) 
                                 / (double)( num_searches_this_interval + 1 );
   else
      search_avg_this_interval = (double)ticks;
   num_searches_this_interval++;
   if ( num_searches )
      search_avg  = ( (double)num_searches * search_avg + (double)ticks ) / (double)( num_searches + 1 );
   else
      search_avg = (double)ticks;
   num_searches++;

   if ( ticks > worst_search ) worst_search = ticks;
}
#endif // defined ( PATHFINDER_TIMING )


}}}