// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Does not use pre-compiled header because it's optimized in debug build.

#include <algorithm>
#include <list>

#include "game_constants.h"
#include "route_planner.h"
#include "cartographer.h"
#include "search_engine.h"

#include "config.h"
#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "leak_dumper.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game { namespace Search {

int GridNeighbours::x = 0;
int GridNeighbours::y = 0;
int GridNeighbours::height = 0;
int GridNeighbours::width = 0;

// =====================================================
// 	class RoutePlanner
// =====================================================

// ===================== PUBLIC ========================

/** Construct RoutePlanner object */
RoutePlanner::RoutePlanner(World *world)
		: world(world)
		, nodeStore(NULL)
		, tNodeStore(NULL)
		, tSearchEngine(NULL)
		, nsgSearchEngine(NULL) {
	theLogger.add( "Initialising SearchEngine", true );

	nodeStore = new NodeStore(world->getMap()->getW(), world->getMap()->getH());
	nsgSearchEngine = new SearchEngine<NodeStore>(nodeStore, true);
	nsgSearchEngine->setInvalidKey(Vec2i(-1));
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);

	tNodeStore = new TransitionNodeStore(2048); //FIXME (size)
	tSearchEngine = new TransitionSearchEngine(tNodeStore, true);
	tSearchEngine->setInvalidKey(NULL);
}

/** delete SearchEngine objects */
RoutePlanner::~RoutePlanner() {
	delete nsgSearchEngine;
	delete tSearchEngine;
}

/** Determine legality of a proposed move for a unit. This function is the absolute last say
  * in such matters.
  * @param unit the unit attempting to move
  * @param pos2 the position unit desires to move to
  * @return true if move may proceed, false if not legal.
  */
bool RoutePlanner::isLegalMove(Unit *unit, const Vec2i &pos2) const {
	assert(world->getMap()->isInside(pos2));
	//assert(unit->getPos().dist(pos2) < 1.5);
	if (unit->getPos().dist(pos2) > 1.5) {
		//TODO: figure out why we need this!  because blocked paths are popping...
		return false;
	}
	const Vec2i &pos1 = unit->getPos();
	const int &size = unit->getSize();
	const Field &field = unit->getCurrField();
	Zone zone = field == Field::AIR ? Zone::AIR : Zone::LAND;

	AnnotatedMap *annotatedMap = world->getCartographer()->getMasterMap();
	if (!annotatedMap->canOccupy(pos2, size, field)) {
		return false; // obstruction in field
	}
	if ( pos1.x != pos2.x && pos1.y != pos2.y ) {
		// Proposed move is diagonal, check if cells either 'side' are free.
		//  eg..  XXXX
		//        X1FX  The Cells marked 'F' must both be free
		//        XF2X  for the move 1->2 to be legit
		//        XXXX
		Vec2i diag1, diag2;
		getDiags(pos1, pos2, size, diag1, diag2);
		if (!annotatedMap->canOccupy(diag1, 1, field) || !annotatedMap->canOccupy(diag2, 1, field)
		|| !world->getMap()->getCell(diag1)->isFree(zone)
		|| !world->getMap()->getCell(diag2)->isFree(zone)) {
			return false; // obstruction, can not move to pos2
		}
	}
	for (int i = pos2.x; i < unit->getSize() + pos2.x; ++i) {
		for (int j = pos2.y; j < unit->getSize() + pos2.y; ++j) {
			if (world->getMap()->getCell(i,j)->getUnit(zone) != unit
			&& !world->getMap()->isFreeCell(Vec2i(i, j), field)) {
				return false; // blocked by another unit
			}
		}
	}
	// pos2 is free, and nothing is in the way
	return true;
}

#define ASTAR_LOWLEVEL aStar<PosGoal,MoveCost,DiagonalDistance>

float RoutePlanner::quickSearch(Field field, int size, const Vec2i &start, const Vec2i &dest) {
	// setup search
	MoveCost moveCost(field, size, world->getCartographer()->getMasterMap());
	DiagonalDistance heuristic(dest);
	nsgSearchEngine->setStart(start, heuristic(start));

	AStarResult r = nsgSearchEngine->ASTAR_LOWLEVEL(PosGoal(dest), moveCost, heuristic);
	if (r == AStarResult::COMPLETE && nsgSearchEngine->getGoalPos() == dest) {
		return nsgSearchEngine->getCostTo(dest);
	}
	return numeric_limits<float>::infinity();
}

