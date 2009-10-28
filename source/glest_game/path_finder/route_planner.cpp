// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Does not use pre-compiled header because it's optimized in debug build.

#include <algorithm>

#include "game_constants.h"
#include "route_planner.h"

#include "search_engine.h"

#include "config.h"
#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"
#include "cartographer.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game { namespace Search {

// =====================================================
// 	class RoutePlanner
// =====================================================

// ===================== PUBLIC ========================

/** The RoutePlanner */
RoutePlanner* RoutePlanner::singleton = NULL;

/** @return a pointer to the RoutePlanner instance */
RoutePlanner* RoutePlanner::getInstance() {
	if ( ! singleton )
		singleton = new RoutePlanner();
	return singleton;
}

/** Construct RoutePlanner object */
RoutePlanner::RoutePlanner() {
	assert( !singleton );
	singleton = this;
}

/** delete SearchEngine objects and NodePool array*/
RoutePlanner::~RoutePlanner() {
	singleton = NULL;
	//TODO fix/reduce dodginess
	delete nsSearchEngine;
	delete nodePool;
}

/** construct and initialise the SeachEngine<NodePool> object and the NodePool array */
void RoutePlanner::init() {
	theLogger.add( "Initialising SearchEngine", true );
	nsSearchEngine = new SearchEngine<NodeStore>();
	nodePool = new NodePool();
	nsSearchEngine->getStorage()->attachNodePool(nodePool);

#if DEBUG_SEARCH_TEXTURES
	if ( Config::getInstance ().getMiscDebugTextures() ) {
		int foo = theConfig.getMiscDebugTextureMode ();
		debug_texture_action = foo == 0 ? RoutePlanner::SHOW_PATH_ONLY
							 : foo == 1 ? RoutePlanner::SHOW_OPEN_CLOSED_SETS
										: RoutePlanner::SHOW_LOCAL_ANNOTATIONS;
	}
#endif
}

/** Determine legality of a proposed move for a unit. This function is the absolute last say
  * in such matters.
  * @param unit the unit attempting to move
  * @param pos2 the position unit desires to move to
  * @return true if move may proceed, false if not legal.
  */
bool RoutePlanner::isLegalMove(Unit *unit, const Vec2i &pos2) const {
	assert(theMap.isInside(pos2));
	//assert(unit->getPos().dist(pos2) < 1.5);
	if ( unit->getPos().dist(pos2) > 1.5 ) {
		//TODO: figure out why we need this!  because blocked paths are popping...
		return false;
	}
	const Vec2i &pos1 = unit->getPos();
	const int &size = unit->getSize();
	const Field &field = unit->getCurrField();
	Zone zone = field == Field::AIR ? Zone::AIR : Zone::LAND;

	AnnotatedMap *annotatedMap = theWorld.getCartographer().getMasterMap();
	if ( ! annotatedMap->canOccupy(pos2, size, field) )
		return false; // obstruction in field
	if ( pos1.x != pos2.x && pos1.y != pos2.y ) {
		// Proposed move is diagonal, check if cells either 'side' are free.
		Vec2i diag1, diag2;
		getDiags(pos1, pos2, size, diag1, diag2);
		if ( ! annotatedMap->canOccupy(diag1, 1, field) 
		||   ! annotatedMap->canOccupy(diag2, 1, field) ) 
			return false; // obstruction, can not move to pos2
		if ( ! theMap.getCell(diag1)->isFree(zone)
		||   ! theMap.getCell(diag2)->isFree(zone) )
			return false; // other unit in the way
	}
	for ( int i = pos2.x; i < unit->getSize() + pos2.x; ++i )
		for ( int j = pos2.y; j < unit->getSize() + pos2.y; ++j )
			if ( theMap.getCell(i,j)->getUnit(zone) != unit
			&&   ! theMap.isFreeCell(Vec2i(i, j), field) )
				return false; // blocked by another unit
	// pos2 is free, and nothing is in the way
	return true;
}

/** Find a path to a location.
  * @param unit the unit requesting the path
  * @param finalPos the position the unit desires to go to
  * @return SearchResult::ARRIVED, SearchResult::MOVING or SearchResult::BLOCKED
  */
