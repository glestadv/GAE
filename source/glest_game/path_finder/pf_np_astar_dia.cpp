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
// File: pf_np_astar_dia.cpp
//
#include "pch.h"

#include "pf_np_astar_dia.h"
#include "path_finder.h"
#include "map.h"

namespace Glest { namespace Game { namespace PathFinder {

// AStarNodePool

AStarNodePoolDIA::AStarNodePoolDIA ()
{
   Logger::getInstance ().add ( "AStarNodePoolDIA::AStarNodePoolDIA ()" );
   maxNodes = pathFindNodesMax;
   lists = new AStarNode*[maxNodes];
   reset ();
}

AStarNodePoolDIA::~AStarNodePoolDIA () 
{
   delete [] lists;
}
void AStarNodePoolDIA::init ( Map *map )
{
   Logger::getInstance ().add ( "AStarNodePoolDIA::init ()" );
   markerArray.init ( map->getW(), map->getH() );
}
// reset the node pool
void AStarNodePoolDIA::reset ()
{
   numOpen = numClosed = 0;
   tmpMaxNodes = maxNodes;
   markerArray.newSearch ();
}

void AStarNodePoolDIA::addOpenNode ( AStarNode *node )
{
   const int top = tmpMaxNodes - 1;
   if ( !numOpen ) lists[top] = node;
   else
   {  // find insert index
      const int openStart = tmpMaxNodes - numOpen - 1;
      int offset = openStart;

      while ( offset < top && lists[offset+1]->est() < node->est() ) 
         offset ++;

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
      lists[offset] = node;
   }
   markerArray.setOpen ( node->pos );
   numOpen ++;
}
// Moves the lowest heuristic node from open to closed and returns a 
// pointer to it, or NULL if there are no open nodes.
AStarNode* AStarNodePoolDIA::getBestCandidate ()
{
   if ( !numOpen ) return NULL;
   lists[numClosed] = lists[tmpMaxNodes - numOpen];
   markerArray.setClosed ( lists[numClosed]->pos );
   numOpen --;
   numClosed ++;
   return lists[numClosed-1];
}
void AStarNodePoolDIA::updateOpenNode ( const Vec2i &pos, AStarNode *curr, float cost )
{
   const int startOpen = tmpMaxNodes - numOpen;
   int ndx = tmpMaxNodes - numOpen;
   while ( ndx < tmpMaxNodes )
   {
      if ( lists[ndx]->pos == pos )
         break;;
      ndx ++;
   }
   // assume we found it...
   if ( curr->distToHere + cost < lists[ndx]->est() )
   {
      AStarNode *ptr = lists[ndx];
      ptr->distToHere = curr->distToHere + 1;
      ptr->prev = curr;
      // shuffle stuff ?
      while ( ndx > startOpen && ptr->est() < lists[ndx-1]->est() )
      {
         lists[ndx] = lists[ndx-1];
         ndx--;
      }
      lists[ndx] = ptr;
   }
}

#ifdef PATHFINDER_DEBUG_TEXTURES

list<Vec2i>* AStarNodePoolDIA::getOpenNodes ()
{
   list<Vec2i> *ret = new list<Vec2i> ();
   const int openStart = tmpMaxNodes - numOpen - 1;
   for ( int i = openStart; i < tmpMaxNodes; ++i )
   {
      ret->push_back ( lists[i]->pos );
   }
   return ret;
}

list<Vec2i>* AStarNodePoolDIA::getClosedNodes ()
{
   list<Vec2i> *ret = new list<Vec2i> ();
   for ( int i = 0; i < numClosed; ++i )
   {
      ret->push_back ( lists[i]->pos );
   }
   return ret;
}

#endif

}}}
