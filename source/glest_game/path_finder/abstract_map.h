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

#ifndef _GLEST_GAME_ABSTRACT_MAP_H_
#define _GLEST_GAME_ABSTRACT_MAP_H_

#include "annotated_map.h"

namespace Glest { namespace Game { namespace Search {

class Cartographer;

// This will actually be 'packed' using bitfields... or not...
struct Entrance {
	int clearance;
	int position;
	int other_position;
};

struct Border {
	Border() { memset(this, 0, sizeof(*this)); clusters[0] = clusters[1] = Vec2i(-1); }
	Entrance transitions[Field::COUNT]; /**< Entrances of maximum clearance for each field */

	/** intra cluster edge weights
	 for vertical borders...        for horizontal borders...
	       2      3					       1
	   +------+------+				   +------+		[0] = NW
	   |      |      |				   |      |		[1] = N	
	 1 |  C1  |  C2  | 4			 0 |  C1  | 2	[2] = NE
	   |      |      |				   |      |		[3] = SE
	   +------+------+				   +------+		[4] = S
	       0      5					   |      |		[5] = SW
	   [0] == SW					 5 |  C2  | 3
	   [1] == W						   |      |
	   [2] == NW					   +------+
	   [3] == NE					       4
	   [4] == E
	   [5] == SE
	*/
	float weights[Field::COUNT][6];
	Border* neighbours[6]; /**< the neighbouring borders */
	Vec2i clusters[2]; /**< the clusters that this border is between */
	bool vertical;

	Vec2i getTransitionPoint(Field f) const {
		assert(transitions[f].position > 0);
		int x = -1, y = -1;
		Vec2i pos;
		if ( vertical ) {
			pos.x = transitions[f].other_position;
			pos.y = transitions[f].position;
		} else {
			pos.x = transitions[f].position;
			pos.y = transitions[f].other_position;
		}
		return pos;
	}

	float getDistanceTo(const Border *that, Field field, int size) const {
		int i = 0;
		while ( i < 6 && neighbours[i] != that ) ++i;
		assert(i != 6);
		if ( transitions[field].clearance < size || that->transitions[field].clearance < size ) {
			return numeric_limits<float>::infinity();
		}
		return weights[field][i];
	}
};

class BorderNeighbours {
public:
	void operator()(const Border* pos, vector<const Border*> &neighbours) {
		for ( int i=0; i < 6; ++i ) {
			if ( pos->neighbours[i] ) {
				neighbours.push_back(pos->neighbours[i]);
			}
		}
	}
};

struct AbstractAStarNode {
	const Border *pos, *prev;
	float heuristic;			  /**< estimate of distance to goal	  */
	float distToHere;			 /**< cost from origin to this node	 */
	//bool startBorder;

	float est()	const	{ 
		return distToHere + heuristic;	
	}										 /**< estimate, costToHere + heuristic */
};

// ========================================================
// class AbstractNodeStorage
// ========================================================
// NodeStorage template interface
class AbstractNodeStorage {
private:
	list<AbstractAStarNode*> openList;
	set<const Border*> open;
	set<const Border*> closed;
	map<const Border*, AbstractAStarNode*> listed;

	int size, nodeCount;
	AbstractAStarNode *stock;

	AbstractAStarNode* getNode();
	void insertIntoOpen(AbstractAStarNode *node);
	bool assertOpen();

public:
	AbstractNodeStorage() : stock(NULL) { }
	~AbstractNodeStorage() { delete [] stock; }
	void init(int size) { this->size = size; stock = new AbstractAStarNode[size]; reset(); }

	void reset() { nodeCount = 0; open.clear(); closed.clear(); openList.clear(); listed.clear(); }
	void setMaxNodes( int limit ) {}
	
	bool isOpen ( const Border* pos ) { return open.find(pos) != open.end(); }
	bool isClosed ( const Border* pos ) { return closed.find(pos) != closed.end(); }