bool RoutePlanner::setupHierarchicalSearch(Unit *unit, const Vec2i &dest, TransitionGoal &goalFunc) {
	// get Transitions for start cluster
	Transitions transitions;
	Vec2i startCluster = ClusterMap::cellToCluster(unit->getPos());
	ClusterMap *clusterMap = world->getCartographer()->getClusterMap();
	clusterMap->getTransitions(startCluster, unit->getCurrField(), transitions);

	DiagonalDistance dd(dest);
	GridNeighbours::setSearchCluster(startCluster);

	bool startTrans = false;
	// attempt quick path from unit->pos to each transition, 
	// if successful add transition to open list
	for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
		float cost = quickSearch(unit->getCurrField(), unit->getSize(), unit->getPos(), (*it)->nwPos);
		if (cost != numeric_limits<float>::infinity()) {
			tSearchEngine->setOpen(*it, dd((*it)->nwPos), cost);
			startTrans = true;
		}
	}
	if (!startTrans) {
		return false;
	}

	// transitions to goal
	transitions.clear();
	Vec2i cluster = ClusterMap::cellToCluster(dest);
	clusterMap->getTransitions(cluster, unit->getCurrField(), transitions);

	GridNeighbours::setSearchCluster(cluster);

	// attempt quick path from dest to each transition, 
	// if successful add transition to goal set
	for (Transitions::iterator it = transitions.begin(); it != transitions.end(); ++it) {
		float cost = quickSearch(unit->getCurrField(), unit->getSize(), dest, (*it)->nwPos);
		if (cost != numeric_limits<float>::infinity()) {
			goalFunc.goalTransitions().insert(*it);
		}
	}
	if (goalFunc.goalTransitions().empty()) {
		return false;
	}
	return true;
}

#define ASTAR_HIERARCHICAL aStar<TransitionGoal,TransitionCost,TransitionHeuristic>

bool RoutePlanner::findWaypointPath(Unit *unit, const Vec2i &dest, WaypointPath &waypoints) {
	TransitionGoal goal;
	if ( !setupHierarchicalSearch(unit, dest, goal) ) {
		GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
		theConsole.addLine("setupHierarchicalSearch() Failed.");
		return false;
	}
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
	TransitionCost cost(unit->getCurrField(), unit->getSize());
	TransitionHeuristic heuristic(dest);
	AStarResult res = tSearchEngine->ASTAR_HIERARCHICAL(goal,cost,heuristic);
	if ( res == AStarResult::COMPLETE ) {
		WaypointPath &wpPath = *unit->getWaypointPath();
		assert(wpPath.empty());
		waypoints.push(dest);
		const Transition *t = tSearchEngine->getGoalPos();
		while ( t ) {
			waypoints.push( t->nwPos );
			t = tSearchEngine->getPreviousPos(t);
		}
		return true;
	}
	theConsole.addLine("SearchEngine::aStar<>() [Hierarchical] failed.");
	return false;
}

#if _GAE_DEBUG_EDITION_
	void collectOpenClosed(SearchEngine<NodeStore> *se) {
		NodeStore* nm = se->getStorage();
		list<Vec2i> *nodes = nm->getOpenNodes();
		list<Vec2i>::iterator nit = nodes->begin();
		for ( ; nit != nodes->end(); ++nit ) 
			PathFinderTextureCallBack::openSet.insert(*nit);
		delete nodes;
		nodes = nm->getClosedNodes();
		for ( nit = nodes->begin(); nit != nodes->end(); ++nit )
			PathFinderTextureCallBack::closedSet.insert(*nit);
		delete nodes;					
	}

	void clearOpenClosed(const Vec2i &start, const Vec2i &target) {
		PathFinderTextureCallBack::pathStart = start;
		PathFinderTextureCallBack::pathDest = target;
		PathFinderTextureCallBack::pathSet.clear();
		PathFinderTextureCallBack::openSet.clear();
		PathFinderTextureCallBack::closedSet.clear();
	}

#	define IF_DEBUG_TEXTURES(x) { x }
#else
#	define IF_DEBUG_TEXTURES(x) {}
#endif  // _GAE_DEBUG_EDITION_

ostream& operator<<(ostream& lhs, Vec2i &rhs) {
	return lhs << "(" << rhs.x << "," << rhs.y << ")";
}

