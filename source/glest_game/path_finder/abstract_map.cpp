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

//#include "pch.h"

#include <algorithm>

#include "search_engine.h"
#include "abstract_map.h"
#include "cartographer.h"

#include "debug_renderer.h"
#include "search_engine.h"

namespace Glest { namespace Game { namespace Search {


// ========================================================
// class AbstractNodeStorage
// ========================================================

AbstractAStarNode* AbstractNodeStorage::getNode() {
	if ( nodeCount == size ) {
		//assert(false);
		return NULL;
	}
	return &stock[nodeCount++];
}

void AbstractNodeStorage::insertIntoOpen(AbstractAStarNode *node) {
	if ( openList.empty() ) {
		openList.push_front(node);
		return;
	}
	list<AbstractAStarNode*>::iterator it = openList.begin();
	while ( it != openList.end() && (*it)->est() <= node->est() ) ++it;
	
	//if ( it != openList.begin() ) {
		openList.insert(it, node);
	//} else {
	//	openList.push_back(node);
	//}
}

bool AbstractNodeStorage::assertOpen() {
	if ( openList.size() < 2 ) {
		return true;
	}
	set<const Border*> seen;
	list<AbstractAStarNode*>::iterator it1, it2 = openList.begin();
	it1 = it2;
	seen.insert((*it1)->pos);
	for ( ++it2; it2 != openList.end(); ++it2 ) {
		if ( seen.find((*it2)->pos) != seen.end() ) {
			theLogger.add("open list has cycle... that's bad.");
			return false;
		}
		seen.insert((*it2)->pos);
		if ( (*it1)->est() > (*it2)->est() ) {
			theLogger.add("open list is not ordered correctly.");
			return false;
		}
	}
	set<const Border*>::iterator it = open.begin();
	for ( ; it != open.end(); ++it ) {
		if ( seen.find(*it) == seen.end() ) {
			theLogger.add("node marked open not on open list.");
			return false;
		}
	}
	it = seen.begin();
	for ( ; it != seen.end(); ++it ) {
		if ( open.find(*it) == open.end() ) {
			theLogger.add("node on open list not marked open.");
			return false;
		}
	}
	return true;
}
Border* AbstractNodeStorage::getBestSeen() {
	assert(false); 
	return NULL;
}

bool AbstractNodeStorage::setOpen ( const Border* pos, const Border* prev, float h, float d ) {
	assert(open.find(pos) == open.end());
	assert(closed.find(pos) == closed.end());
	
	assert(assertOpen());
	
	AbstractAStarNode *node = getNode();
	if ( !node ) return false;
	node->pos = pos;
	node->prev = prev;
	node->distToHere = d;
	node->heuristic = h;
	//node->startBorder = prev != NULL ? false : true;
	open.insert(pos);
	insertIntoOpen(node);
	listed[pos] = node;
	
	assert(assertOpen());

	return true;
}

void AbstractNodeStorage::updateOpen ( const Border* pos, const Border* &prev, const float cost ) {

	assert(assertOpen());

	AbstractAStarNode *prevNode = listed[prev];
	AbstractAStarNode *posNode = listed[pos];
	if ( prevNode->distToHere + cost < posNode->distToHere ) {
		openList.remove(posNode);
		posNode->prev = prev;
		posNode->distToHere = prevNode->distToHere + cost;
		insertIntoOpen(posNode);
	}

	assert(assertOpen());

}

const Border* AbstractNodeStorage::getBestCandidate() {
	if ( openList.empty() ) return NULL;
	AbstractAStarNode *node = openList.front();
	const Border *best = node->pos;
	openList.pop_front();
	open.erase(open.find(best));
	closed.insert(best);
	return best;
}


// ========================================================
// class AbstractMap
// ========================================================

AbstractMap::AbstractMap(Cartographer *cartographer) 
		: cartographer(cartographer) {
	aMap = cartographer->getMasterMap();

	assert(aMap->getWidth() % clusterSize == 0);
	assert(aMap->getHeight() % clusterSize == 0);

	w = aMap->getWidth() / clusterSize;
	h = aMap->getHeight() / clusterSize;

	vertBorders = new Border[(w-1)*h];
	horizBorders = new Border[w*(h-1)];

	for ( int i=0; i < (w-1)*h; ++i ) {
		vertBorders[i].vertical = true;
	}
	for ( int i=0; i < w*(h-1); ++i ) {
		horizBorders[i].vertical = false;
	}

	// the 'sentinel' (for borders on the edges of the map)
	for ( int f = 0; f < Field::COUNT; ++f ) {
		sentinel.transitions[f].clearance = 0;
		sentinel.transitions[f].position = -1;
		for ( int i = 0; i < 6; ++i ) {
			sentinel.weights[f][i] = -1;
		}
	}

	// init Borders (and hence inter-cluster edges) & evaluate clusters (intra-cluster edges)
	for ( int i = h - 1; i >= 0; --i ) {
		for ( int j = w - 1; j >= 0; --j ) {
			Vec2i cluster(j, i);
			initCluster(cluster);
			evalCluster(cluster);
		}
	}
}

AbstractMap::~AbstractMap() {
	delete [] vertBorders;
	delete [] horizBorders;
}

#define POS_X ((cluster.x+1)*clusterSize-i-1)
#define POS_Y ((cluster.y+1)*clusterSize-i-1)

void AbstractMap::initCluster(Vec2i cluster) {
	theLogger.add(string("initCluster() : ") + "Cluster: " + Vec2iToStr(cluster));

	Border *west = getWestBorder(cluster);
	Border *north = getNorthBorder(cluster);

	Border *b;
	// set border.clusters & border.neighbours
	if ( cluster.x > 0 ) {
		west->clusters[0] = Vec2i(cluster.x - 1, cluster.y);
		// neighbours
		b = getSouthBorder(west->clusters[0]);
		west->neighbours[0] = ( b != &sentinel ) ? b : NULL;
		b = getWestBorder(west->clusters[0]);
		west->neighbours[1] = ( b != &sentinel ) ? b : NULL;
		b = getNorthBorder(west->clusters[0]);
		west->neighbours[2] = ( b != &sentinel ) ? b : NULL;
		assert(getEastBorder(west->clusters[0]) == west);

	} else {
		west->clusters[0] = Vec2i(-1, -1);
		// neighbours
		west->neighbours[0] = NULL;
		west->neighbours[1] = NULL;
		west->neighbours[2] = NULL;
	}
	west->clusters[1] = cluster;

	// more neighbours
	b = getNorthBorder(west->clusters[1]);
	west->neighbours[3] = ( b != &sentinel ) ? b : NULL;
	b = getEastBorder(west->clusters[1]);
	west->neighbours[4] = ( b != &sentinel ) ? b : NULL;
	b = getSouthBorder(west->clusters[1]);
	west->neighbours[5] = ( b != &sentinel ) ? b : NULL;


	if ( cluster.y > 0 ) {
		north->clusters[0] = Vec2i(cluster.x, cluster.y - 1);
		// neighbours
		b = getWestBorder(north->clusters[0]);
		north->neighbours[0] = (b != &sentinel) ? b : NULL;
		b = getNorthBorder(north->clusters[0]);
		north->neighbours[1] = (b != &sentinel) ? b : NULL;
		b = getEastBorder(north->clusters[0]);
		north->neighbours[2] = (b != &sentinel) ? b : NULL;
		assert(getSouthBorder(north->clusters[0]) == north);
	} else {
		north->clusters[0] = Vec2i(-1, -1);
		// neighbours
		north->neighbours[0] = NULL;
		north->neighbours[1] = NULL;
		north->neighbours[2] = NULL;
	}
	north->clusters[1] = cluster;
	// more neighbours
	b = getEastBorder(north->clusters[1]);
	north->neighbours[3] = ( b != &sentinel ) ? b : NULL;
	b = getSouthBorder(north->clusters[1]);
	north->neighbours[4] = ( b != &sentinel ) ? b : NULL;
	b = getWestBorder(north->clusters[1]);
	north->neighbours[5] = ( b != &sentinel ) ? b : NULL;
	
	if ( west != &sentinel ) {
		int x1 = cluster.x * clusterSize;
		int x2 = x1 - 1;
		int cy = (cluster.y + 1) * clusterSize - clusterSize / 2;
		memset(west->transitions, 0, sizeof(Entrance) * Field::COUNT);
		for ( int i=0; i < clusterSize; ++i ) {
			for ( int f = 0; f < Field::COUNT; ++f ) {
				int clear1 = aMap->metrics[Vec2i(x1,POS_Y)].get(enum_cast<Field>(f));
				int clear2 = aMap->metrics[Vec2i(x2,POS_Y)].get(enum_cast<Field>(f));
				int local = min(clear1, clear2);
				//if ( cluster == Vec2i(3,3) && enum_cast<Field>(f) == Field::LAND ) {
				//	theLogger.add(Vec2iToStr(Vec2i(x1,POS_Y))+"="+intToStr(clear1)+" "
				//		+Vec2iToStr(Vec2i(x2,POS_Y))+"="+intToStr(clear2)+ " local="+intToStr(local));
				//}
				if ( local > west->transitions[f].clearance 
				||  (local && local == west->transitions[f].clearance 
				&&  abs(POS_Y - cy) < abs(west->transitions[f].position - cy)) ) {
					//if ( cluster == Vec2i(3,3) && enum_cast<Field>(f) == Field::LAND ) {
					//	theLogger.add("setting local="+intToStr(local));
					//}
					west->transitions[f].clearance = local;
					west->transitions[f].position = POS_Y; // store i instead ???
					west->transitions[f].other_position = x1;
				}
			}
		}
#		if DEBUG_PATHFINDER_CLUSTER_OVERLAY
		//if ( cluster.y == 3 ) {
			if ( west->transitions[Field::LAND].clearance > 0 ) {
				int y = west->transitions[Field::LAND].position;
				PathfinderClusterOverlay::entranceCells.insert(Vec2i(x1,y));
				PathfinderClusterOverlay::entranceCells.insert(Vec2i(x2,y));
			}
			//theLogger.add("west border");
		//}
#		endif
	}
	if ( north != &sentinel ) {
		int y1 = cluster.y * clusterSize;
		int y2 = y1 - 1;
		int cx = (cluster.x + 1) * clusterSize - clusterSize / 2;
		memset(north->transitions, 0, sizeof(Entrance) * Field::COUNT);
		for ( int i=0; i < clusterSize; ++i ) {
			for ( int f = 0; f < Field::COUNT; ++f ) {
				int clear1 = aMap->metrics[Vec2i(POS_X,y1)].get(enum_cast<Field>(f));
				int clear2 = aMap->metrics[Vec2i(POS_X,y2)].get(enum_cast<Field>(f));
				int local = min(clear1, clear2);
				if ( cluster.y == 3 && enum_cast<Field>(f) == Field::LAND ) {
					theLogger.add(Vec2iToStr(Vec2i(POS_X,y1))+"="+intToStr(clear1)+" "
						+Vec2iToStr(Vec2i(POS_X,y2))+"="+intToStr(clear2)+ " local="+intToStr(local));
				}
				if ( local > north->transitions[f].clearance
				||  (local && local == north->transitions[f].clearance 
				&&  abs(POS_X - cx) < abs(north->transitions[f].position - cx)) ) {
					//if ( cluster.y == 3 && enum_cast<Field>(f) == Field::LAND ) {
					//	theLogger.add("setting local="+intToStr(local));
					//}
					north->transitions[f].clearance = local;
					north->transitions[f].position = POS_X;
					north->transitions[f].other_position = y1;
				}
			}			
		}
#		if DEBUG_PATHFINDER_CLUSTER_OVERLAY
		//if ( cluster.y == 3 ) {
			if ( north->transitions[Field::LAND].clearance > 0 ) {
				int x = north->transitions[Field::LAND].position;
				PathfinderClusterOverlay::entranceCells.insert(Vec2i(x,y1));
				PathfinderClusterOverlay::entranceCells.insert(Vec2i(x,y2));	
			//	theLogger.add(string() + "Cluster: " + Vec2iToStr(cluster) + " North transition is at "
			//		+ Vec2iToStr(Vec2i(x,y1)));
			} //else {
			//	theLogger.add(string() + "Cluster: " + Vec2iToStr(cluster) + " has no North transition");
			//}
			//theLogger.add("north border");
		//}
#		endif
	}
}

float AbstractMap::aStarPathLength(Field f, Vec2i &start, Vec2i &dest) {
	if ( start == dest ) {
		return 0.f;
	}
	SearchEngine<NodeMap,GridNeighbours>* se = cartographer->getSearchEngine();
	MoveCost costFunc(f, 1, aMap);
	DiagonalDistance dd(dest);
	se->setNodeLimit( clusterSize * clusterSize );
	se->setStart(start, dd(start));
	int res = se->aStar<PosGoal,MoveCost,DiagonalDistance>(PosGoal(dest), costFunc, dd);
	se->setNodeLimit( -1 );
	Vec2i goalPos = se->getGoalPos();
	if ( res != AStarResult::COMPLETE || goalPos != dest ) {
		return numeric_limits<float>::infinity();
	}
#	if DEBUG_PATHFINDER_CLUSTER_OVERLAY
	if ( f == Field::LAND ) {
		Vec2i aPos = se->getPreviousPos(goalPos);
		while ( aPos != start ) {
			PathfinderClusterOverlay::pathCells.insert(aPos);
			aPos = se->getPreviousPos(aPos);
		}
	}
#	endif
	return se->getCostTo(goalPos);
}

#define NORTH_Y (cluster.y*clusterSize)
#define SOUTH_Y (cluster.y*clusterSize+clusterSize-1)
#define WEST_X (cluster.x*clusterSize)
#define EAST_X (cluster.x*clusterSize+clusterSize-1)

bool AbstractMap::isInCluster(Vec2i cluster, Vec2i cell) {
	if ( cell.x > EAST_X || cell.x < WEST_X || cell.y < NORTH_Y || cell.y > SOUTH_Y ) {
		return false;
	}
	return true;
}

void AbstractMap::evalCluster(Vec2i cluster) {
	Border *north = getNorthBorder(cluster);
	Border *east = getEastBorder(cluster);
	Border *south = getSouthBorder(cluster);
	Border *west = getWestBorder(cluster);

	Vec2i start, dest;

	GridNeighbours::setSearchClusterLocal(cluster);

	for ( int i = 0; i < Field::COUNT; ++i ) {
		//
		// TODO: when/if AnnotatedMap takes into account elevation
		// fix this, it's currently symmetric
		//
		if ( north->transitions[i].clearance ) {
			start.x = north->transitions[i].position;
			start.y = NORTH_Y;
			// path to east, south & west
			if ( east->transitions[i].clearance ) {
				dest.x = EAST_X;
				dest.y = east->transitions[i].position;
				assert(isInCluster(cluster,start) && isInCluster(cluster,dest));
				east->weights[i][2] = aStarPathLength(enum_cast<Field>(i), start, dest);
				north->weights[i][3] = east->weights[i][2];
			} else {
				north->weights[i][3] = -1.f;
			}
			if ( south->transitions[i].clearance ) {
				dest.x = south->transitions[i].position;
				dest.y = SOUTH_Y;
				assert(isInCluster(cluster,start) && isInCluster(cluster,dest));
				south->weights[i][1] = aStarPathLength(enum_cast<Field>(i), start, dest);
				north->weights[i][4] = south->weights[i][1];
			} else {
				north->weights[i][4] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				dest.x = WEST_X;
				dest.y = west->transitions[i].position;
				assert(isInCluster(cluster,start) && isInCluster(cluster,dest));
				west->weights[i][3] = aStarPathLength(enum_cast<Field>(i), start, dest);
				north->weights[i][5] = west->weights[i][3];
			} else {
				north->weights[i][5] = -1.f;
			}
		} else {
			if ( east->transitions[i].clearance ) {
				east->weights[i][2] = -1.f;
			}
			if ( south->transitions[i].clearance ) {
				south->weights[i][1] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				west->weights[i][3] = -1.f;
			}
		}
		if ( east->transitions[i].clearance ) {
			start.x = EAST_X;
			start.y = east->transitions[i].position;
			// path to south & west
			if ( south->transitions[i].clearance ) {
				dest.x = south->transitions[i].position;
				dest.y = SOUTH_Y;
				assert(isInCluster(cluster,start) && isInCluster(cluster,dest));
				south->weights[i][2] = aStarPathLength(enum_cast<Field>(i), start, dest);
				east->weights[i][0] = south->weights[i][2];
			} else {
				east->weights[i][0] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				dest.x = WEST_X;
				dest.y = west->transitions[i].position;
				assert(isInCluster(cluster,start) && isInCluster(cluster,dest));
				west->weights[i][4] = aStarPathLength(enum_cast<Field>(i), start, dest);
				east->weights[i][1] = west->weights[i][4];
			} else {
				east->weights[i][1] = -1.f;
			}
		} else {
			if ( south->transitions[i].clearance ) {
				south->weights[i][2] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				west->weights[i][4] = -1.f;
			}
		}
		if ( south->transitions[i].clearance ) {
			start.x = south->transitions[i].position;
			start.y = SOUTH_Y;
			// path to west
			if ( west->transitions[i].clearance ) {
				dest.x = WEST_X;
				dest.y = west->transitions[i].position;
				assert(isInCluster(cluster,start) && isInCluster(cluster,dest));
				west->weights[i][5] = aStarPathLength(enum_cast<Field>(i), start, dest);
				south->weights[i][0] = west->weights[i][5];
			} else {
				south->weights[i][0] = -1.f;
			}
		} else {
			if ( west->transitions[i].clearance ) {
				west->weights[i][5] = -1.f;
			}
		}
	}
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);

}
/*
bool AbstractMap::search(SearchParams params, list<Vec2i> &apath) {
	Vec2i startCluster(0), destCluster(0);
	startCluster.x = params.start.x / clusterSize;
	startCluster.y = params.start.y / clusterSize;
	destCluster.x = params.dest.x / clusterSize;
	destCluster.y = params.dest.y / clusterSize;
	
	apath.clear();
	if ( startCluster.dist(destCluster) < 1.5 ) {
		apath.push_back(params.start);
		apath.push_back(params.dest);
		return true;
	}

}*/


void AbstractMap::getBorders(Vec2i cluster, vector<Border*> &borders, Border *exclude) {
	if ( getNorthBorder(cluster) != &sentinel ) borders.push_back(getNorthBorder(cluster));
	if ( getEastBorder(cluster) != &sentinel ) borders.push_back(getEastBorder(cluster));
	if ( getSouthBorder(cluster) != &sentinel ) borders.push_back(getSouthBorder(cluster));
	if ( getWestBorder(cluster) != &sentinel ) borders.push_back(getWestBorder(cluster));
}
/*
void AbstractMap::getNeighbours(const Border *border) {

}
*/
Border* AbstractMap::getNorthBorder(Vec2i cluster) {
	if ( cluster.y == 0 ) {
		return &sentinel;
	} else {
		return &horizBorders[(cluster.y - 1) * w + cluster.x ];
	}
}

Border* AbstractMap::getEastBorder(Vec2i cluster) {
	if ( cluster.x == w - 1 ) {
		return &sentinel;
	} else {
		return &vertBorders[cluster.y * (w - 1) + cluster.x ];
	}
}

Border* AbstractMap::getSouthBorder(Vec2i cluster) {
	if ( cluster.y == h - 1 ) {
		return &sentinel;
	} else {
		return &horizBorders[cluster.y * w + cluster.x];
	}

}

Border* AbstractMap::getWestBorder(Vec2i cluster) {
	if ( cluster.x == 0 ) {
		return &sentinel;
	} else {
		return &vertBorders[cluster.y * (w - 1) + cluster.x - 1];
	}
}

}}}