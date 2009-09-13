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
// File: annotated_map.cpp
//
// Annotated Map, for use in pathfinding.
//
#include "pch.h"

#include <limits>

//#include "annotated_map.h"

#include "map.h"
#include "path_finder.h"

#include "profiler.h"

#define LOG(x) Logger::getInstance().add(x)

namespace Glest { namespace Game { namespace Search {

uint32 CellMetrics::get ( const Field field ) {
	switch ( field ) {
		case FieldWalkable: return field0;
		case FieldAir: return field1;
		case FieldAnyWater: return field2;
		case FieldDeepWater: return field3;
		case FieldAmphibious: return field4;
		default: throw runtime_error ( "Unknown Field passed to CellMetrics::get()" );
	}
}
void CellMetrics::set ( const Field field, uint32 val ) {
	assert ( val <= AnnotatedMap::maxClearanceValue );
	switch ( field ) {
		case FieldWalkable: field0 = val; return;
		case FieldAir: field1 = val; return;
		case FieldAnyWater: field2 = val; return;
		case FieldDeepWater: field3 = val; return;
		case FieldAmphibious: field4 = val; return;
		default: throw runtime_error ( "Unknown Field passed to CellMetrics::set()" );
	}
}
void CellMetrics::setAll ( uint32 val ) {
	assert ( val <= AnnotatedMap::maxClearanceValue );
	field0 = field1 = field2 = field3 = field4 = val;
}


SearchParams::SearchParams ( Unit *u ) {
	start = u->getPos(); 
	field = u->getCurrField ();
	size = u->getSize (); 
	team = u->getTeam ();
	goalFunc = NULL;
}


SearchParams::SearchParams () {
	start = dest = Vec2i(-1);
	field = FieldWalkable;
	size = 1; 
	team = -1;
	goalFunc = NULL;
}


const int AnnotatedMap::maxClearanceValue = 7;

AnnotatedMap::AnnotatedMap ( Map *m ) {
	cMap = m;
	metrics.init ( m->getW(), m->getH() );
	initMapMetrics ( cMap );
	aNodePool = new AStarNodePool ();
	aNodePool->init ( cMap );
}

AnnotatedMap::~AnnotatedMap () {
	delete aNodePool;
}

void AnnotatedMap::initMapMetrics ( Map *map ) {
	const int east = map->getW() - 1;
	int x = east;
	int y = map->getH() - 1;

	// set southern two rows to zero.
	for ( ; x >= 0; --x ) {
		metrics[Vec2i(x,y)].setAll ( 0 );
		metrics[Vec2i(x,y-1)].setAll ( 0 );
	}
	for ( y -= 2; y >= 0; -- y) {
		for ( x = east; x >= 0; --x ) {
			Vec2i pos ( x, y );
			if ( x > east - 2 ) { // far east tile, not valid
				metrics[pos].setAll ( 0 );
			}
			else {
				computeClearances ( pos );
			}
		}
	}
}

// pos: location of object added/removed
// size: size of same object
void AnnotatedMap::updateMapMetrics ( const Vec2i &pos, const int size ) {
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	PROFILE_LVL2_START("Updating Map Metrics");

	// first, re-evaluate the cells occupied (or formerly occupied)
	for ( int i = size - 1; i >= 0 ; --i ) {
		for ( int j = size - 1; j >= 0; --j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			computeClearances ( occPos );
		}
	}
	cascadingUpdate ( pos, size );
	PROFILE_LVL2_STOP("Updating Map Metrics");
}

//DEBUG
static char dbgBuf [1024];

void AnnotatedMap::cascadingUpdate ( const Vec2i &pos, const int size,  const Field field ) {
	list<Vec2i> *leftList, *aboveList, leftList1, leftList2, aboveList1, aboveList2;
	leftList = &leftList1;
	aboveList = &aboveList1;
	// both the left and above lists need to be sorted, bigger values first (right->left, bottom->top)
	for ( int i = size - 1; i >= 0; --i ) {
		// Check if positions are on map, (the '+i' components are along the sides of the building/object, 
		// so we assume they are ok). If so, list them
		if ( pos.x-1 >= 0 ) {
			leftList->push_back ( Vec2i (pos.x-1,pos.y+i) );
		}
		if ( pos.y-1 >= 0 ) {
			aboveList->push_back ( Vec2i (pos.x+i,pos.y-1) );
		}
	}
	// the cell to the nothwest...
	Vec2i *corner = NULL;
	Vec2i cornerHolder ( pos.x-1, pos.y-1 );
	if ( pos.x-1 >= 0 && pos.y-1 >= 0 ) {
		corner = &cornerHolder;
	}

	while ( !leftList->empty() || !aboveList->empty() || corner ) {
		// the left and above lists for the next loop iteration
		list<Vec2i> *newLeftList, *newAboveList;
		newLeftList = leftList == &leftList1 ? &leftList2 : &leftList1;
		newAboveList = aboveList == &aboveList1 ? &aboveList2 : &aboveList1;

		if ( !leftList->empty() ) {
			for ( VLIt it = leftList->begin (); it != leftList->end (); ++it ) {
				bool changed = false;
				if ( field == FieldCount ) { // permanent annotation, update all
					CellMetrics old = metrics[*it];
					computeClearances ( *it );
					if ( old != metrics[*it] ) {
						if ( it->x - 1 >= 0 ) { // if there is a cell to the left, add it to
							newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
						}
					}
				}
				else { // local annotation, only check field, store original clearances
					uint32 old = metrics[*it].get ( field );
					if ( old ) computeClearance ( *it, field );
					if ( old && old > metrics[*it].get ( field ) ) {
						if ( localAnnt.find (*it) == localAnnt.end() ) {
							localAnnt[*it] = old; // was original clearance
						}
						if ( it->x - 1 >= 0 ) { // if there is a cell to the left, add it to
							newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
						}
					}
				}
			}
		}
		if ( !aboveList->empty() ) {
			for ( VLIt it = aboveList->begin (); it != aboveList->end (); ++it ) {
				if ( field == FieldCount ) {
					CellMetrics old = metrics[*it];
					computeClearances ( *it );
					if ( old != metrics[*it] ) {
						if ( it->y - 1 >= 0 ) {
							newAboveList->push_back ( Vec2i(it->x,it->y-1) );
						}
					}
				}
				else {
					uint32 old = metrics[*it].get ( field );
					if ( old ) computeClearance ( *it, field );
					if ( old && old > metrics[*it].get ( field ) ) {
						if ( localAnnt.find (*it) == localAnnt.end() ) {
							localAnnt[*it] = old;
						}
						if ( it->y - 1 >= 0 )  {
							newAboveList->push_back ( Vec2i(it->x,it->y-1) );
						}
					}
				}
			}
		}
		if ( corner ) {
			// Deal with the corner...
			if ( field == FieldCount ) {
				CellMetrics old = metrics[*corner];
				computeClearances ( *corner );
				if ( old != metrics[*corner] ) {
					int x = corner->x, y  = corner->y;
					if ( x - 1 >= 0 ) {
						newLeftList->push_back ( Vec2i(x-1,y) );
						if ( y - 1 >= 0 ) {
							*corner = Vec2i(x-1,y-1);
						}
						else {
							corner = NULL;
						}
					}
					else {
						corner = NULL;
					}
					if ( y - 1 >= 0 ) { 
						newAboveList->push_back ( Vec2i(x,y-1) );
					}
				}
				else { // no update
					corner = NULL;
				}
			}
			else {
				uint32 old = metrics[*corner].get ( field );
				if ( old ) computeClearance ( *corner, field );
				if ( old && old > metrics[*corner].get ( field ) ) {
					if ( localAnnt.find (*corner) == localAnnt.end() ) {
						localAnnt[*corner] = old;
					}
					int x = corner->x, y  = corner->y;
					if ( x - 1 >= 0 ) {
						newLeftList->push_back ( Vec2i(x-1,y) );
						if ( y - 1 >= 0 ) {
							*corner = Vec2i(x-1,y-1);
						}
						else {
							corner = NULL;
						}
					}
					else {
						corner = NULL;
					}
					if ( y - 1 >= 0 ) {
						newAboveList->push_back ( Vec2i(x,y-1) );
					}
				}
				else {
					corner = NULL;
				}
			}
		}
		leftList->clear (); 
		leftList = newLeftList;
		aboveList->clear (); 
		aboveList = newAboveList;
	}// end while
}

void AnnotatedMap::annotateUnit ( const Unit *unit, const Field field ) {
	const int size = unit->getSize ();
	const Vec2i &pos = unit->getPos ();
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	// first, re-evaluate the cells occupied
	for ( int i = size - 1; i >= 0 ; --i ) {
		for ( int j = size - 1; j >= 0; --j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			if ( !unit->getType()->hasCellMap() || unit->getType()->getCellMapCell (i, j) ) {
				if ( localAnnt.find (occPos) == localAnnt.end() ) {
					localAnnt[occPos] = metrics[occPos].get (field);
				}
				metrics[occPos].set ( field, 0 );
			}
			else {
				uint32 old =  metrics[occPos].get (field);
				computeClearance (occPos, field);
				if ( old != metrics[occPos].get (field) && localAnnt.find (occPos) == localAnnt.end() ) {
					localAnnt[occPos] = old;
				}
			}
		}
	}
	// propegate changes to left and above
	cascadingUpdate ( pos, size, field );
}

// Set clearances (all fields) to 0 if cell blocked, or 'calculate' metrics
void AnnotatedMap::computeClearances ( const Vec2i &pos ) {
	assert ( cMap->isInside ( pos ) );
	assert ( pos.x <= getWidth() - 2 );
	assert ( pos.y <= getHeight() - 2 );

	Cell *cell = cMap->getCell ( pos );

	// is there a building here, or an object on the tile ??
	bool surfaceBlocked = ( cell->getUnit(ZoneSurface) && !cell->getUnit(ZoneSurface)->isMobile() )
							||   !cMap->getTile ( cMap->toTileCoords ( pos ) )->isFree ();
	// Walkable
	if ( surfaceBlocked || cell->isDeepSubmerged () )
		metrics[pos].set ( FieldWalkable, 0 );
	else
		computeClearance ( pos, FieldWalkable );
	// Any Water
	if ( surfaceBlocked || !cell->isSubmerged () )
		metrics[pos].set ( FieldAnyWater, 0 );
	else
		computeClearance ( pos, FieldAnyWater );
	// Deep Water
	if ( surfaceBlocked || !cell->isDeepSubmerged () )
		metrics[pos].set ( FieldDeepWater, 0 );
	else
		computeClearance ( pos, FieldDeepWater );
	// Amphibious:
	if ( surfaceBlocked )
		metrics[pos].set ( FieldAmphibious, 0 );
	else
		computeClearance ( pos, FieldAmphibious );
	// Air
	computeClearance ( pos, FieldAir );
}

// Computes clearance base on metrics to the sout and east, Does NOT check if this cell is an obstactle
// assumes metrics of cells to the south, south-east & south are correct
uint32 AnnotatedMap::computeClearance ( const Vec2i &pos, Field f ) {
	uint32 clear = metrics[Vec2i(pos.x, pos.y + 1)].get ( f );
	if ( clear > metrics[Vec2i( pos.x + 1, pos.y + 1 )].get ( f ) )
		clear = metrics[Vec2i( pos.x + 1, pos.y + 1 )].get ( f );
	if ( clear > metrics[Vec2i( pos.x + 1, pos.y )].get ( f ) )
		clear = metrics[Vec2i( pos.x + 1, pos.y )].get ( f );
	clear ++;
	if ( clear > maxClearanceValue ) 
		clear = maxClearanceValue;
	metrics[pos].set ( f, clear );
	return clear;
}

bool AnnotatedMap::canOccupy ( const Vec2i &pos, int size, Field field ) const {
	assert ( cMap->isInside ( pos ) );
	return metrics[pos].get ( field ) >= size ? true : false;
}

void AnnotatedMap::annotateLocal ( const Unit *unit, const Field field ) {
	PROFILE_LVL2_START("Local Annotations");
	const Vec2i &pos = unit->getPos ();
	const int &size = unit->getSize ();
	assert ( cMap->isInside ( pos ) );
	assert ( cMap->isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	const int dist = 3;
	set<Unit*> annotate;

	for ( int y = pos.y - dist; y < pos.y + size + dist; ++y ) {
		for ( int x = pos.x - dist; x < pos.x + size + dist; ++x ) {
			if ( metrics[Vec2i(x,y)].get(field) ) {
				Unit *u = cMap->getCell(x,y)->getUnit ( field );
				if ( u && u != unit ) {
					annotate.insert ( u );
				}
			}
		}
	}
	for ( set<Unit*>::iterator it = annotate.begin(); it != annotate.end(); ++it ) {
		annotateUnit ( *it, field );
	}
	PROFILE_LVL2_STOP("Local Annotations");
}

void AnnotatedMap::clearLocalAnnotations ( Field field ) {
	PROFILE_LVL2_START("Local Annotations");
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin (); it != localAnnt.end (); ++ it ) {
		assert ( it->second <= maxClearanceValue );
		assert ( cMap->isInside ( it->first ) );
		metrics[it->first].set ( field, it->second );
	}
	localAnnt.clear ();
	PROFILE_LVL2_STOP("Local Annotations");
}

bool AnnotatedMap::AStarSearch ( SearchParams &params, list<Vec2i> &path ) {
	PROFILE_LVL2_START("Searching");

	bool pathFound = false, nodeLimitReached = false;
	AStarNode *minNode = NULL;
	const Vec2i *Directions = OffsetsSize1Dist1;
	aNodePool->reset ();
	aNodePool->addToOpen ( NULL, params.start, heuristic ( params.start, params.dest ), 0 );
	while ( ! nodeLimitReached ) {
		minNode = aNodePool->getBestCandidate ();
		if ( ! minNode ) break; // done, failed
		PROFILE_LVL3_START("Checking Goal");
		if ( minNode->pos == params.dest || ! minNode->exploredCell 
		||  ( params.goalFunc && params.goalFunc (minNode->pos ) ) ) { // done, success
			pathFound = true;
			PROFILE_LVL3_STOP("Checking Goal");
			break; 
		}
		PROFILE_LVL3_STOP("Checking Goal");
		for ( int i = 0; i < 8 && ! nodeLimitReached; ++i ) {  // for each neighbour
			PROFILE_LVL3_START("Checking Move Legality");
			Vec2i sucPos = minNode->pos + Directions[i];
			if ( ! cMap->isInside ( sucPos ) 
			||	 ! canOccupy (sucPos, params.size, params.field )) {
				PROFILE_LVL3_STOP("Checking Move Legality");
				continue;
			}
			bool diag = false;
			if ( minNode->pos.x != sucPos.x && minNode->pos.y != sucPos.y ) {
				// if diagonal move and either diag cell is not free...
				diag = true;
				Vec2i diag1, diag2;
				getDiags ( minNode->pos, sucPos, params.size, diag1, diag2 );
				if ( !canOccupy ( diag1, 1, params.field ) 
				||	 !canOccupy ( diag2, 1, params.field ) ) {
					PROFILE_LVL3_STOP("Checking Move Legality");
					continue; // not allowed
				}
			}
			PROFILE_LVL3_STOP("Checking Move Legality");
			// Assumes heuristic is admissable, or that you don't care if it isn't
			if ( aNodePool->isOpen ( sucPos ) ) {
				PROFILE_LVL3_START("Updating Open Nodes");
				aNodePool->updateOpenNode ( sucPos, minNode, diag ? 1.4 : 1.0 );
				PROFILE_LVL3_STOP("Updating Open Nodes");
			}
			else if ( ! aNodePool->isClosed ( sucPos ) ) {
				PROFILE_LVL3_START("Adding New Open Nodes");
				bool exp = cMap->getTile (Map::toTileCoords (sucPos))->isExplored (params.team);
				PROFILE_LVL3_START("Heuristic");
				float h = heuristic ( sucPos, params.dest );
				PROFILE_LVL3_STOP("Heuristic");
				float d = minNode->distToHere + (diag?1.4:1.0);
				PROFILE_LVL3_START("Add to Node Pool");
				if ( ! aNodePool->addToOpen ( minNode, sucPos, h, d, exp ) ) {
					nodeLimitReached = true;
				}
				PROFILE_LVL3_STOP("Add to Node Pool");
				PROFILE_LVL3_STOP("Adding New Open Nodes");
			}
		} // end for each neighbour of minNode
	} // end while ( ! nodeLimitReached )
	if ( ! pathFound && ! nodeLimitReached ) {
		PROFILE_LVL2_STOP("Searching");
		return false;
	}
	PROFILE_LVL3_START("Post Processing");
	if ( nodeLimitReached ) {
		// get node closest to goal
		minNode = aNodePool->getBestHNode ();
		path.clear ();
		while ( minNode ) {
			path.push_front ( minNode->pos );
			minNode = minNode->prev;
		}
		int backoff = path.size () / 10;
		// back up a bit, to avoid a possible cul-de-sac
		for ( int i=0; i < backoff ; ++i ) {
			path.pop_back ();
		}
	}
	else {  // fill in path
		path.clear ();
		while ( minNode ) {
			path.push_front ( minNode->pos );
			minNode = minNode->prev;
		}
	}
	if ( path.size () < 2 ) {
		path.clear ();
		PROFILE_LVL3_STOP("Post Processing");
		PROFILE_LVL2_STOP("Searching");
		return true; //tsArrived
	}
	PROFILE_LVL3_STOP("Post Processing");
	PROFILE_LVL2_STOP("Searching");

#	ifdef _GAE_DEBUG_EDITION_
		if ( Config::getInstance().getMiscDebugTextures() ) {
			PathFinder *pf = PathFinder::getInstance();
			pf->PathStart = path.front();
			pf->PathDest = path.back();
			pf->OpenSet.clear(); pf->ClosedSet.clear();
			pf->PathSet.clear(); pf->LocalAnnotations.clear ();
			if ( pf->debug_texture_action == PathFinder::ShowOpenClosedSets ) {
				list<Vec2i> *alist = aNodePool->getOpenNodes ();
				for ( VLIt it = alist->begin(); it != alist->end(); ++it ) {
					pf->OpenSet.insert ( *it );
				}
				delete alist;
				alist = aNodePool->getClosedNodes ();
				for ( VLIt it = alist->begin(); it != alist->end(); ++it ) {
					pf->ClosedSet.insert ( *it );
				}
				delete alist;
			}
			if ( pf->debug_texture_action == PathFinder::ShowOpenClosedSets 
			||   pf->debug_texture_action == PathFinder::ShowPathOnly )
			for ( VLIt it = path.begin(); it != path.end(); ++it ) {
					pf->PathSet.insert ( *it );
			}
			if ( pf->debug_texture_action == PathFinder::ShowLocalAnnotations ) {
				pf->LocalAnnotations.clear();
				list<pair<Vec2i,uint32>> *annt = getLocalAnnotations ();
				for ( list<pair<Vec2i,uint32>>::iterator it = annt->begin(); it != annt->end(); ++it ) {
					pf->LocalAnnotations[it->first] = it->second;
				}
				delete annt;
			}
		}
#	endif
	path.pop_front();
	//assert ( assertValidPath ( path ) );
	return true;
}


bool AnnotatedMap::assertValidPath ( list<Vec2i> &path ) {
	if ( path.size () < 2 ) return true;
	VLIt it = path.begin();
	Vec2i prevPos = *it;
	for ( ++it; it != path.end(); ++it ) {
		if ( prevPos.dist(*it) < 0.99 || prevPos.dist(*it) > 1.42 )
			return false;
		prevPos = *it;
	}
	return true;
}
void AnnotatedMap::getDiags ( const Vec2i &s, const Vec2i &d, const int size, Vec2i &d1, Vec2i &d2 ) {
	assert ( s.x != d.x && s.y != d.y );
	if ( size == 1 ) {
		d1.x = s.x; d1.y = d.y;
		d2.x = d.x; d2.y = s.y;
		return;
	}
	if ( d.x > s.x ) {  // travelling east
		if ( d.y > s.y ) {  // se
			d1.x = d.x + size - 1; d1.y = s.y;
			d2.x = s.x; d2.y = d.y + size - 1;
		}
		else {  // ne
			d1.x = s.x; d1.y = d.y;
			d2.x = d.x + size - 1; d2.y = s.y - size + 1;
		}
	}
	else {  // travelling west
		if ( d.y > s.y ) {  // sw
			d1.x = d.x; d1.y = s.y;
			d2.x = s.x + size - 1; d2.y = d.y + size - 1;
		}
		else {  // nw
			d1.x = d.x; d1.y = s.y - size + 1;
			d2.x = s.x + size - 1; d2.y = d.y;
		}
	}
}


float AnnotatedMap::AStarPathLength ( SearchParams &params ) {
	if ( params.start == params.dest ) return 0.f;
	bool pathFound = false, nodeLimitReached = false;
	AStarNode *minNode = NULL;
	const Vec2i *Directions = OffsetsSize1Dist1;
	aNodePool->reset ();
	aNodePool->addToOpen ( NULL, params.start, heuristic ( params.start, params.dest ), 0 );
	while ( ! nodeLimitReached ) {
		minNode = aNodePool->getBestCandidate ();
		if ( ! minNode ) break; // done, failed
		if ( minNode->pos == params.dest || ! minNode->exploredCell 
		||  ( params.goalFunc && params.goalFunc (minNode->pos ) ) ) { // done, success
			pathFound = true; 
			break; 
		}
		for ( int i = 0; i < 8 && ! nodeLimitReached; ++i ) {  // for each neighbour
			Vec2i sucPos = minNode->pos + Directions[i];
			if ( ! cMap->isInside ( sucPos ) 
			||	 ! canOccupy (sucPos, params.size, params.field )) {
				continue;
			}
			bool diag = false;
			if ( minNode->pos.x != sucPos.x && minNode->pos.y != sucPos.y ) {
				// if diagonal move and either diag cell is not free...
				diag = true;
				Vec2i diag1, diag2;
				getDiags ( minNode->pos, sucPos, params.size, diag1, diag2 );
				if ( !canOccupy ( diag1, 1, params.field ) 
				||	 !canOccupy ( diag2, 1, params.field ) ) {
					continue; // not allowed
				}
			}
			// Assumes heuristic is admissable, or that you don't care if it isn't
			if ( aNodePool->isOpen ( sucPos ) ) {
				aNodePool->updateOpenNode ( sucPos, minNode, diag ? 1.4 : 1.0 );
			}
			else if ( ! aNodePool->isClosed ( sucPos ) ) {
				bool exp = cMap->getTile (Map::toTileCoords (sucPos))->isExplored (params.team);
				float h = heuristic ( sucPos, params.dest );
				float d = minNode->distToHere + ( diag ? 1.4 : 1.0 );
				if ( ! aNodePool->addToOpen ( minNode, sucPos, h, d, exp ) ) {
					nodeLimitReached = true;
				}
			}
		} // end for each neighbour of minNode
	} // end while ( ! nodeLimitReached )

	if ( ! pathFound ) {
		return numeric_limits<float>::infinity();
	}
	else {
		float dist = 0.f;
		Vec2i lastPos = minNode->pos;
		minNode = minNode->prev;
		while ( minNode ) {
			if ( minNode->pos.x != lastPos.x || minNode->pos.y != lastPos.y ) {
				dist += 1.42;
			}
			else {
				dist += 1.0;
			}
			lastPos = minNode->pos;
			minNode = minNode->prev;
		}
		return dist;
	}
}


#ifdef _GAE_DEBUG_EDITION_

list<pair<Vec2i,uint32>>* AnnotatedMap::getLocalAnnotations () {
	list<pair<Vec2i,uint32>> *ret = new list<pair<Vec2i,uint32>> ();
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin (); it != localAnnt.end (); ++ it )
		ret->push_back ( pair<Vec2i,uint32> (it->first,metrics[it->first].get(FieldWalkable)) );
	return ret;
}

#endif

}}}
