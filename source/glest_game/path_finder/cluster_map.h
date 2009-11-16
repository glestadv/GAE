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

#ifndef _GLEST_GAME_CLUSTER_MAP_H_
#define _GLEST_GAME_CLUSTER_MAP_H_

#include "annotated_map.h"
#include "game_constants.h"

namespace Glest { namespace Game { namespace Search {

class Cartographer;
struct Transition;

struct Edge : pair<Transition*, vector<float>> {
	Edge(Transition *t)  { first = t; }
	Transition* transition() const { return first; }
	float cost(int size) const { return second[size-1]; }
	int maxClear() { return second.size(); }
};
typedef vector<Edge*> Edges;

struct Transition {
	int clearance;
	Vec2i nwPos;
	bool vertical;
	Edges edges;

	Transition(Vec2i pos, int clear, bool vert) : nwPos(pos), clearance(clear), vertical(vert) {}
};
typedef vector<const Transition*> Transitions;

class TransitionNeighbours {
	class Builder {
		Transitions &transitions;
	public:
		Builder(Transitions &t) : transitions(t) {}
		void operator()(Edge *e) {
			transitions.push_back(e->transition());
		}
	};
public:
	void operator()(const Transition* pos, Transitions &neighbours) const {
		for_each(pos->edges.begin(), pos->edges.end(), Builder(neighbours));
	}

};

/** cost function for searching cluster map */
class TransitionCost {
	// predicate for find_if
	/*class Finder {
		const Transition *t;
	public:
		Finder(const Transition *t) : t(t) {}
		bool operator()(const Edge *e) const { 
			return (t == e->transition());
		}
	};*/
	// data
	Field field;
	int size;

public:
	TransitionCost(Field f, int s) : field(f), size(s) {}
	float operator()(const Transition *t, const Transition *t2) const {
		Edges::const_iterator it = //find_if(t->edges.begin(), t->edges.end(), Finder(t2));
			t->edges.begin();
		for ( ; it != t->edges.end(); ++it ) {
			if ( (*it)->transition() == t2 ) {
				break;
			}
		}
		if ( it == t->edges.end() ) {
			throw runtime_error("bad connection.");
		}
		if ( it != t->edges.end() && (*it)->maxClear() >= size ) {
			return (*it)->cost(size);
		}
		return numeric_limits<float>::infinity();
	}
};

/** goal function for search on cluster map */
class TransitionGoal {
	set<const Transition*> goals;
public:
	TransitionGoal() {}
	set<const Transition*>& goalTransitions() {return goals;}
	bool operator()(const Transition *t, const float d) const {
		return goals.find(t) != goals.end();
	}
};

struct ClusterBorder {
	Transitions transitions[Field::COUNT];
};

#define POS_X ((cluster.x+1)*clusterSize-i-1)
#define POS_Y ((cluster.y+1)*clusterSize-i-1)

class ClusterMap {
	int w, h;
	ClusterBorder *vertBorders, *horizBorders, sentinel;
	Cartographer *carto;
	AnnotatedMap *aMap;

public:
	ClusterMap(AnnotatedMap *aMap, Cartographer *carto);
	~ClusterMap() {
		delete [] vertBorders;
		delete [] horizBorders;
	}

	static const int clusterSize;// = 16;

	static Vec2i cellToCluster (const Vec2i &cellPos) {
		return Vec2i(cellPos.x / clusterSize, cellPos.y / clusterSize);
	}
	// ClusterBorder getters
	ClusterBorder* getNorthBorder(Vec2i cluster) {
		return ( cluster.y == 0 ) ? &sentinel : &horizBorders[(cluster.y - 1) * w + cluster.x ];
	}
	ClusterBorder* getEastBorder(Vec2i cluster) {
		return ( cluster.x == w - 1 ) ? &sentinel : &vertBorders[cluster.y * (w - 1) + cluster.x ];
	}
	ClusterBorder* getSouthBorder(Vec2i cluster) {
		return ( cluster.y == h - 1 ) ? &sentinel : &horizBorders[cluster.y * w + cluster.x];
	}
	ClusterBorder* getWestBorder(Vec2i cluster) { 
		return ( cluster.x == 0 ) ? &sentinel : &vertBorders[cluster.y * (w - 1) + cluster.x - 1]; 
	}
	/*
	void getBorders(Vec2i cluster, vector<ClusterBorder*> &borders, ClusterBorder *exclude = NULL) {
		ClusterBorder *b = getNorthBorder(cluster);
		if ( b != &sentinel && b != exclude ) borders.push_back(getNorthBorder(cluster));
		b = getEastBorder(cluster);
		if ( b != &sentinel && b != exclude ) borders.push_back(getEastBorder(cluster));
		b = getSouthBorder(cluster);
		if ( b != &sentinel && b != exclude ) borders.push_back(getSouthBorder(cluster));
		b = getWestBorder(cluster);
		if ( b != &sentinel && b != exclude ) borders.push_back(getWestBorder(cluster));
	}*/

	void initCluster(Vec2i cluster);
	void evalCluster(Vec2i cluster);
	float aStarPathLength(Field f, int size, Vec2i &start, Vec2i &dest);
	void getTransitions(Vec2i cluster, Field f, Transitions &t);
	void setClusterSearchSpace(Transition *t, Transition *t2);
};

struct TransitionAStarNode {
	const Transition *pos, *prev;
	float heuristic;			  /**< estimate of distance to goal	  */
	float distToHere;			 /**< cost from origin to this node	 */
	//bool startBorder;

	float est()	const	{ 
		return distToHere + heuristic;	
	}										 /**< estimate, costToHere + heuristic */
};

// ========================================================
// class TransitionNodeStorage
// ========================================================
// NodeStorage template interface
class TransitionNodeStore {
private:
	list<TransitionAStarNode*> openList;
	set<const Transition*> open;
	set<const Transition*> closed;
	map<const Transition*, TransitionAStarNode*> listed;

	int size, nodeCount;
	TransitionAStarNode *stock;

	TransitionAStarNode* getNode();
	void insertIntoOpen(TransitionAStarNode *node);
	bool assertOpen();

public:
	TransitionNodeStore() : stock(NULL) { }
	~TransitionNodeStore() { delete [] stock; }
	void init(int size) { this->size = size; stock = new TransitionAStarNode[size]; reset(); }

	void reset() { nodeCount = 0; open.clear(); closed.clear(); openList.clear(); listed.clear(); }
	void setMaxNodes( int limit ) {}
	
	bool isOpen ( const Transition* pos ) { return open.find(pos) != open.end(); }
	bool isClosed ( const Transition* pos ) { return closed.find(pos) != closed.end(); }

	bool setOpen ( const Transition* pos, const Transition* prev, float h, float d );
	void updateOpen ( const Transition* pos, const Transition* &prev, const float cost );
	const Transition* getBestCandidate();
	Transition* getBestSeen();

	float getHeuristicAt( const Transition* &pos )		{ return listed[pos]->heuristic;	}
	float getCostTo( const Transition* pos )			{ return listed[pos]->distToHere;	}
	float getEstimateFor( const Transition* pos )		{ return listed[pos]->est();		}
	const Transition* getBestTo( const Transition* pos )	{ return listed[pos]->prev;			}
};


}}}

#endif