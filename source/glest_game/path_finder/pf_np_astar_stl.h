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
// File: pf_np_astar_stl.h
//

#include "pf_datastructs.h"
#include "pf_nodepool.h"

namespace Glest { namespace Game { namespace PathFinder {


class AStarComp
{
public:
   bool operator () ( const AStarNode *one, const AStarNode *two ) const;
};

class AStarNodePoolSTL : public AStarNodePool
{
public:
   AStarNodePoolSTL ();
   virtual ~AStarNodePoolSTL () {};

   void init ( Map *map );

   // reset everything
   virtual void reset ();

   // Is this pos already listed?
   virtual bool isOpen ( const Vec2i &pos );
   virtual bool isClosed ( const Vec2i &pos );
  
   // conditionaly update node at 'pos' (known to be open) with path through neighbour
   virtual void updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost );
//#ifndef LOW_LEVEL_SEARCH_ADMISSABLE_HEURISTIC
   // conditionaly update node at 'pos' (known to be closed) with path through neighbour & re-open if updated
   //virtual void updateClosedNode ( const Vec2i &pos, AStarNode *neighbour, float cost );
//#endif
   // moves 'best' node from open to closed, and returns it, or NULL if open is empty
   virtual AStarNode* getBestCandidate ();

#ifdef PATHFINDER_DEBUG_TEXTURES
   virtual list<Vec2i>* getOpenNodes ();
   virtual list<Vec2i>* getClosedNodes ();
   list<Vec2i> listedNodes;
#endif

private:
   //map<Vec2i, AStarNode*> open;
   //map<Vec2i, AStarNode*> closed;
   DoubleMarkerArray markerArray;
   PointerArray pointerArray;
   vector<AStarNode*> openHeap;
   //priority_queue<AStarNode*, vector<AStarNode*>, &compNodes> openHeap;

   virtual void addOpenNode ( AStarNode *node );
};

}}}