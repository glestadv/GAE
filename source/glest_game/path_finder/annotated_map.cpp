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

#include "map.h"
#include "route_planner.h"

#include "profiler.h"

#define LOG(x) Logger::getInstance().add(x)

namespace Glest { namespace Game { namespace Search {

/** Construct AnnotatedMap object, 'theMap' must be constructed and loaded
  * @param master true if this is the master map, false for a foggy map (default true)
  */
AnnotatedMap::AnnotatedMap(bool master) {
	metrics.init( theMap.getW(), theMap.getH() );
	if ( master ) {
		initMapMetrics();
	} else {
		metrics.zero();
	}
}

AnnotatedMap::~AnnotatedMap() {
}

/** Initialise clearance data for a master map.
  */
void AnnotatedMap::initMapMetrics() {
	const int east = theMap.getW() - 1;
	int x = east;
	int y = theMap.getH() - 1;

	// set southern two rows to zero.
	for ( ; x >= 0; --x ) {
		metrics[Vec2i(x,y)].setAll( 0 );
		metrics[Vec2i(x,y-1)].setAll( 0 );
	}
	for ( y -= 2; y >= 0; -- y) {
		for ( x = east; x >= 0; --x ) {
			Vec2i pos( x, y );
			if ( x > east - 2 ) { // far east tile, not valid
				metrics[pos].setAll( 0 );
			}
			else {
				computeClearances( pos );
			}
		}
	}
}

/** Update clearance data, when an obstactle is placed or removed from the map
  * @param pos the cell co-ordinates of the obstacle added/removed
  * @param size the size of the obstacle
  */
void AnnotatedMap::updateMapMetrics( const Vec2i &pos, const int size ) {
	assert( theMap.isInside ( pos ) );
	assert( theMap.isInside ( pos.x + size - 1, pos.y + size - 1 ) );
	PROFILE_LVL2_START("Updating Map Metrics");

	// first, re-evaluate the cells occupied (or formerly occupied)
	for ( int i = size - 1; i >= 0 ; --i ) {
		for ( int j = size - 1; j >= 0; --j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			computeClearances( occPos );
		}
	}
	cascadingUpdate( pos, size );
	PROFILE_LVL2_STOP("Updating Map Metrics");
}

/** Perform a 'cascading update' of clearance metrics having just changed clearances
  * @param pos the cell co-ordinates of the obstacle added/removed
  * @param size the size of the obstacle
  * @param field the field to update (local annotation), or Field::COUNT to update all fields (permanent)
  */
void AnnotatedMap::cascadingUpdate( const Vec2i &pos, const int size,  const Field field ) {
	list<Vec2i> *leftList, *aboveList, leftList1, leftList2, aboveList1, aboveList2;
	leftList = &leftList1;
	aboveList = &aboveList1;
	// both the left and above lists need to be sorted, bigger values first (right->left, bottom->top)
	for ( int i = size - 1; i >= 0; --i ) {
		// Check if positions are on map, (the '+i' components are along the sides of the building/object, 
		// so we assume they are ok). If so, list them
		if ( pos.x-1 >= 0 ) {
			leftList->push_back( Vec2i(pos.x-1,pos.y+i) );
		}
		if ( pos.y-1 >= 0 ) {
			aboveList->push_back( Vec2i(pos.x+i,pos.y-1) );
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
			for ( VLIt it = leftList->begin(); it != leftList->end(); ++it ) {
				bool changed = false;
				if ( field == Field::COUNT ) { // permanent annotation, update all
					CellMetrics old = metrics[*it];
					computeClearances( *it );
					if ( old != metrics[*it] ) {
						if ( it->x - 1 >= 0 ) { // if there is a cell to the left, add it to
							newLeftList->push_back ( Vec2i(it->x-1,it->y) ); // the new left list
						}
					}
				}
				else { // local annotation, only check field, store original clearances
					uint32 old = metrics[*it].get( field );
					if ( old ) computeClearance( *it, field );
					if ( old && old > metrics[*it].get( field ) ) {
						if ( localAnnt.find(*it) == localAnnt.end() ) {
							localAnnt[*it] = old; // was original clearance
						}
						if ( it->x - 1 >= 0 ) { // if there is a cell to the left, add it to
							newLeftList->push_back( Vec2i(it->x-1,it->y) ); // the new left list
						}
					}
				}
			}
		}
		if ( !aboveList->empty() ) {
			for ( VLIt it = aboveList->begin(); it != aboveList->end(); ++it ) {
				if ( field == Field::COUNT ) {
					CellMetrics old = metrics[*it];
					computeClearances( *it );
					if ( old != metrics[*it] ) {
						if ( it->y - 1 >= 0 ) {
							newAboveList->push_back( Vec2i(it->x,it->y-1) );
						}
					}
				}
				else {
					uint32 old = metrics[*it].get( field );
					if ( old ) computeClearance( *it, field );
					if ( old && old > metrics[*it].get( field ) ) {
						if ( localAnnt.find(*it) == localAnnt.end() ) {
							localAnnt[*it] = old;
						}
						if ( it->y - 1 >= 0 )  {
							newAboveList->push_back( Vec2i(it->x,it->y-1) );
						}
					}
				}
			}
		}
		if ( corner ) {
			// Deal with the corner...
			if ( field == Field::COUNT ) {
				CellMetrics old = metrics[*corner];
				computeClearances( *corner );
				if ( old != metrics[*corner] ) {
					int x = corner->x, y  = corner->y;
					if ( x - 1 >= 0 ) {
						newLeftList->push_back( Vec2i(x-1,y) );
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
						newAboveList->push_back( Vec2i(x,y-1) );
					}
				}
				else { // no update
					corner = NULL;
				}
			}
			else {
				uint32 old = metrics[*corner].get( field );
				if ( old ) computeClearance( *corner, field );
				if ( old && old > metrics[*corner].get( field ) ) {
					if ( localAnnt.find (*corner) == localAnnt.end() ) {
						localAnnt[*corner] = old;
					}
					int x = corner->x, y  = corner->y;
					if ( x - 1 >= 0 ) {
						newLeftList->push_back( Vec2i(x-1,y) );
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
						newAboveList->push_back( Vec2i(x,y-1) );
					}
				}
				else {
					corner = NULL;
				}
			}
		}
		leftList->clear(); 
		leftList = newLeftList;
		aboveList->clear(); 
		aboveList = newAboveList;
	}// end while
}

/** Temporarily annotate the map, to treat unit as an obstacle
  * @param unit the unit to treat as an obstacle
  * @param field the field to annotate
  */
void AnnotatedMap::annotateUnit( const Unit *unit, const Field field ) {
	const int size = unit->getSize();
	const Vec2i &pos = unit->getPos();
	assert( theMap.isInside( pos ) );
	assert( theMap.isInside( pos.x + size - 1, pos.y + size - 1 ) );
	// first, re-evaluate the cells occupied
	for ( int i = size - 1; i >= 0 ; --i ) {
		for ( int j = size - 1; j >= 0; --j ) {
			Vec2i occPos = pos;
			occPos.x += i; occPos.y += j;
			if ( !unit->getType()->hasCellMap() || unit->getType()->getCellMapCell(i, j) ) {
				if ( localAnnt.find(occPos) == localAnnt.end() ) {
					localAnnt[occPos] = metrics[occPos].get(field);
				}
				metrics[occPos].set( field, 0 );
			}
			else {
				uint32 old =  metrics[occPos].get( field );
				computeClearance( occPos, field );
				if ( old != metrics[occPos].get( field ) && localAnnt.find( occPos ) == localAnnt.end() ) {
					localAnnt[occPos] = old;
				}
			}
		}
	}
	// propegate changes to left and above
	cascadingUpdate( pos, size, field );
}

/** Compute clearances (all fields) for a location
  * @param pos the cell co-ordinates 
  */
void AnnotatedMap::computeClearances( const Vec2i &pos ) {
	assert( theMap.isInside ( pos ) );
	assert( pos.x <= theMap.getW() - 2 );
	assert( pos.y <= theMap.getH() - 2 );

	Cell *cell = theMap.getCell( pos );

	// is there a building here, or an object on the tile ??
	bool surfaceBlocked = ( cell->getUnit( Zone::LAND ) && !cell->getUnit( Zone::LAND )->isMobile() )
							||   !theMap.getTile( theMap.toTileCoords( pos ) )->isFree ();
	// Walkable
	if ( surfaceBlocked || cell->isDeepSubmerged() )
		metrics[pos].set( Field::LAND, 0 );
	else
		computeClearance( pos, Field::LAND );
	// Any Water
	if ( surfaceBlocked || !cell->isSubmerged() )
		metrics[pos].set( Field::ANY_WATER, 0 );
	else
		computeClearance( pos, Field::ANY_WATER );
	// Deep Water
	if ( surfaceBlocked || !cell->isDeepSubmerged() )
		metrics[pos].set( Field::DEEP_WATER, 0 );
	else
		computeClearance( pos, Field::DEEP_WATER );
	// Amphibious:
	if ( surfaceBlocked )
		metrics[pos].set( Field::AMPHIBIOUS, 0 );
	else
		computeClearance( pos, Field::AMPHIBIOUS );
	// Air
	computeClearance( pos, Field::AIR );
}

/** Computes clearance based on metrics to the sout and east, Does NOT check if this cell is an obstactle,
  * assumes metrics of cells to the south, south-east & east are correct
  * @param pos the co-ordinates of the cell
  * @param field the field to update
  */
uint32 AnnotatedMap::computeClearance( const Vec2i &pos, Field f ) {
	uint32 clear = metrics[Vec2i( pos.x, pos.y + 1 )].get ( f );
	if ( clear > metrics[Vec2i( pos.x + 1, pos.y + 1 )].get ( f ) ) {
		clear = metrics[Vec2i( pos.x + 1, pos.y + 1 )].get ( f );
	}
	if ( clear > metrics[Vec2i( pos.x + 1, pos.y )].get ( f ) ) {
		clear = metrics[Vec2i( pos.x + 1, pos.y )].get ( f );
	}
	clear ++;
	if ( clear > maxClearanceValue )  {
		clear = maxClearanceValue;
	}
	metrics[pos].set( f, clear );
	return clear;
}
/** Perform 'local annotations', annotate the map to treat other mobile units in
  * the vincinity of unit as obstacles
  * @param unit the unit about to perform a search
  * @param field the field that the unit is about to search in
  */
void AnnotatedMap::annotateLocal( const Unit *unit, const Field field ) {
	PROFILE_LVL2_START("Local Annotations");
	const Vec2i &pos = unit->getPos();
	const int &size = unit->getSize();
	assert( theMap.isInside( pos ) );
	assert( theMap.isInside( pos.x + size - 1, pos.y + size - 1 ) );
	const int dist = 3;
	set<Unit*> annotate;

	// find surrounding units
	for ( int y = pos.y - dist; y < pos.y + size + dist; ++y ) {
		for ( int x = pos.x - dist; x < pos.x + size + dist; ++x ) {
			if ( metrics[Vec2i( x, y )].get( field ) ) { // clearance != 0
				Unit *u = theMap.getCell( x, y )->getUnit( field );
				if ( u && u != unit ) { // the set will take care of duplicates for us
					annotate.insert( u );
				}
			}
		}
	}
	// annotate map for each nearby unit
	for ( set<Unit*>::iterator it = annotate.begin(); it != annotate.end(); ++it ) {
		annotateUnit( *it, field );
	}
	PROFILE_LVL2_STOP("Local Annotations");
}

/** Clear all local annotations
  * @param field the field annotations were applied to
  */
void AnnotatedMap::clearLocalAnnotations( Field field ) {
	PROFILE_LVL2_START("Local Annotations");
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin(); it != localAnnt.end(); ++ it ) {
		assert( it->second <= maxClearanceValue );
		assert( theMap.isInside( it->first ) );
		metrics[it->first].set( field, it->second );
	}
	localAnnt.clear();
	PROFILE_LVL2_STOP("Local Annotations");
}

#if DEBUG_SEARCH_TEXTURES

list<pair<Vec2i,uint32>>* AnnotatedMap::getLocalAnnotations() {
	list<pair<Vec2i,uint32>> *ret = new list<pair<Vec2i,uint32>>();
	for ( map<Vec2i,uint32>::iterator it = localAnnt.begin (); it != localAnnt.end(); ++ it )
		ret->push_back( pair<Vec2i,uint32>(it->first,metrics[it->first].get(Field::LAND)) );
	return ret;
}

#endif

}}}
