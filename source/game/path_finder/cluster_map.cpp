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
#include "search_engine.h"
#include "route_planner.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

namespace Glest { namespace Game { namespace Search {

int Edge::numEdges[Field::COUNT];
int Transition::numTransitions[Field::COUNT];


#if SILNARM_HAS_DEFINED_SOME_CRAZY_PREPROCESSOR_SYMBOL
#	define WRITE_AND_FLUSH(x) { cout << x, cout.flush(); }
#else
#	define WRITE_AND_FLUSH(x) {}
#endif

void Edge::zeroCounters() {
	for (Field f(0); f < Field::COUNT; ++f) {
		numEdges[f] = 0;
	}
}

void Transition::zeroCounters() {
	for (Field f(0); f < Field::COUNT; ++f) {
		numTransitions[f] = 0;
	}
}

ClusterMap::ClusterMap(AnnotatedMap *aMap, Cartographer *carto) 
		: aMap(aMap) , carto(carto), dirty(false) {
	w = aMap->getWidth() / clusterSize;
	h = aMap->getHeight() / clusterSize;
	vertBorders = new ClusterBorder[(w-1)*h];
	horizBorders = new ClusterBorder[w*(h-1)];
	// init Borders (and hence inter-cluster edges) & evaluate clusters (intra-cluster edges)

	Edge::zeroCounters();
	Transition::zeroCounters();

	theLogger.setClusterCount(w * h);

	static char buf[512];
	char *ptr = buf;
	ptr += sprintf(ptr, "Initialising cluster map.\n");
	int64 time_millis = Chrono::getCurMillis();

	for (int i = h - 1; i >= 0; --i) {
		for (int j = w - 1; j >= 0; --j) {
			Vec2i cluster(j, i);
			WRITE_AND_FLUSH( "Cluster " << cluster << endl )
			initCluster(cluster);
			WRITE_AND_FLUSH( "Cluster " << cluster << "initialised.\n" )
			evalCluster(cluster);
			WRITE_AND_FLUSH( "Cluster " << cluster << "evaluated.\n" )
			theLogger.clusterInit();
		}
	}
	time_millis = Chrono::getCurMillis() - time_millis;
	ptr += sprintf(ptr, "\ttook %dms\n", (int)time_millis);
	theLogger.add(buf);
}

ClusterMap::~ClusterMap() {
	delete [] vertBorders;
	delete [] horizBorders;
	for (Field f(0); f < Field::COUNT; ++f) {
		assert(Edge::NumEdges(f) == 0);
		assert(Transition::NumTransitions(f) == 0);
		if (Edge::NumEdges(f) != 0 || Transition::NumTransitions(f) != 0) {
			throw runtime_error("memory leak");
		}
	}
}

void ClusterMap::assertValid() {
	bool valid[Field::COUNT];
	bool inUse[Field::COUNT];
	int numNodes[Field::COUNT];
	int numEdges[Field::COUNT];

	for (Field f(0); f < Field::COUNT; ++f) {
		typedef set<const Transition *> TSet;
		TSet tSet;

		typedef pair<Vec2i, bool> TKey;
		typedef map<TKey, const Transition *> TKMap;

		TKMap tkMap;

		valid[f] = true;
		numNodes[f] = 0;
		numEdges[f] = 0;
		inUse[f] = aMap->maxClearance[f] != 0;
		if (f == Field::AIR) inUse[f] = false;
		if (!inUse[f]) {
			continue;
		}

		// collect all transitions, checking for membership in tSet and tkMap (indicating an error)
		// and filling tSet and tkMap
		for (int i=0; i < (w - 1) * h; ++i) {
			ClusterBorder *b = &vertBorders[i];
			for (int j=0; j < b->transitions[f].n; ++j) {
				const Transition *t = b->transitions[f].transitions[j]; 
				if (tSet.find(t) != tSet.end()) {
					cout << "single transition on multiple borders.\n";
					valid[f] = false;
				} else {
					tSet.insert(t);
					TKey key(t->nwPos, t->vertical);
					if (tkMap.find(key) != tkMap.end()) {
						cout << "seperate transitions of same orientation on same cell.\n";
						valid[f] = false;
					} else {
						tkMap[key] = t;
					}
				}
				++numNodes[f];
			}
			
		}
		for (int i=0; i < w * (h - 1); ++i) {
			ClusterBorder *b = &horizBorders[i];
			for (int j=0; j < b->transitions[f].n; ++j) {
				const Transition *t = b->transitions[f].transitions[j]; 
				if (tSet.find(t) != tSet.end()) {
					cout << "single transition on multiple borders.\n";
					valid[f] = false;
				} else {
					tSet.insert(t);
					TKey key(t->nwPos, t->vertical);
					if (tkMap.find(key) != tkMap.end()) {
						cout << "seperate transitions of same orientation on same cell.\n";
						valid[f] = false;
					} else {
						tkMap[key] = t;
					}
				}
				++numNodes[f];
			}
		}
		
		// with a complete collection, iterate, check all dest transitions
		for (TSet::iterator it = tSet.begin(); it != tSet.end(); ++it) {
			const Edges &edges = (*it)->edges;
			for (Edges::const_iterator eit = edges.begin(); eit != edges.end(); ++eit) {
				TSet::iterator it2 = tSet.find((*eit)->transition());
				if (it2 == tSet.end()) {
					cout << "Invalid edge.\n";
					valid[f] = false;
				} else {
					if (*it == *it2) {
						cout << "self referential transition.\n";
						valid[f] = false;
					}
				}
				++numEdges[f];
			}
		}
	}
	cout << "\nClusterMap::assertValid()";
	cout << "\n=========================\n";
	for (Field f(0); f < Field::COUNT; ++f) {
		if (!inUse[f]) {
			cout << "Field::" << FieldNames[f] << " not in use.\n";
		} else {
			cout << "Field::" << FieldNames[f] << " in use and " << (!valid[f]? "NOT " : "") << "valid.\n";
			cout << "\t" << numNodes[f] << " transitions inspected.\n";
			cout << "\t" << numEdges[f] << " edges inspected.\n";
		}
	}
}

/** Entrance init helper class */
class InsideOutIterator {
private:
	int  centre, incr, end;
	bool flip;

public:
	InsideOutIterator(int low, int high)
			: flip(false) {
		centre = low + (high - low) / 2;
		incr = 0;
		end = ((high - low) % 2 != 0) ? low - 1 : high + 1;
	}