TravelState RoutePlanner::findPathToLocation(Unit *unit, const Vec2i &finalPos) {
	UnitPath &path = *unit->getPath();
	Vec2i pos;
	//if arrived (where we wanted to go)
	if( finalPos == unit->getPos() ) {
		unit->setCurrSkill(SkillClass::STOP);
		return TravelState::ARRIVED;
	} else if( ! path.empty() ) {	//route cache
		if ( attemptMove(unit) ) return TravelState::MOVING;
		if ( path.size() > 15  && repairPath(unit) ) {
			if ( attemptMove(unit) ) return TravelState::MOVING;
		}
	}
	//route cache miss and either no repair performed or repair failed
	const Vec2i &target = computeNearestFreePos(unit, finalPos); // set target for PosGoal Function
	//if arrived (as close as we can get to it)
	if ( target == unit->getPos() ) {
		unit->setCurrSkill(SkillClass::STOP);
		return TravelState::ARRIVED;
	}
	path.clear();
	
	// do a 'limited search'
	AnnotatedMap *aMap = theWorld.getCartographer().getAnnotatedMap(unit);
	aMap->annotateLocal(unit, unit->getCurrField()); // annotate map
	// reset nodepool and add start to open
	nsSearchEngine->setNodeLimit(NodePool::size);
	nsSearchEngine->setStart(unit->getPos(), DiagonalDistance(target)(unit->getPos())); 
	int result = nsSearchEngine->pathToPos(aMap, unit, target); // perform search
	aMap->clearLocalAnnotations(unit->getCurrField()); // clear annotations
	if ( result == AStarResult::FAILED ) BLOCKED()
	if ( result == AStarResult::PARTIAL ) {
		// Queue for 'complete' search...
	}
	// Partail or Complete... extract path...
	pos = nsSearchEngine->getGoalPos();
	while ( pos.x >= 0 ) {
		path.push(pos);
		pos = nsSearchEngine->getPreviousPos(pos);
	}
	if ( path.size() ) path.pop();
	if ( path.empty() ) {
		unit->setCurrSkill(SkillClass::STOP);
		return TravelState::ARRIVED;
	}
	if ( attemptMove(unit) ) return TravelState::MOVING; // should always succeed
	unit->setCurrSkill(SkillClass::STOP);
	path.incBlockCount();
	return TravelState::BLOCKED;
}

/** repair a blocked path
  * @param unit unit whose path is blocked 
  * @return true if repair succeeded */
bool RoutePlanner::repairPath(Unit *unit) {
	UnitPath &path = *unit->getPath();
	assert(path.size() > 12);
	int i=8;
	while ( i-- ) {
		path.pop();
	}
	AnnotatedMap *aMap = theWorld.getCartographer().getAnnotatedMap(unit);
	aMap->annotateLocal(unit, unit->getCurrField());
	// reset nodepool and add start to open
	nsSearchEngine->setNodeLimit(256);
	nsSearchEngine->setStart(unit->getPos(), DiagonalDistance(path.peek())(unit->getPos()));
	int result = nsSearchEngine->pathToPos(aMap, unit, path.peek()); // perform search
	aMap->clearLocalAnnotations(unit->getCurrField());

	if ( result == AStarResult::COMPLETE ) {
		Vec2i pos = nsSearchEngine->getGoalPos();
		// skip target (is on the 'front' of the path already...
		pos = nsSearchEngine->getPreviousPos(pos);
		while ( pos.x >= 0 ) {
			path.push(pos);
			pos = nsSearchEngine->getPreviousPos(pos);
		}
		return true;
	}
	path.clear();
	return false;
}

// ==================== PRIVATE ====================

// return finalPos if free, else a nearest free pos within maxFreeSearchRadius
// cells, or unit's current position if none found
//
// Move me to Cartographer, as findFreePos(), rewrite using a Dijkstra Search
//
/** find nearest free position a unit can occupy 
  * @param unit the unit in question
  * @param finalPos the position unit wishes to be
  * @return finalPos if free and occupyable by unit, else the closest such position, or the unit's 
  * current position if none found
  * @todo reimplement with Dijkstra search
  */
Vec2i RoutePlanner::computeNearestFreePos(const Unit *unit, const Vec2i &finalPos) {
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
	Field field = unit->getCurrField();
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	if ( theMap.areAproxFreeCells(finalPos, size, field, teamIndex) ) {
		return finalPos;
	}

	//find nearest pos

	// Local annotate target if visible
	// set 

	// REPLACE ME!
	//
	// Use the new super-dooper SearchEngine, do a Dijkstra search from target, 
	// with a GoalFunc that returns true if the cell is unoccupid (and clearnce > unit.size).
	// Now that's efficient... ;-)

	Vec2i nearestPos = unitPos;
	float nearestDist = unitPos.dist(finalPos);
	for ( int i = -maxFreeSearchRadius; i <= maxFreeSearchRadius; ++i ) {
		for ( int j = -maxFreeSearchRadius; j <= maxFreeSearchRadius; ++j ) {
			Vec2i currPos = finalPos + Vec2i(i, j);
			if ( theMap.areAproxFreeCells(currPos, size, field, teamIndex) ){
				float dist = currPos.dist(finalPos);

				//if nearer from finalPos
				if ( dist < nearestDist ) {
					nearestPos = currPos;
					nearestDist = dist;
				}
				//if the distance is the same compare distance to unit
				else if ( dist == nearestDist ) {
					if ( currPos.dist(unitPos) < nearestPos.dist(unitPos) )
						nearestPos = currPos;
				}
			}
		}
	}
	return nearestPos;
}

} // end namespace Glest::Game::PathFinder

