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

#include "pch.h"

#include <algorithm>

#include "search_engine.h"
#include "cluster_map.h"
#include "cartographer.h"

#include "debug_renderer.h"
#include "search_engine.h"

namespace Glest { namespace Game { namespace Search {

ClusterMap::ClusterMap(AnnotatedMap *aMap, Cartographer *carto) 
		: aMap(aMap) , carto(carto) {
	w = aMap->getWidth() / clusterSize;
	h = aMap->getHeight() / clusterSize;
	vertBorders = new ClusterBorder[(w-1)*h];
	horizBorders = new ClusterBorder[w*(h-1)];
	// init Borders (and hence inter-cluster edges) & evaluate clusters (intra-cluster edges)
	for (int i = h - 1; i >= 0; --i) {
		for (int j = w - 1; j >= 0; --j) {
			Vec2i cluster(j, i);
			initCluster(cluster);
			evalCluster(cluster);
		}
	}
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
}

/** initialise/re-initialise cluster (evaluates north and west borders) */
void ClusterMap::initCluster(Vec2i cluster) {
	ClusterBorder *west = getWestBorder(cluster);
	ClusterBorder *north = getNorthBorder(cluster);
	Transitions::iterator it;

	if (north != &sentinel) {
		int y = cluster.y * clusterSize - 1;
		int y2 = y + 1;
		bool clear = false;   // true while evaluating a Transition, false when obstacle hit
		int max_clear = -1;  // max clearance seen for current Transition
		int max_pos = -1;	// position of max clearance for current Transition
		int cx = (cluster.x + 1) * clusterSize - clusterSize / 2;
		for (Field f = enum_cast<Field>(0); f < Field::COUNT; ++f) {
			if (!aMap->maxClearance[f]) continue;
			deleteValues(north->transitions[f].begin(), north->transitions[f].end());
			north->transitions[f].clear();			
			clear = false;
			max_clear = -1;
			max_pos = -1;
			for (int i=0; i < clusterSize; ++i) {
				int clear1 = aMap->metrics[Vec2i(POS_X,y)].get(enum_cast<Field>(f));
				int clear2 = aMap->metrics[Vec2i(POS_X,y2)].get(enum_cast<Field>(f));
				int local = min(clear1, clear2);
				if (local) {
					clear = true;
					if (local > max_clear || ( local == max_clear && abs(POS_X - cx) < abs(max_pos - cx) )) {
						max_clear = local;
						max_pos = POS_X;
					} 
				} else {
					if (clear) {
						assert(max_clear > 0 && max_pos != -1);
						Transition *t = new Transition(Vec2i(max_pos,y), max_clear, false);
						north->transitions[f].push_back(t);
						max_clear = -1;
						max_pos = -1;
					}
					clear = false;
				}
			} // for i < clusterSize
			if (clear) {
				assert(max_clear > 0 && max_pos != -1 );
				Transition *t = new Transition(Vec2i(max_pos,y), max_clear, false);
				north->transitions[f].push_back(t);
			}
		}// for each Field
#		if _GAE_DEBUG_EDITION_
			Transitions::iterator it = north->transitions[Field::LAND].begin();
			for ( ; it != north->transitions[Field::LAND].end(); ++it) {
				PathfinderClusterOverlay::entranceCells.insert((*it)->nwPos);
			}
#		endif
	} // if not sentinel

	if (west != &sentinel) {
		int x = cluster.x * clusterSize - 1;
		int x2 = x + 1;
		bool clear = false;	  // true while evaluating an Transition, false when obstacle hit
		int max_clear = -1;  // max clearance seen for current Transition
		int max_pos = -1;	// position of max clearance for current Transition
		int cy = (cluster.y + 1) * clusterSize - clusterSize / 2;
		for (Field f = enum_cast<Field>(0); f < Field::COUNT; ++f) {
			if (!aMap->maxClearance[f]) continue;

			deleteValues(west->transitions[f].begin(), west->transitions[f].end());
			west->transitions[f].clear();			

			clear = false;
			max_clear = -1;
			max_pos = -1;
			for (int i=0; i < clusterSize; ++i) {
				int clear1 = aMap->metrics[Vec2i(x,POS_Y)].get(enum_cast<Field>(f));
				int clear2 = aMap->metrics[Vec2i(x2,POS_Y)].get(enum_cast<Field>(f));
				int local = min(clear1, clear2);
				if (local) {
					clear = true;
					if (local > max_clear || ( local == max_clear && abs(POS_Y - cy) < abs(max_pos - cy) )) {
						max_clear = local;
						max_pos = POS_Y;
					}
				} else {
					if (clear) {
						assert(max_clear > 0 && max_pos != -1 );
						Transition *t = new Transition(Vec2i(x,max_pos), max_clear, true);
						west->transitions[f].push_back(t);
						max_clear = -1;
						max_pos = -1;
					}
					clear = false;
				}
			} // for i < clusterSize
			if (clear) {
				assert(max_clear > 0 && max_pos != -1 );
				Transition *t = new Transition(Vec2i(x,max_pos), max_clear, true);
				west->transitions[f].push_back(t);
			}
		}// for each Field

#		if _GAE_DEBUG_EDITION_
			Transitions::iterator it = west->transitions[Field::LAND].begin();
			for ( ; it != west->transitions[Field::LAND].end(); ++it) {
				PathfinderClusterOverlay::entranceCells.insert((*it)->nwPos);
			}
#		endif
	} // if not sentinel

}

//bool showIntraClusterPaths = false;

float ClusterMap::aStarPathLength(Field f, int size, Vec2i &start, Vec2i &dest) {
	if (start == dest) {
		return 0.f;
	}
	SearchEngine<NodeMap,GridNeighbours>* se = carto->getSearchEngine();
	MoveCost costFunc(f, size, aMap);
	DiagonalDistance dd(dest);
	se->setNodeLimit(clusterSize * clusterSize);
	se->setStart(start, dd(start));
	AStarResult res = se->aStar<PosGoal,MoveCost,DiagonalDistance>(PosGoal(dest), costFunc, dd);
	Vec2i goalPos = se->getGoalPos();
	if (res != AStarResult::COMPLETE || goalPos != dest) {
		return numeric_limits<float>::infinity();
	}
#	if _GAE_DEBUG_EDITION_
		/*if (f == Field::LAND && size == 1 && showIntraClusterPaths) {
			Vec2i aPos = se->getPreviousPos(goalPos);
			while (aPos != start) {
				PathfinderClusterOverlay::pathCells.insert(aPos);
				aPos = se->getPreviousPos(aPos);
			}
		}*/
#	endif
	return se->getCostTo(goalPos);
}

void ClusterMap::getTransitions(Vec2i cluster, Field f, Transitions &t) {
	ClusterBorder *north = getNorthBorder(cluster);
	ClusterBorder *east = getEastBorder(cluster);
	ClusterBorder *south = getSouthBorder(cluster);
	ClusterBorder *west = getWestBorder(cluster);

	Transitions::iterator it;
	
	it = north->transitions[f].begin();
	for ( ; it != north->transitions[f].end(); ++it)
		t.push_back(*it);
	
	it = east->transitions[f].begin();
	for ( ; it != east->transitions[f].end(); ++it)
		t.push_back(*it);
	
	it = south->transitions[f].begin();
	for ( ; it != south->transitions[f].end(); ++it)
		t.push_back(*it);
	
	it = west->transitions[f].begin();
	for ( ; it != west->transitions[f].end(); ++it)
		t.push_back(*it);
}

ostream& operator<<(ostream &stream, const Vec2i &p) {
	return stream << "(" << p.x << ", " << p.y << ")";
}

/** compute intra-cluster path lengths */
void ClusterMap::evalCluster(Vec2i cluster) {
	/*if (cluster.x == 2 && cluster.y == 4) {
		showIntraClusterPaths = true;
	} else {
		showIntraClusterPaths = false;
	}*/

	GridNeighbours::setSearchCluster(cluster);
	Transitions transitions;
	for (Field f(0); f < Field::COUNT; ++f) {
		if (!aMap->maxClearance[f]) continue;
		transitions.clear();
		getTransitions(cluster, f, transitions);
		Transitions::iterator it = transitions.begin();
		for ( ; it != transitions.end(); ++it) {
			Transition *t = const_cast<Transition*>(*it);
			Vec2i start = t->nwPos;
			Transitions::iterator it2 = transitions.begin();
			for ( ; it2 != transitions.end(); ++it2) {
				Transition *t2 = const_cast<Transition*>(*it2);
				if (t == t2) continue;
				Vec2i dest = t2->nwPos;
				float cost  = aStarPathLength(f, 1, start, dest);
				if (cost == numeric_limits<float>::infinity()) continue;
				Edge *e = new Edge(t2);
				t->edges.push_back(e);
				e->second.push_back(cost);
				int size = 2;
				int maxClear = t->clearance > t2->clearance ? t2->clearance : t->clearance;
				while (size <= maxClear) {
					cost = aStarPathLength(f, size, start, dest);
					if (cost == numeric_limits<float>::infinity()) {
						break;
					}
					e->second.push_back(cost);
					assert(size == e->maxClear());
					++size;
				}
			}
		}
		/*
		if (showIntraClusterPaths) {
			cout << FieldNames[f] << " initialised ...\n";
			cout << "Transition at 32,63:\n";
			const Edges &edges = getNorthBorder(Vec2i(2,4))->transitions[Field::LAND][0]->edges;
			for (Edges::const_iterator it = edges.begin(); it != edges.end(); ++it) {
				Edge* const &e = *it;
				cout << "Edge to " << e->first->nwPos << " @ cost " << e->cost(1) << endl;
			}
			cout << endl;
		}
		*/
	} // for each Field
}

// ========================================================
// class TransitionNodeStorage
// ========================================================

TransitionAStarNode* TransitionNodeStore::getNode() {
	if (nodeCount == size) {
		//assert(false);
		return NULL;
	}
	return &stock[nodeCount++];
}

void TransitionNodeStore::insertIntoOpen(TransitionAStarNode *node) {
	if (openList.empty()) {
		openList.push_front(node);
		return;
	}
	list<TransitionAStarNode*>::iterator it = openList.begin();
	while (it != openList.end() && (*it)->est() <= node->est()) {
		++it;
	}
	openList.insert(it, node);
}

bool TransitionNodeStore::assertOpen() {
	if (openList.size() < 2) {
		return true;
	}
	set<const Transition*> seen;
	list<TransitionAStarNode*>::iterator it1, it2 = openList.begin();
	it1 = it2;
	seen.insert((*it1)->pos);
	for (++it2; it2 != openList.end(); ++it2) {
		if (seen.find((*it2)->pos) != seen.end()) {
			theLogger.add("open list has cycle... that's bad.");
			cout << "open list has cycle... that's bad." << endl;
			return false;
		}
		seen.insert((*it2)->pos);
		if ((*it1)->est() > (*it2)->est() + 0.0001f) { // stupid inaccurate fp
			theLogger.add("open list is not ordered correctly.");
			cout << "Open list corrupt: it1.est() == " << (*it1)->est() 
				<< " > it2.est() == " << (*it2)->est() << endl;
			return false;
		}
	}
	set<const Transition*>::iterator it = open.begin();
	for ( ; it != open.end(); ++it) {
		if (seen.find(*it) == seen.end()) {
			theLogger.add("node marked open not on open list.");
			cout << "node marked open not on open list." << endl;
			return false;
		}
	}
	it = seen.begin();
	for ( ; it != seen.end(); ++it) {
		if (open.find(*it) == open.end()) {
			theLogger.add("node on open list not marked open.");
			cout << "node on open list not marked open." << endl;
			return false;
		}
	}
	return true;
}

Transition* TransitionNodeStore::getBestSeen() {
	assert(false); 
	return NULL;
}

bool TransitionNodeStore::setOpen(const Transition* pos, const Transition* prev, float h, float d) {
	assert(open.find(pos) == open.end());
	assert(closed.find(pos) == closed.end());
	
	//REMOVE
	assert(assertOpen());
	
	TransitionAStarNode *node = getNode();
	if (!node) return false;
	node->pos = pos;
	node->prev = prev;
	node->distToHere = d;
	node->heuristic = h;
	open.insert(pos);
	insertIntoOpen(node);
	listed[pos] = node;
	
	//REMOVE
	assert(assertOpen());

	return true;
}

void TransitionNodeStore::updateOpen(const Transition* pos, const Transition* &prev, const float cost) {

	//REMOVE
	assert(assertOpen());

	TransitionAStarNode *prevNode = listed[prev];
	TransitionAStarNode *posNode = listed[pos];
	if (prevNode->distToHere + cost < posNode->distToHere) {
		openList.remove(posNode);
		posNode->prev = prev;
		posNode->distToHere = prevNode->distToHere + cost;
		insertIntoOpen(posNode);
	}

	//REMOVE
	assert(assertOpen());
}

const Transition* TransitionNodeStore::getBestCandidate() {
	if (openList.empty()) return NULL;
	TransitionAStarNode *node = openList.front();
	const Transition *best = node->pos;
	openList.pop_front();
	open.erase(open.find(best));
	closed.insert(best);
	return best;
}

}}}