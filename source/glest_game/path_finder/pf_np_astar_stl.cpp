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
// File: pf_np_astar_stl.cpp
//
#include "pch.h"

#include "pf_np_astar_stl.h"
#include "logger.h"
#include "map.h"
#include "path_finder.h"

namespace Glest { namespace Game { namespace PathFinder {

/* using the member function est() chokes MSVC++ in debug mode... go figure...
bool compNodes ( const AStarNode *one, const AStarNode *two )
{
   if ( one->est () > two->est () ) 
      return true;
   if ( one->est () == two->est () )
   {
      if ( one->heuristic > two->heuristic )
         return true;
   }
   return false;
}*/
// MSVC++ friendly version... does its own sums...
bool AStarComp::operator () ( const AStarNode *one, const AStarNode *two ) const
{
   if ( one->distToHere + one->heuristic < two->distToHere + two->heuristic ) 
      return false;
   if ( one->distToHere + one->heuristic > two->distToHere + two->heuristic ) 
      return true;
   // tie, prefer closer to goal...
   if ( one->heuristic < two->heuristic )
      return false;
   if ( one->heuristic > two->heuristic )
      return true;
   // still tied... prefer nodes 'in line' with goal ???

   // just distinguish them somehow...
   return one < two;
}

AStarNodePoolSTL::AStarNodePoolSTL ()
{
   Logger::getInstance ().add ( "AStarNodePoolSTL::AStarNodePoolSTL ()" );
   openHeap.reserve ( pathFindNodesMax );
}
void AStarNodePoolSTL::init ( Map *map )
{
   Logger::getInstance ().add ( "AStarNodePoolSTL::init ()" );
   markerArray.init ( map->getW(), map->getH() );
   pointerArray.init ( map->getW(), map->getH() );
}
void AStarNodePoolSTL::reset ()
{
   AStarNodePool::reset ();
   markerArray.newSearch ();
   openHeap.clear ();

#ifdef PATHFINDER_DEBUG_TEXTURES
   listedNodes.clear ();
#endif
}

bool AStarNodePoolSTL::isOpen ( const Vec2i &pos )
{
   return markerArray.isOpen ( pos );//open.find ( pos ) != open.end ();
}

bool AStarNodePoolSTL::isClosed ( const Vec2i &pos )
{
   return markerArray.isClosed ( pos );//closed.find ( pos ) != closed.end ();
}

void AStarNodePoolSTL::addOpenNode ( AStarNode *node )
{
#ifdef PATHFINDER_DEBUG_TEXTURES
   listedNodes.push_back ( node->pos );
#endif
   //if ( open.find ( node->pos ) != open.end () ) throw new runtime_error ( "boo" );
   //open[node->pos] = node;
   markerArray.setOpen ( node->pos );
   pointerArray.set ( node->pos, node );
   openHeap.push_back ( node );
   push_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
}

void AStarNodePoolSTL::updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost )
{
   AStarNode *posNode = (AStarNode*)pointerArray.get ( pos );//open[pos];
   if ( neighbour->distToHere + cost < posNode->distToHere )
   {
      posNode->distToHere = neighbour->distToHere + cost;
      posNode->prev = neighbour;

      // We could just push_heap from begin to 'posNode' (as we're only decreasing key)
      // but we need a quick method to get an iterator to posNode...
      make_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
   }
}

#ifndef LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC

void AStarNodePoolSTL::updateClosedNode ( const Vec2i &pos, AStarNode *neighbour, float cost )
{
   AStarNode *posNode = closed[pos];
   if ( neighbour->distToHere + cost < posNode->distToHere )
   {
      posNode->distToHere = neighbour->distToHere + cost;
      posNode->prev = neighbour;
      addOpenNode ( posNode );
      map<Vec2i,AStarNode*>::iterator it = closed.find ( posNode->pos );
      //if ( it == closed.end () ) throw new runtime_error ("boo");
      closed.erase ( it );
   }
}

#endif

AStarNode* AStarNodePoolSTL::getBestCandidate ()
{
   if ( openHeap.empty() ) return NULL;
   pop_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
   AStarNode *ret = openHeap.back();
   openHeap.pop_back ();
   //map<Vec2i,AStarNode*>::iterator it = closed.find ( ret->pos );
   //if ( it != closed.end () ) throw new runtime_error ("boo");
   markerArray.setClosed ( ret->pos );
   //closed[ret->pos] = ret;
   //it = open.find ( ret->pos );
   //if ( it == open.end () ) throw new runtime_error ("boo");
   //open.erase ( ret->pos );
   return ret;
}

#ifdef PATHFINDER_DEBUG_TEXTURES

list<Vec2i>* AStarNodePoolSTL::getOpenNodes ()
{
   list<Vec2i> *ret = new list<Vec2i> ();
   /*
   map<Vec2i,AStarNode*>::iterator it = open.begin();
   for ( ; it != open.end (); ++it )
   {
      ret->push_back ( it->first );
   }
   return ret;*/
   list<Vec2i>::iterator it = listedNodes.begin();
   for ( ; it != listedNodes.end (); ++it )
   {
      if ( isOpen ( *it ) ) ret->push_back ( *it );
   }
   return ret;
}

list<Vec2i>* AStarNodePoolSTL::getClosedNodes ()
{
   list<Vec2i> *ret = new list<Vec2i> ();
   /*
   map<Vec2i,AStarNode*>::iterator it = closed.begin();
   for ( ; it != closed.end (); ++it )
   {
      ret->push_back ( it->first );
   }
   return ret;*/
   list<Vec2i>::iterator it = listedNodes.begin();
   for ( ; it != listedNodes.end (); ++it )
   {
      if ( isClosed ( *it ) ) ret->push_back ( *it );
   }
   return ret;

}

#endif

}}}
