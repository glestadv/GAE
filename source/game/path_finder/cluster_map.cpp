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
		: aMap(aMap) , carto(carto), dirty(false) {
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


int ClusterMap::eClear[clusterSize];

void ClusterMap::addNorthTransition(ClusterBorder *cb, Field f, int y, int max_clear, int startPos, int endPos, int run) {
	assert(max_clear > 0 && startPos != -1 && endPos != -1);
	if (run < 12) {
		// find central most pos with max clearance
		int cp = (startPos + endPos) / 2;
		int incr = 0;
		bool minus = true;
		int n = 0;
		int tp = cp;
		while (n < run) {
			if (eClear[startPos - tp] == max_clear) {
				Transition *t = new Transition(Vec2i(tp,y), max_clear, false);
				cb->transitions[f].push_back(t);
				return;
			}
			if (minus) {
				++incr;
				tp = cp + incr;
				minus = false;
			} else {
				tp = cp - incr;
				minus = true;
			}
			++n;
		}
		assert(false);
	} else {
		// find two points, as close as possible to 1/4 and 3/4 accross entrance
		int crp = endPos + (startPos - endPos) / 4;
		int clp = endPos + (startPos - endPos) * 3 / 4;
		// right-centre
		int incr = 0;
		bool minus = true;
		int tp = crp;
		while (tp <= startPos) {
			if (eClear[startPos - tp] == max_clear) {
				Transition *t = new Transition(Vec2i(tp,y), max_clear, false);
				cb->transitions[f].push_back(t);
				break;
			}
			if (minus) {
				++incr;
				tp = crp + incr;
				minus = false;
			} else {
				tp = crp - incr;
				minus = true;
			}
		}
		// left-centre
		incr = 0;
		minus = true;
		tp = clp;
		while (tp >= endPos) {
			if (eClear[startPos - tp] == max_clear) {
				Transition *t = new Transition(Vec2i(tp,y), max_clear, false);
				cb->transitions[f].push_back(t);
				break;
			}
			if (minus) {
				++incr;
				tp = clp + incr;
				minus = false;
			} else {
				tp = clp - incr;
				minus = true;
			}
		}
	}
}

void ClusterMap::addWestTransition(ClusterBorder *cb, Field f, int x, int max_clear, int startPos, int endPos, int run) {
	assert(max_clear > 0 && startPos != -1 && endPos != -1);
	if (run < 12) {
		// find central most pos with max clearance
		int cp = (startPos + endPos) / 2;
		int incr = 0;
		bool minus = true;
		int n = 0;
		int tp = cp;
		while (n < run) {
			if (eClear[startPos - tp] == max_clear) {
				Transition *t = new Transition(Vec2i(x, tp), max_clear, true);
				cb->transitions[f].push_back(t);
				return;
			}
			if (minus) {
				++incr;
				tp = cp + incr;
				minus = false;
			} else {
				tp = cp - incr;
				minus = true;
			}
			++n;
		}
		assert(false);
	} else {
		// find two points, as close as possible to 1/4 and 3/4 accross entrance
		int cbp = endPos + (startPos - endPos) / 4;
		int ctp = endPos + (startPos - endPos) * 3 / 4;
		// bottom-centre
		int incr = 0;
		bool minus = true;
		int tp = cbp;
		while (tp <= startPos) {
			if (eClear[startPos - tp] == max_clear) {
				Transition *t = new Transition(Vec2i(x,tp), max_clear, true);
				cb->transitions[f].push_back(t);
				break;
			}
			if (minus) {
				++incr;
				tp = cbp + incr;
				minus = false;
			} else {
				tp = cbp - incr;
				minus = true;
			}
		}
		// top-centre
		incr = 0;
		minus = true;
		tp = ctp;
		while (tp >= endPos) {
			if (eClear[startPos - tp] == max_clear) {
				Transition *t = new Transition(Vec2i(x,tp), max_clear, true);
				cb->transitions[f].push_back(t);
				break;
			}
			if (minus) {
				++incr;
				tp = ctp + incr;
				minus = false;
			} else {
				tp = ctp - incr;
				minus = true;
			}
		}

	}
}

void ClusterMap::initNorthBorder(const Vec2i &cluster) {
	ClusterBorder *north = getNorthBorder(cluster);
	if (north != &sentinel) {
		int y = cluster.y * clusterSize - 1;
		int y2 = y + 1;
		bool clear = false;   // true while evaluating a Transition, false when obstacle hit
		int max_clear = -1;  // max clearance seen for current Transition
		int startPos = -1; // start position of entrance
		int endPos = -1;  // end position of entrance
		int run = 0;	 // to count entrance 'width'
		for (Field f(0); f < Field::COUNT; ++f) {
			if (!aMap->maxClearance[f]) continue;
			deleteValues(north->transitions[f].begin(), north->transitions[f].end());
			north->transitions[f].clear();			
			clear = false;
			max_clear = -1;
			for (int i=0; i < clusterSize; ++i) {
				int clear1 = aMap->metrics[Vec2i(POS_X,y)].get(f);
				int clear2 = aMap->metrics[Vec2i(POS_X,y2)].get(f);
				int local = min(clear1, clear2);
				if (local) {
					if (!clear) {
						clear = true;
						startPos = POS_X;
					}
					eClear[run++] = local;
					endPos = POS_X;
					if (local > max_clear) {
						max_clear = local;
					} 
				} else {
					if (clear) {
						addNorthTransition(north, f, y, max_clear, startPos, endPos, run);
						run = 0;
						startPos = endPos = max_clear = -1;
						clear = false;
					}
				}
			} // for i < clusterSize
			if (clear) {
				addNorthTransition(north, f, y, max_clear, startPos, endPos, run);
			}
		}// for each Field
#		if _GAE_DEBUG_EDITION_
			Transitions::iterator it = north->transitions[Field::LAND].begin();
			for ( ; it != north->transitions[Field::LAND].end(); ++it) {
				PathfinderClusterOverlay::entranceCells.insert((*it)->nwPos);
			}
#		endif
	} // if not sentinel
}

void ClusterMap::initWestBorder(const Vec2i &cluster) {
	ClusterBorder *west = getWestBorder(cluster);
	if (west != &sentinel) {
		int x = cluster.x * clusterSize - 1;
		int x2 = x + 1;
		bool clear = false;  // true while evaluating an Transition, false when obstacle hit
		int max_clear = -1; // max clearance seen for current Transition
		int startPos = -1; // start position of entrance
		int endPos = -1;  // end position of entrance
		int run = 0;	 // to count entrance 'width'
		for (Field f(0); f < Field::COUNT; ++f) {
			if (!aMap->maxClearance[f]) continue;
			deleteValues(west->transitions[f].begin(), west->transitions[f].end());
			west->transitions[f].clear();			
			clear = false;
			max_clear = -1;
			for (int i=0; i < clusterSize; ++i) {
				int clear1 = aMap->metrics[Vec2i(x,POS_Y)].get(f);
				int clear2 = aMap->metrics[Vec2i(x2,POS_Y)].get(f);
				int local = min(clear1, clear2);
				if (local) {
					if (!clear) {
						clear = true;
						startPos = POS_Y;
					}
					eClear[run++] = local;
					endPos = POS_Y;
					if (local > max_clear) {
						max_clear = local;
					}
				} else {
					if (clear) {
						addWestTransition(west, f, x, max_clear, startPos, endPos, run);
						run = 0;
						startPos = endPos = max_clear = -1;
						clear = false;
					}
				}
			} // for i < clusterSize
			if (clear) {
				addWestTransition(west, f, x, max_clear, startPos, endPos, run);
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

/** initialise/re-initialise cluster (evaluates north and west borders) */
void ClusterMap::initCluster(const Vec2i &cluster) {
	initNorthBorder(cluster);
	initWestBorder(cluster);
}

//bool showIntraClusterPaths = false;

float ClusterMap::aStarPathLength(Field f, int size, const Vec2i &start, const Vec2i &dest) {
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

void ClusterMap::getTransitions(const Vec2i &cluster, Field f, Transitions &t) {
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

void ClusterMap::disconnectCluster(const Vec2i &cluster) {
	for (Field f(0); f < Field::COUNT; ++f) {
		Transitions t;
		getTransitions(cluster, f, t);
		set<const Transition*> tset;
		for (Transitions::iterator it = t.begin(); it != t.end(); ++it) {
			tset.insert(*it);
		}
		for (Transitions::iterator it = t.begin(); it != t.end(); ++it) {
			Edges e = (*it)->edges;
			Edges::iterator eit = e.begin(); 
			while (eit != e.end()) {
				if (tset.find((*eit)->transition()) != tset.end()) {
					eit = e.erase(eit);
				} else {
					++eit;
				}
			}
		}
	}
}

void ClusterMap::update() {
	/*
	for (set<Vec2i>::iterator it = dirtyNorthBorders.begin(); it != dirtyNorthBorders.end(); ++it) {
		if (it->y > 0 && it->y < h) {
			dirtyClusters.insert(Vec2i(it->x, it->y));
			dirtyClusters.insert(Vec2i(it->x, it->y - 1));
		}
	}
	for (set<Vec2i>::iterator it = dirtyWestBorders.begin(); it != dirtyWestBorders.end(); ++it) {
		if (it->x > 0 && it->x < w) {
			dirtyClusters.insert(Vec2i(it->x, it->y));
			dirtyClusters.insert(Vec2i(it->x - 1, it->y));
		}
	}
	for (set<Vec2i>::iterator it = dirtyClusters.begin(); it != dirtyClusters.end(); ++it) {
		disconnectCluster(*it);
	}
	for (set<Vec2i>::iterator it = dirtyNorthBorders.begin(); it != dirtyNorthBorders.end(); ++it) {
		initNorthBorder(*it);
	}
	for (set<Vec2i>::iterator it = dirtyWestBorders.begin(); it != dirtyWestBorders.end(); ++it) {
		initWestBorder(*it);
	}
	for (set<Vec2i>::iterator it = dirtyClusters.begin(); it != dirtyClusters.end(); ++it) {
		evalCluster(*it);
	}
	*/
	dirtyClusters.clear();
	dirtyClusters.clear();
	dirtyWestBorders.clear();
	dirty = false;
}

/** compute intra-cluster path lengths */
void ClusterMap::evalCluster(const Vec2i &cluster) {
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
	assert(open.find(pos) != open.end());
	assert(closed.find(prev) != closed.end());

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