bool RoutePlanner::refinePath(Unit *unit) {
	WaypointPath &wpPath = *unit->getWaypointPath();
	UnitPath &path = *unit->getPath();
	assert(!wpPath.empty());

	const Vec2i &startPos = path.empty() ? unit->getPos() : path.back();
	const Vec2i &destPos = wpPath.front();
	AnnotatedMap *aMap = theWorld.getCartographer()->getAnnotatedMap(unit);
	//SearchEngine<NodeMap,GridNeighbours> *se = theWorld.getCartographer()->getSearchEngine();
	
	MoveCost cost(unit, aMap);
	DiagonalDistance dd(destPos);
	PosGoal posGoal(destPos);

	nsgSearchEngine->setStart(startPos, dd(startPos));
	AStarResult res = nsgSearchEngine->ASTAR_LOWLEVEL(posGoal, cost, dd);
	if (res != AStarResult::COMPLETE) {
		return false;
	}
	IF_DEBUG_TEXTURES( collectOpenClosed(nsgSearchEngine); )
	// extract path
	Vec2i pos = nsgSearchEngine->getGoalPos();
	assert(pos == destPos);
	list<Vec2i>::iterator it = path.end();
	while (pos.x != -1) {
		it = path.insert(it, pos);
		pos = nsgSearchEngine->getPreviousPos(pos);
	}
	// erase start point (already on path or is start pos)
	it = path.erase(it);
	// pop waypoint
	wpPath.pop();
	return true;
}

#undef max

void RoutePlanner::smoothPath(Unit *unit) {
	AnnotatedMap* const &aMap = world->getCartographer()->getMasterMap();
	int min_x = numeric_limits<int>::max(), 
		max_x = -1, 
		min_y = numeric_limits<int>::max(), 
		max_y = -1;
	set<Vec2i> onPath;
	UnitPath::iterator it = unit->getPath()->begin();
	for ( ; it  != unit->getPath()->end(); ++it) {
		if (it->x < min_x) min_x = it->x;
		if (it->x > max_x) max_x = it->x;
		if (it->y < min_y) min_y = it->y;
		if (it->y > max_y) max_y = it->y;
		onPath.insert(*it);
	}
	assert(unit->getPath()->size() > 1);
	Rect2i bounds(min_x, min_y, max_x + 1, max_y + 1);

	it = unit->getPath()->begin();
	UnitPath::iterator nit = it;
	++nit;

	while (nit != unit->getPath()->end()) {
		onPath.erase(*it);
		Vec2i sp = *it;
		for (OrdinalDir d(0); d < OrdinalDir::COUNT; ++d) {
			Vec2i p = *it + OrdinalOffsets[d];
			if (p == *nit) continue;
			Vec2i intersect(-1);
			while (bounds.isInside(p)) {
				if (!aMap->canOccupy(p, unit->getSize(), unit->getCurrField())) {
					break;
				}
				if (d % 2 == 1) { // diagonal
					Vec2i off1, off2;
					getDiags(p - OrdinalOffsets[d], p, unit->getSize(), off1, off2);
					if (!aMap->canOccupy(off1, 1, unit->getCurrField())
					|| !aMap->canOccupy(off2, 1, unit->getCurrField())) {
						break;
					}
				}
				if (onPath.find(p) != onPath.end()) {
					intersect = p;
					break;
				}
				p += OrdinalOffsets[d];
			}
			if (intersect != Vec2i(-1)) {
				UnitPath::iterator eit = nit;
				while (*eit != intersect) {
					onPath.erase(*eit++);
				}
				nit = unit->getPath()->erase(nit, eit);
				sp += OrdinalOffsets[d];
				while (sp != intersect) {
					unit->getPath()->insert(nit, sp);
					onPath.insert(sp); // do we need this? Can these get us further hits ??
					sp += OrdinalOffsets[d];
				}
				break; // break foreach direction
			} // if shortcut
		} // foreach direction
		nit = ++it;
		++nit;
	}
}


/** Find a path to a location.
  * @param unit the unit requesting the path
  * @param finalPos the position the unit desires to go to
  * @return ARRIVED, MOVING, BLOCKED or IMPOSSIBLE
  */
