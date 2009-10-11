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

NodePool::NodePool() {
	stock = new AStarNode[pathFindNodesMax];
	numNodes = 0;
	tmpMaxNodes = pathFindNodesMax;
	leastH = NULL;
	openHeap.reserve( pathFindNodesMax );
	markerArray.init( theMap.getW(), theMap.getH() );
	pointerArray.init( theMap.getW(), theMap.getH() );
}

NodePool::~NodePool() {
	delete [] stock;
}

void NodePool::reset() {
	numNodes = 0;
	tmpMaxNodes = pathFindNodesMax;
	leastH = NULL;
	markerArray.newSearch();
	openHeap.clear();
#  if DEBUG_SEARCH_TEXTURES
	listedNodes.clear();
#  endif
}

void NodePool::setMaxNodes( const int max ) {
	//assert( max >= 50 && max <= pathFindNodesMax ); // reasonable number ?
	//assert( !numNodes ); // can't do this after we've started using it.
	tmpMaxNodes = max;
}

bool NodePool::addToOpen( AStarNode *prev, const Vec2i &pos, float h, float d, bool exp ) {
	if ( numNodes == tmpMaxNodes ) {
		return false;
	}
#  if DEBUG_SEARCH_TEXTURES
	listedNodes.push_back ( pos );
#  endif
//	stock[numNodes].next = NULL;
	stock[numNodes].prev = prev;
	stock[numNodes].pos = pos;
	stock[numNodes].distToHere = d;
	stock[numNodes].heuristic = h;
	stock[numNodes].exploredCell = exp;
	addOpenNode( &stock[numNodes] );
	if ( numNodes ) {
		if ( h < leastH->heuristic ) {
			leastH = &stock[numNodes];
		}
	}
	else {
		leastH = &stock[numNodes];
	}
	numNodes++;
	return true;
}

void NodePool::addOpenNode ( AStarNode *node ) {
	markerArray.setOpen ( node->pos );
	pointerArray.set ( node->pos, node );
	openHeap.push_back ( node );
	push_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
}

void NodePool::updateOpenNode ( const Vec2i &pos, AStarNode *neighbour, float cost ) {
	AStarNode *posNode = (AStarNode*)pointerArray.get ( pos );
	if ( neighbour->distToHere + cost < posNode->distToHere ) {
		posNode->distToHere = neighbour->distToHere + cost;
		posNode->prev = neighbour;

		// We could just push_heap from begin to 'posNode' (as we're only decreasing key)
		// but we need a quick method to get an iterator to posNode...
		//FIXME: We should just manage the heap oursleves...
		make_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
	}
}

NodePool::AStarNode* NodePool::getBestCandidateNode () {
	if ( openHeap.empty() ) return NULL;
	pop_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
	AStarNode *ret = openHeap.back();
	openHeap.pop_back ();
	markerArray.setClosed ( ret->pos );
	return ret;
}

#if DEBUG_SEARCH_TEXTURES

list<Vec2i>* NodePool::getOpenNodes () {
	list<Vec2i> *ret = new list<Vec2i> ();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end (); ++it ) {
		if ( isOpen ( *it ) ) ret->push_back ( *it );
	}
	return ret;
}

list<Vec2i>* NodePool::getClosedNodes () {
	list<Vec2i> *ret = new list<Vec2i> ();
	list<Vec2i>::iterator it = listedNodes.begin();
	for ( ; it != listedNodes.end (); ++it ) {
		if ( isClosed ( *it ) ) ret->push_back ( *it );
	}
	return ret;
}

#endif // DEBUG_SEARCH_TEXTURES

}}}