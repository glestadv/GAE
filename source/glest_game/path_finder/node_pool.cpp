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
// File: node_pool.cpp
//
#include "pch.h"

#include "node_pool.h"

namespace Glest { namespace Game { namespace Search {

#if 0
// =====================================================
// 	class OpenList::Head
// =====================================================

/** get best node, precondition: list not empty */
AStarNode* OpenList::Head::getBest() {
	assert(start);
	AStarNode *ret = start->data;
	freeNodes.push_back(start);
	if ( start->next ) {
		start = start->next;
		start->prev = NULL;
	} else {
		start = end = NULL;
	}
	count--;
	return ret;
}

/** add directly to end of list, for pre-sorted data. precondition: list not full */
void OpenList::Head::addToEnd(AStarNode *node) {
	if ( freeNodes.empty() ) {
		throw runtime_error("bad programmer!");
	}
	Node *lNode = freeNodes.back();
	freeNodes.pop_back();
	lNode->data = node;
	count++;
	insertAtEnd(lNode);
}

/** Add a node, possibly pushing another out @return pointer to node pushed off list, or NULL */
AStarNode* OpenList::Head::add(AStarNode *node) {
	if ( freeNodes.empty() ) {
		// head list full, replace end
		assert(node->est() < end->data->est());
		AStarNode *junk = end->data;
		Node *nn = unlink(end);
		nn->data = node;
		Node *ptr = start;
		while ( ptr && ptr->data->est() < nn->data->est() ) {
			ptr = ptr->next;
		}
		if ( ptr ) {
			insertBefore(nn,ptr);
		} else {
			insertAtEnd(nn);
		}
		maxEstimate = end->data->est();
		return junk;
	} 
	Node *lNode = freeNodes.back();
	freeNodes.pop_back();
	lNode->data = node;
	count++;
	if ( !start ) {
		lNode->next = lNode->prev = NULL;
		start = end = lNode;
		return NULL;
	}
	Node *ptr = start;
	while ( node->est() > ptr->data->est() ) {
		if ( !ptr->next ) {
			insertAtEnd(lNode);
			maxEstimate = node->est();
			return NULL;
		}
		ptr = ptr->next;
	}
	// ptr points to node one past where we want this one
	insertBefore(lNode,ptr);
	return NULL;
}

bool OpenList::Head::adjust(AStarNode *node) {
	Node *ptr = end;
	while ( ptr ) {
		if ( ptr->data == node ) {
			if ( ptr->prev ) {
				if ( ptr->prev->data->est() > ptr->data->est() ) {
					Node *look = ptr->prev;
					unlink(ptr);
					while ( look->prev && look->prev->data->est() > ptr->data->est() ) {
						look = look->prev;
					}
					insertBefore(ptr, look);
				}
			}
			return true;
		}
		ptr = ptr->prev;
	}
	return false;
}

void OpenList::Head::insertBefore(Node *insert, Node *ref) {
	if ( ref->prev ) {
		insert->prev = ref->prev;
		ref->prev->next = insert;
	} else {
		start = insert;
		insert->prev = NULL;
	}
	insert->next = ref;
	ref->prev = insert;
}

void OpenList::Head::insertAtEnd(Node *insert) {
	insert->next = NULL;
	insert->prev = end;
	end = insert;
	if ( insert->prev ) {
		insert->prev->next = insert;
	} else {
		start = insert;
	}
}

OpenList::Head::Node* OpenList::Head::unlink(Node *node) {
	if ( node->prev) {
		node->prev->next = node->next;
	} else {
		start = node->next;
	}
	if ( node->next ) {
		node->next->prev = node->prev;
	} else {
		end = node->prev;
	}
	return node;
}

bool OpenList::Head::assertList() {
	set<Node*> seen;
	Node *ptr = start;
	while ( ptr ) {
		if ( seen.find(ptr) != seen.end() ) {
			return false; // cycle
		}
		seen.insert(ptr);
		if ( ptr->next ) {
			if ( ptr->data->est() > ptr->next->data->est() ) {
				return false; // not in order
			}
			if ( ptr->next->prev != ptr ) {
				return false; // inconsistant next/prev links
			}
		}
		ptr = ptr->next;
	}
	if ( count != seen.size() ) {
		return false; // count inconsistant
	}
	return true;
}

// =====================================================
// 	class OpenList
// =====================================================

/** refill the head list, <b>pre-condition:</b> head is empty, bucket is not */
void OpenList::fill_head() {
	assert(head.empty() && !bucket.empty());
	bucket.sort();
	int num = maxHeadSize;
	if ( bucket.size() < num ) num = bucket.size();
	for ( ; num; --num ) {
		AStarNode *node = bucket.back();
		bucket.pop_back();
		head.addToEnd(node);
	}
	// maintain bucket.minEstimate
	bucket.setMinEst(bucket.back()->est());
}

void OpenList::push(AStarNode *node) {
	if ( totalSize() < maxHeadSize && bucket.empty() ) {
		AStarNode *displaced = head.add(node);
		assert(!displaced);			
	} else {
		// if ( node->est() < maxInHead ) sort into head else throw in bucket
		if ( node->est() < head.maxEst() ) {
			AStarNode *displaced = head.add(node);
			if ( displaced ) {
				bucket.add(displaced);
			}
		} else {
			bucket.add(node);
		}
	}
}

/** @todo ADD CODE */
void OpenList::adjust(AStarNode *node, float costToHere) {
	if ( node->est() <= head.maxEst() ) {
		node->distToHere = costToHere;
		// probably in head... not definately
		// if in head, adjust and return
		if ( head.adjust(node) ) {
			return;
		}
		// else its in the bucket, fall through
	} else {
		node->distToHere = costToHere; // update
	}
	// in bucket
	if ( node->est() < head.maxEst() ) { // good enough to go in head ?
		AStarNode *disp = head.add(node);
		if ( disp ) {
			bucket.add(disp);
		}
	}
}

AStarNode* OpenList::pop() {
	if ( head.empty() ) {
		if ( bucket.empty() ) {
			return NULL;
		}
		fill_head();
	}
	return head.getBest();
}
#endif

// =====================================================
// 	class NodeStore
// =====================================================

NodeStore::NodeStore(int w, int h) 
		: tmpMaxNodes(NodePool::size)
		, numNodes(0)
		, leastH(NULL)
		, markerArray(w,h)
		, pointerArray(w,h)
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
//	assert(prev.x < 0 || isClosed(prev));
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
#if 0
	openList.push(node);
#else
	openHeap.push_back(node);
	push_heap(openHeap.begin(), openHeap.end(), AStarComp());
#endif
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
		posNode->posOff.ox = prev.x - pos.x;
		posNode->posOff.oy = prev.y - pos.y;
#if 0
		openList.adjust(posNode, prevNode->distToHere + cost);
#elif 0
		posNode->distToHere = prevNode->distToHere + cost;
		vector<AStarNode*>::iterator it = find(openHeap.begin(), openHeap.end(), posNode);// openHeap.begin();
		push_heap(openHeap.begin(), it + 1, AStarComp());
#else		
		posNode->distToHere = prevNode->distToHere + cost;
		make_heap ( openHeap.begin(), openHeap.end(), AStarComp() );
#endif
	}
}

/** @deprecated use getBestCandidate() */
AStarNode* NodeStore::getBestCandidateNode () {
#if 0
	AStarNode *ret = openList.pop();
	if ( ret ) {
		markerArray.setClosed(ret->pos());
	}
	return ret;
#else
	if ( openHeap.empty() ) {
		return NULL;
	}	
	pop_heap(openHeap.begin(), openHeap.end(), AStarComp());
	AStarNode *ret = openHeap.back();
	openHeap.pop_back();
	return ret;
#endif
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