TravelState RoutePlanner::findPathToLocation(Unit *unit, const Vec2i &finalPos) {
	UnitPath &path = *unit->getPath();
	WaypointPath &wpPath = *unit->getWaypointPath();

	// if arrived (where we wanted to go)
	if(finalPos == unit->getPos()) {
		unit->setCurrSkill(SkillClass::STOP);
		return TravelState::ARRIVED;
	} else if (!path.empty()) { // route cache
		assert(unit->getPos().dist(path.front()) < 1.5f);
		if (attemptMove(unit)) {
			if (!wpPath.empty() && path.size() < 12) {
				// if there are less than 12 steps left on this path, and there are more waypoints
				IF_DEBUG_TEXTURES( clearOpenClosed(unit->getPos(), finalPos); )
				while (!wpPath.empty() && path.size() < 20) {
					// refine path to at least 20 steps (or end of path)
					if (!refinePath(unit)) {
						theConsole.addLine("refinePath() failure.");
						theLogger.add("refinePath() failure.");
						break;
					}
				}
				IF_DEBUG_TEXTURES
				(	for (UnitPath::iterator pit = path.begin(); pit != path.end(); ++pit) 
						PathFinderTextureCallBack::pathSet.insert(*pit);
				)
				smoothPath(unit);
			}
			return TravelState::MOVING;
		}
		// path blocked, quickSearch to next waypoint...
		theConsole.addLine("Blocked!");

		//if (path.size() > 15  && repairPath(unit)) { // repair ?? (ditch this??)
		//	if (attemptMove(unit)) {
		//		return TravelState::MOVING;
		//	}
		//}
	}
	// route cache miss or blocked
	
	const Vec2i &target = computeNearestFreePos(unit, finalPos); // set target for PosGoal Function
	// if arrived (as close as we can get to it)
	if (target == unit->getPos()) {
		unit->setCurrSkill(SkillClass::STOP);
		return TravelState::ARRIVED;
	}
	path.clear();
	wpPath.clear();

	Vec2i startCluster = ClusterMap::cellToCluster(unit->getPos());
	Vec2i destCluster  = ClusterMap::cellToCluster(target);

	if (startCluster.dist(destCluster) < 3.f) {
		if (quickSearch(unit->getCurrField(), unit->getSize(), unit->getPos(), target)
		!= numeric_limits<float>::infinity()) {
			Vec2i pos = nsgSearchEngine->getGoalPos();
			while (pos.x != -1) {
				path.push_front(pos);
				pos = nsgSearchEngine->getPreviousPos(pos);
			}
			if (!path.empty()) path.pop();
			if (attemptMove(unit)) {
				return TravelState::MOVING;
			}
			path.clear();
			// else fall through and try hierarchical
		}
	}

	// Hierarchical Search
	tSearchEngine->reset();
	if (!findWaypointPath(unit, target, wpPath)) {
		if (unit->getFaction()->isThisFaction()) {
			theConsole.addLine("Destination unreachable?");
		}
		return TravelState::IMPOSSIBLE;
	}
	IF_DEBUG_TEXTURES
	(	DebugRenderer::clearWaypoints();
		WaypointPath::iterator it = wpPath.begin();
		for ( ; it != wpPath.end(); ++it) {
			Vec3f vert = world->getMap()->getTile(Map::toTileCoords(*it))->getVertex();
			vert.x += it->x % Map::cellScale + 0.5f;
			vert.z += it->y % Map::cellScale + 0.5f;
			vert.y += 0.25f;
			DebugRenderer::addWaypoint(vert);
		}
		clearOpenClosed(unit->getPos(), target);
	)
	
	//TODO post process, scan wpPath, if prev.dist(pos) < 4 cull prev
	assert(wpPath.size() > 1);
	wpPath.pop();

	// refine path, to at least 20 steps (or end of path)
	while (!wpPath.empty() && path.size() < 20) {
		if (!refinePath(unit)) {
			theLogger.add("refinePath failed!");
			return TravelState::BLOCKED;
		}
	}
	smoothPath(unit);

	IF_DEBUG_TEXTURES
	(	for (UnitPath::iterator pit = path.begin(); pit != path.end(); ++pit) 
			PathFinderTextureCallBack::pathSet.insert(*pit);
	)
	if (attemptMove(unit)) {
		return TravelState::MOVING;
	}

	theLogger.add("Hierarchical refined path blocked ? valid ?!?");
	unit->setCurrSkill(SkillClass::STOP);
	return TravelState::INVALID;


	/*
	// Low Level Search with NodeMap (this is for testing purposes only, not node limited)

	// this is the 'straight' A* using the NodeMap (no node limit)
	AnnotatedMap *aMap = theWorld.getCartographer()->getAnnotatedMap(unit);
	SearchEngine<NodeMap> *se = theWorld.getCartographer()->getSearchEngine();
	DiagonalDistance dd(target);
	MoveCost cost(unit, aMap);
	PosGoal goal(target);
	se->setNodeLimit(-1);
	se->setStart(unit->getPos(), dd(unit->getPos()));
	AStarResult res = se->ASTAR_TO_POS(goal,cost,dd);
	list<Vec2i>::iterator it;
	IF_DEBUG_TEXTURES
	( 
		list<Vec2i> *nodes = NULL;
		NodeMap* nm = se->getStorage();
	)
	Vec2i goalPos, pos;
	switch (res) {
		case AStarResult::COMPLETE:
			goalPos = se->getGoalPos();
			pos = goalPos;
			while (pos.x != -1) {
				path.push(pos);
				pos = se->getPreviousPos(pos);
			}
			if ( path.size() ) path.pop();
			IF_DEBUG_TEXTURES( collectOpenClosed(se); )
			IF_DEBUG_TEXTURES
					(	UnitPath::iterator pit = path.begin();
						for ( ; pit != path.end(); ++pit ) 
							PathFinderTextureCallBack::pathSet.insert(*pit);
					)
			// it's a horrible mess atm the moment anyway... leave me alone! :P
			goto End;
			break; // case AStarResult::COMPLETE
		case AStarResult::FAILED:
			wpPath.clear();
			return TravelState::IMPOSSIBLE;
			break;
		default:
			throw runtime_error("Something that shouldn't have happened, did happen :(");
			break;
	}
	*/


	/*
	// Low Level Search with NodeStore
	path.clear();
	wpPath.clear();
	// do a 'limited search'
	AnnotatedMap *aMap = theWorld.getCartographer()->getAnnotatedMap(unit);
	aMap->annotateLocal(unit, unit->getCurrField()); // annotate map
	// reset nodepool and add start to open
	nsgSearchEngine->setNodeLimit(NodePool::size);
	nsgSearchEngine->setStart(unit->getPos(), DiagonalDistance(target)(unit->getPos())); 
	AStarResult result = nsgSearchEngine->pathToPos(aMap, unit, target); // perform search
	aMap->clearLocalAnnotations(unit->getCurrField()); // clear annotations
	if (result == AStarResult::FAILED) {
		unit->setCurrSkill(SkillClass::STOP);
		path.incBlockCount();
		return TravelState::IMPOSSIBLE;
	}
	if (result == AStarResult::NODE_LIMIT) {
		theLogger.add("non hierarchical search was node limited.");
		// Queue for 'complete' search...
	}
	// Partail or Complete... extract path...
	Vec2i pos = nsgSearchEngine->getGoalPos();
	while (pos.x >= 0) {
		path.push(pos);
		pos = nsgSearchEngine->getPreviousPos(pos);
	}
	if (path.size()) path.pop();

End:
	if (path.empty()) {
		unit->setCurrSkill(SkillClass::STOP);
		return TravelState::ARRIVED;
	}
	if (attemptMove(unit)) return TravelState::MOVING; // should always succeed
	unit->setCurrSkill(SkillClass::STOP);
	path.incBlockCount();
	return TravelState::BLOCKED;
	*/
}