	int operator*() const {
		return centre + (flip ? incr : -incr);
	}

	void operator++() {
		flip = !flip;
		if (flip) ++incr;
	}

	bool more() {
		return **this != end;
	}
};

void ClusterMap::addBorderTransition(EntranceInfo &info) {
	assert(info.max_clear > 0 && info.startPos != -1 && info.endPos != -1);	
	WRITE_AND_FLUSH( 
		"\t\tEntrance from (" << info.endPos << ", " << info.pos << ") to (" 
		<< info.startPos << ", " << info.pos << ")" << endl 
	)
	if (info.run < 12) {
		// find central most pos with max clearance
		InsideOutIterator it(info.endPos, info.startPos);
		while (it.more()) {
			if (eClear[info.startPos - *it] == info.max_clear) {
				Transition *t;
				if (info.vert) {
					t = new Transition(Vec2i(info.pos, *it), info.max_clear, true, info.f);
				} else {
					t = new Transition(Vec2i(*it, info.pos), info.max_clear, false, info.f);
				}
				WRITE_AND_FLUSH( "\t\t\tadding Entrance at " << t->nwPos << "\n" )
				info.cb->transitions[info.f].add(t);
				return;
			}
			++it;
		}
		WRITE_AND_FLUSH( "\t\t\tno pos with clearance == max_clear found!!!! WTF!!!" )
		assert(false);
	} else {
		// look for two, as close as possible to 1/4 and 3/4 accross
		int l1 = info.endPos;
		int h1 = info.endPos + (info.startPos - info.endPos) / 2 - 1;
		int l2 = info.endPos + (info.startPos - info.endPos) / 2;
		int h2 = info.startPos;
		InsideOutIterator it(l1, h1);
		int first_at = -1;
		while (it.more()) {
			if (eClear[info.startPos - *it] == info.max_clear) {
				first_at = *it;
				break;
			}
			++it;
		}
		if (first_at != -1) {
			it = InsideOutIterator(l2, h2);
			int next_at = -1;
			while (it.more()) {
				if (eClear[info.startPos - *it] == info.max_clear) {
					next_at = *it;
					break;
				}
				++it;
			}
			if (next_at != -1) {
				Transition *t1, *t2;
				if (info.vert) {
					t1 = new Transition(Vec2i(info.pos, first_at), info.max_clear, true, info.f);
					t2 = new Transition(Vec2i(info.pos, next_at), info.max_clear, true, info.f);
				} else {
					t1 = new Transition(Vec2i(first_at, info.pos), info.max_clear, false, info.f);
					t2 = new Transition(Vec2i(next_at, info.pos), info.max_clear, false, info.f);
				}
				WRITE_AND_FLUSH( "\t\t\tadding Entrance at " << t1->nwPos << "\n" )
				WRITE_AND_FLUSH( "\t\t\tadding Entrance at " << t2->nwPos << "\n" )
				info.cb->transitions[info.f].add(t1);
				info.cb->transitions[info.f].add(t2);
				return;
			}
		}
		// failed to find two, just add one...
		it = InsideOutIterator(info.endPos, info.startPos);
		while (it.more()) {
			if (eClear[info.startPos - *it] == info.max_clear) {
				Transition *t;
				if (info.vert) {
					t = new Transition(Vec2i(info.pos, *it), info.max_clear, true, info.f);
				} else {
					t = new Transition(Vec2i(*it, info.pos), info.max_clear, false, info.f);
				}
				WRITE_AND_FLUSH( "\t\t\tadding Entrance at " << t->nwPos << "\n" )
				info.cb->transitions[info.f].add(t);
				return;
			}
			++it;
		}
		WRITE_AND_FLUSH( "\t\t\tno pos with clearance == max_clear found!!!! WTF!!!" )
		assert(false);
	}
}

void ClusterMap::initClusterBorder(const Vec2i &cluster, bool north) {
	WRITE_AND_FLUSH( "\tInit " << (north ? "north" : "west") << " border [cluster=" << cluster << "]\n" )
	ClusterBorder *cb = north ? getNorthBorder(cluster) : getWestBorder(cluster);
	EntranceInfo inf;
	inf.cb = cb;
	inf.vert = !north;
	if (cb != &sentinel) {
		int pos = north ? cluster.y * clusterSize - 1 : cluster.x * clusterSize - 1;
		inf.pos = pos;
		int pos2 = pos + 1;
		bool clear = false;   // true while evaluating a Transition, false when obstacle hit
		inf.max_clear = -1;  // max clearance seen for current Transition
		inf.startPos = -1; // start position of entrance
		inf.endPos = -1;  // end position of entrance
		inf.run = 0;	 // to count entrance 'width'
		for (Field f(0); f < Field::COUNT; ++f) {
			if (!aMap->maxClearance[f] || f == Field::AIR) continue;
#			if _GAE_DEBUG_EDITION_
				if (f == Field::LAND) {	
					for (int i=0; i < cb->transitions[f].n; ++i) {
						PathfinderClusterOverlay::entranceCells.erase(
							cb->transitions[f].transitions[i]->nwPos
						);
					}
				}
#			endif
			cb->transitions[f].clear();
			clear = false;
			inf.f = f;
			inf.max_clear = -1;
			for (int i=0; i < clusterSize; ++i) {
				int clear1, clear2;
				if (north) {
					clear1 = aMap->metrics[Vec2i(POS_X,pos)].get(f);
					clear2 = aMap->metrics[Vec2i(POS_X,pos2)].get(f);
				} else {
					clear1 = aMap->metrics[Vec2i(pos, POS_Y)].get(f);
					clear2 = aMap->metrics[Vec2i(pos2, POS_Y)].get(f);
				}
				int local = min(clear1, clear2);
				if (local) {
					if (!clear) {
						clear = true;
						inf.startPos = north ? POS_X : POS_Y;
					}
					eClear[inf.run++] = local;
					inf.endPos = north ? POS_X : POS_Y;
					if (local > inf.max_clear) {
						inf.max_clear = local;
					} 
				} else {
					if (clear) {
						addBorderTransition(inf);
						inf.run = 0;
						inf.startPos = inf.endPos = inf.max_clear = -1;
						clear = false;
					}
				}
			} // for i < clusterSize
			if (clear) {
				addBorderTransition(inf);
				inf.run = 0;
				inf.startPos = inf.endPos = inf.max_clear = -1;
				clear = false;
			}
		}// for each Field
#		if _GAE_DEBUG_EDITION_
			for (int i=0; i < cb->transitions[Field::LAND].n; ++i) {
				PathfinderClusterOverlay::entranceCells.insert(
					cb->transitions[Field::LAND].transitions[i]->nwPos
				);
			}
#		endif
	} // if not sentinel
	WRITE_AND_FLUSH( "\tDone Init " << (north ? "north" : "west") << " border [cluster=" << cluster << "]\n" )
}

float ClusterMap::aStarPathLength(Field f, int size, const Vec2i &start, const Vec2i &dest) {
	if (start == dest) {
		return 0.f;
	}
	SearchEngine<NodeStore> *se = carto->getRoutePlanner()->getSearchEngine();
	MoveCost costFunc(f, size, aMap);
	DiagonalDistance dd(dest);
	se->setNodeLimit(clusterSize * clusterSize);
	se->setStart(start, dd(start));
	AStarResult res = se->aStar<PosGoal,MoveCost,DiagonalDistance>(PosGoal(dest), costFunc, dd);
	Vec2i goalPos = se->getGoalPos();
	if (res != AStarResult::COMPLETE || goalPos != dest) {
		return numeric_limits<float>::infinity();
	}
	return se->getCostTo(goalPos);
}

void ClusterMap::getTransitions(const Vec2i &cluster, Field f, Transitions &t) {
	ClusterBorder *b = getNorthBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
	b = getEastBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
	b = getSouthBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
	b = getWestBorder(cluster);
	for (int i=0; i < b->transitions[f].n; ++i) {
		t.push_back(b->transitions[f].transitions[i]);
	}
}

void ClusterMap::disconnectCluster(const Vec2i &cluster) {
	//cout << "Disconnecting cluster " << cluster << endl;
	for (Field f(0); f < Field::COUNT; ++f) {
		if (!aMap->maxClearance[f] || f == Field::AIR) continue;
		Transitions t;
		getTransitions(cluster, f, t);
		set<const Transition*> tset;
		for (Transitions::iterator it = t.begin(); it != t.end(); ++it) {
			tset.insert(*it);
		}
		int del = 0;
		for (Transitions::iterator it = t.begin(); it != t.end(); ++it) {
			Transition *t = const_cast<Transition*>(*it);
			Edges::iterator eit = t->edges.begin(); 
			while (eit != t->edges.end()) {
				if (tset.find((*eit)->transition()) != tset.end()) {
					delete *eit;
					eit = t->edges.erase(eit);
					++del;
				} else {
					++eit;
				}
			}
		}
		//cout << "\tDeleted " << del << " edges in Field = " << FieldNames[f] << ".\n";
	}
	
}

void ClusterMap::update() {
	//cout << "ClusterMap::update()" << endl;
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
		//cout << "cluster " << *it << " dirty." << endl;
		disconnectCluster(*it);
	}
	for (set<Vec2i>::iterator it = dirtyNorthBorders.begin(); it != dirtyNorthBorders.end(); ++it) {
		//cout << "cluster " << *it << " north border dirty." << endl;
		initClusterBorder(*it, true);
	}
	for (set<Vec2i>::iterator it = dirtyWestBorders.begin(); it != dirtyWestBorders.end(); ++it) {
		//cout << "cluster " << *it << " west border dirty." << endl;
		initClusterBorder(*it, false);
	}
	for (set<Vec2i>::iterator it = dirtyClusters.begin(); it != dirtyClusters.end(); ++it) {
		evalCluster(*it);
	}
	