#if defined DEBUG && defined VALIDATE_MAP
// Why is this here?  Doesn't it belong in world.cpp?  It's here because we compile path_finder.cpp
// optimized in debug since it's the only possible way you can really debug and this is a dog slow
// function.
class ValidationMap {
public:
	int h;
	int w;
	char *cells;

	ValidationMap(int h, int w) : h(h), w(w), cells(new char[h * w * Zone::COUNT]) {
		reset();
	}

	void reset() {
		memset(cells, 0, h * w * Zone::COUNT);
	}

	void validate(int x, int y, Zone zone) {
		assert(!getCell(x, y, zone));
		getCell(x, y, zone) = 1;
	}

	char &getCell(int x, int y, Zone zone) {
		assert(x >= 0 && x < w);
		assert(y >= 0 && y < h);
		assert(zone >= 0 && zone < Zone::COUNT);
		return cells[zone * h * w + x * w + y];
	}
};

void World::assertConsistiency() {
	// go through each map cell and make sure each unit in cells are supposed to be there
	// go through each unit and make sure they are in the cells they are supposed to be in
	// iterate through unit references: pets, master, target and make sure they are valid or
	//		null-references
	// make sure alive/dead states of all units is good.
	// whatever else I can think of

	static ValidationMap validationMap(map.getH(), map.getW());
	validationMap.reset();

	// make sure that every unit is in their cells and mark those as validated.
	for(Factions::iterator fi = factions.begin(); fi != factions.end(); ++fi) {
		for(int ui = 0; ui < fi->getUnitCount(); ++ui) {
			Unit *unit = fi->getUnit(ui);
/*
			if(!((unit->getHp() == 0 && unit->isDead()) || (unit->getHp() > 0 && unit->isAlive()))) {
				cerr << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout.flush();
				assert(false);
			}
*/
			if(unit->isDead() && unit->getCurrSkill()->getClass() == SkillClass::DIE) {
				continue;
			}

			const UnitType *ut = unit->getType();
			int size = ut->getSize();
			Zone zone = unit->getCurrZone ();
			const Vec2i &pos = unit->getPos();

			for(int x = 0; x < size; ++x) {
				for(int y = 0; y < size; ++y) {
					Vec2i currPos = pos + Vec2i(x, y);
					assert(map.isInside(currPos));

					if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
						Unit *unitInCell = map.getCell(currPos)->getUnit(zone);
						if(unitInCell != unit) {
							cerr << "Unit id " << unit->getId()
									<< " from faction " << fi->getIndex()
									<< "(type = " << unit->getType()->getName() << ")"
									<< " not in cells (" << currPos.x << ", " << currPos.y << ", " << zone << ")";
							if(unitInCell == NULL && !unit->getHp()) {
								cerr << " but has zero HP and is not executing SkillClass::DIE." << endl;
							} else {
								cerr << endl;
								assert(false);
							}
						}
						validationMap.validate(currPos.x, currPos.y, zone);
					}
				}
			}
		}
	}

	// make sure that every cell that was not validated is empty
	for(int x = 0; x < map.getW(); ++x) {
		for(int y = 0; y < map.getH(); ++y ) {
			for(int zone = 0; zone < Zone::COUNT; ++zone) {
				if(!validationMap.getCell(x, y, (Zone)zone)) {
					Cell *cell = map.getCell(x, y);
					if(cell->getUnit((Zone)zone)) {
						cerr << "Cell not empty at " << x << ", " << y << ", " << zone << endl;
						cerr << "Cell has pointer to unit object at " << cell->getUnit((Zone)zone) << endl;

						assert(false);
					}
				}
			}
		}
	}
}
#else
void World::assertConsistiency() {}
#endif

void World::doHackyCleanUp() {
	for ( Units::const_iterator u = newlydead.begin(); u != newlydead.end(); ++u ) {
		Unit &unit = **u;
		for ( int i=0; i < unit.getSize(); ++i ) {
			for ( int j=0; j < unit.getSize(); ++j ) {
				Cell *cell = map.getCell(unit.getPos() + Vec2i(i,j));
				if ( cell->getUnit(unit.getCurrZone()) == &unit ) {
					cell->setUnit(unit.getCurrZone(), NULL);
				}
			}
		}
	}
	newlydead.clear();
}


}} //end namespace