/** repair a blocked path
  * @param unit unit whose path is blocked 
  * @return true if repair succeeded */
bool RoutePlanner::repairPath(Unit *unit) {
	
	/*
	UnitPath &path = *unit->getPath();
	assert(path.size() > 12);
	int i=8;
	while ( i-- ) {
		path.pop();
	}
	AnnotatedMap *aMap = theWorld.getCartographer()->getAnnotatedMap(unit);
	aMap->annotateLocal(unit, unit->getCurrField());
	// reset nodepool and add start to open
	nsgSearchEngine->setNodeLimit(256);
	nsgSearchEngine->setStart(unit->getPos(), DiagonalDistance(path.peek())(unit->getPos()));
	int result = nsgSearchEngine->pathToPos(aMap, unit, path.peek()); // perform search
	aMap->clearLocalAnnotations(unit->getCurrField());

	if ( result == AStarResult::COMPLETE ) {
		Vec2i pos = nsgSearchEngine->getGoalPos();
		// skip target (is on the 'front' of the path already...
		pos = nsgSearchEngine->getPreviousPos(pos);
		while ( pos.x >= 0 ) {
			path.push(pos);
			pos = nsgSearchEngine->getPreviousPos(pos);
		}
		return true;
	}
	path.clear();
	return false;
	*/
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
	if ( world->getMap()->areAproxFreeCells(finalPos, size, field, teamIndex) ) {
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
			if ( world->getMap()->areAproxFreeCells(currPos, size, field, teamIndex) ){
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

} // end namespace Glest::Game::Search

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