	dirtyClusters.clear();
	dirtyClusters.clear();
	dirtyWestBorders.clear();
	dirty = false;
}

/** compute intra-cluster path lengths */
void ClusterMap::evalCluster(const Vec2i &cluster) {
	GridNeighbours::setSearchCluster(cluster);
	Transitions transitions;
	for (Field f(0); f < Field::COUNT; ++f) {
		if (!aMap->maxClearance[f] || f == Field::AIR) continue;
		transitions.clear();
		getTransitions(cluster, f, transitions);
		Transitions::iterator it = transitions.begin();
		for ( ; it != transitions.end(); ++it) {
			Transition *t = const_cast<Transition*>(*it);
			Vec2i start = t->nwPos;
			Transitions::iterator it2 = transitions.begin();
			for ( ; it2 != transitions.end(); ++it2) {
				const Transition* &t2 = *it2;
				if (t == t2) continue;
				Vec2i dest = t2->nwPos;
				float cost  = aStarPathLength(f, 1, start, dest);
				if (cost == numeric_limits<float>::infinity()) continue;
				Edge *e = new Edge(t2, f);
				t->edges.push_back(e);
				e->addWeight(cost);
				int size = 2;
				int maxClear = t->clearance > t2->clearance ? t2->clearance : t->clearance;
				while (size <= maxClear) {
					cost = aStarPathLength(f, size, start, dest);
					if (cost == numeric_limits<float>::infinity()) {
						break;
					}
					e->addWeight(cost);
					assert(size == e->maxClear());
					++size;
				}
			}
		}
	} // for each Field
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
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