	bool setOpen ( const Border* pos, const Border* prev, float h, float d );
	void updateOpen ( const Border* pos, const Border* &prev, const float cost );
	const Border* getBestCandidate();
	Border* getBestSeen();

	float getHeuristicAt( const Border* &pos )		{ return listed[pos]->heuristic;	}
	float getCostTo( const Border* pos )			{ return listed[pos]->distToHere;	}
	float getEstimateFor( const Border* pos )		{ return listed[pos]->est();		}
	const Border* getBestTo( const Border* pos )	{ return listed[pos]->prev;			}
};

// ========================================================
// class AbstractMap
// ========================================================

class AbstractMap {
	
	Border *vertBorders, *horizBorders, sentinel;
	int w, h; // width and height, in clusters
	AnnotatedMap *aMap;
	Cartographer *cartographer;

	static bool isInCluster(Vec2i cluster, Vec2i cell);

public:
	AbstractMap(Cartographer *c);
	~AbstractMap();

	static const int clusterSize = 16;

	static Vec2i cellToCluster (const Vec2i &cellPos) {
		return Vec2i(cellPos.x / clusterSize, cellPos.y / clusterSize);
	}

	Border* getSentinel() { return &sentinel; }

	// 'Initialise' a cluster (evaluates north and west borders)
	void initCluster(Vec2i cluster);

	// Update a cluster, evaluates all borders
	void updateCluster(Vec2i cluster);

	// compute intra-cluster path lengths
	void evalCluster(Vec2i cluster);

	//bool search(SearchParams params, list<Vec2i> &apath);

	void getBorders(Vec2i cluster, vector<Border*> &borders, Border *exclude = NULL);

	// Border getters
	Border* AbstractMap::getNorthBorder(Vec2i cluster) {
		return ( cluster.y == 0 )
			? &sentinel : &horizBorders[(cluster.y - 1) * w + cluster.x ];
	}

	Border* AbstractMap::getEastBorder(Vec2i cluster) {
		return ( cluster.x == w - 1 )
			? &sentinel : &vertBorders[cluster.y * (w - 1) + cluster.x ];
	}

	Border* AbstractMap::getSouthBorder(Vec2i cluster) {
		return ( cluster.y == h - 1 ) 
			? &sentinel : &horizBorders[cluster.y * w + cluster.x];
	}

	Border* getWestBorder(Vec2i cluster) { 
		return ( cluster.x == 0 )	
			? &sentinel : &vertBorders[cluster.y * (w - 1) + cluster.x - 1];
	}
private:

	float aStarPathLength(Field f, Vec2i &start, Vec2i &dest);
};


class AbstractSearchGoal {
public:
	AbstractSearchGoal(const Vec2i &target, AbstractMap *aMap) 
		: targetCluster(AbstractMap::cellToCluster(target)), aMap(aMap) {}
	/** search target */
	Vec2i targetCluster;
	AbstractMap *aMap;
	vector<Border*> borders;
	/** The goal function  */
	//FIXME: need to make sure actual target is reachable from border, gonna be messy :(
	bool operator()(const Border* &pos, const float costSoFar) { 
		aMap->getBorders(targetCluster,borders);
		bool ret = find(borders.begin(), borders.end(), pos) != borders.end();
		borders.clear();
		return ret;
	}
};

class AbstractMoveCost {
public:
	AbstractMoveCost(const Unit *unit) : unit(unit) {}
	const Unit *unit;
	float operator()(const Border* one, const Border* two) const {
		return one->getDistanceTo(two, unit->getCurrField(), unit->getSize());
	}
};

class AbstractHeuristic {
public:
	AbstractHeuristic(const Vec2i &target, Field field) : target(target), field(field) {}
	/** search target */
	Vec2i target;
	Field field;
	/** The heuristic function.
	  * @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target
	  */
	float operator()(const Border* &b) const {
		Vec2i pos = b->getTransitionPoint(field);
		float dx = (float)abs(pos.x - target.x), 
			  dy = (float)abs(pos.y - target.y);
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return 1.4 * diag + straight;
	}};

}}}

#endif
