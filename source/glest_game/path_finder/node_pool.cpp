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
// File: astar_nodepool.cpp
//
#include "pch.h"

#include "route_planner.h"
#include "node_pool.h"
#include "map.h"
#include "unit.h"

namespace Glest { namespace Game { namespace Search {

NodeStore::NodeStore() 
		: tmpMaxNodes(NodePool::size)
		, numNodes(0)
		, leastH(NULL)
		, markerArray(theMap.getW(), theMap.getH())
		, pointerArray(theMap.getW(), theMap.getH())
		, pool(NULL) {
	openHeap.reserve(512);
}

NodeStore::~NodeStore() {
}
/** reset the node pool for a new search (resets tmpMaxNodes too) */
void NodeStore::reset() {
	numNodes = 0;
	tmpMaxNodes = NodePool::size;
	leastH = NULL;
	markerArray.newSearch();
	openHeap.clear();
	if ( pool ) pool->reset();
#	if DEBUG_SEARCH_TEXTURES
		listedNodes.clear();
#	endif
}
/** set a maximum number of nodes to expand */
void NodeStore::setMaxNodes(const int max) {
	assert(max >= 32 && max <= 2048); // reasonable number ?
	assert(!numNodes); // can't do this after we've started using it.
	tmpMaxNodes = max;
}

/** marks an unvisited position as open
  * @param pos the position to open
  * @param prev the best known path to pos is from
  * @param h the heuristic for pos
  * @param d the costSoFar for pos
  * @return true if added, false if node limit reached
  */
bool NodeStore::setOpen(const Vec2i &pos, const Vec2i &prev, float h, float d) {
	assert(!isOpen(pos));
	assert(prev.x < 0 || isClosed(prev));
	AStarNode *node = pool->newNode();
	if ( !node ) { // NodePool exhausted
		return false;
	}
	node->posOff = pos;
	if ( prev.x >= 0 ) {
		node->posOff.ox = prev.x - pos.x;
		node->posOff.oy = prev.y - pos.y;
	} else {
		node->posOff.ox = 0;
		node->posOff.oy = 0;
	}
	node->distToHere = d;
	node->heuristic = h;
	addOpenNode(node);
	if ( !numNodes || h < leastH->heuristic ) {
			leastH = node;
	}
	numNodes++;
	return true;
}

/** add a new node to the open list 
  * pointer to the node to add
  */
void NodeStore::addOpenNode(AStarNode *node) {
	assert(!isOpen(node->pos()));
	markerArray.setOpen(node->pos());
	pointerArray.set(node->pos(), node);
	openHeap.push_back(node);
	push_heap(openHeap.begin(), openHeap.end(), AStarComp());
}
/** conditionally update a node on the open list. Tests if a path through a new nieghbour
  * is better than the existing known best path to pos, updates if so.
  * @param pos the open postion to test
  * @param prev the new path from
  * @param d the distance to here through prev
  */
void NodeStore::updateOpen(const Vec2i &pos, const Vec2i &prev, const float cost) {
	//assert(isClosed(prev));
	AStarNode *posNode, *prevNode;
	posNode = pointerArray.get(pos);
	prevNode = pointerArray.get(prev);
	if ( prevNode->distToHere + cost < posNode->distToHere ) {
		posNode->distToHere = prevNode->distToHere + cost;
		posNode->posOff.ox = prev.x - pos.x;
		posNode->posOff.oy = prev.y - pos.y;

		//stringstream str;
		//str << "open heap is " << openHeap.size() << " items, ";
		//int ndx = 0;
		vector<AStarNode*>::iterator it = find(openHeap.begin(), openHeap.end(), posNode);// openHeap.begin();
		/*
		if ( *it == posNode ) {
			return;
		}
		for ( ; it != openHeap.end(); ++it, ++ndx ) {
			if ( *it == posNode ) {
				str << "found openPos at index " << ndx;
				break;
			}
		}*/
		push_heap(openHeap.begin(), it + 1, AStarComp());
		/*
		for ( it = openHeap.begin(), ndx = 0; it != openHeap.end(); ++it, ++ndx ) {
			if ( *it == posNode ) {
				str << ", openPos now at index " << ndx;
				break;
			}
		}
		LOG(str.str());*/

		// We could just push_heap from begin to 'posNode' (as we're only decreasing key)
		// but we need a quick method to get an iterator to posNode...
		//FIXME: We should just manage the heap oursleves...
		//make_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
	}
}

/** @deprecated use getBestCandidate() */
AStarNode* NodeStore::getBestCandidateNode () {
	if ( openHeap.empty() ) {
		return NULL;
	}
	pop_heap(openHeap.begin(), openHeap.end(), AStarComp());
	AStarNode *ret = openHeap.back();
	openHeap.pop_back();
	markerArray.setClosed(ret->pos());
	return ret;
}

#if DEBUG_SEARCH_TEXTURES

list<Vec2i>* NodeStore::getOpenNodes() {
	list<Vec2i> *ret = new list<Vec2i>();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end (); ++it ) {
		if ( isOpen(*it) ) ret->push_back(*it);
	}
	return ret;
}

list<Vec2i>* NodeStore::getClosedNodes() {
	list<Vec2i> *ret = new list<Vec2i>();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end(); ++it ) {
		if ( isClosed(*it) ) ret->push_back(*it);
	}
	return ret;
}

#endif // DEBUG_SEARCH_TEXTURES

}}}