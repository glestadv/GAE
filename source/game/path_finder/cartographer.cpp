// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Does not use pre-compiled header because it's optimized in debug build.

#include <algorithm>

#include "game_constants.h"
#include "cartographer.h"

#include "search_engine.h"
#include "cluster_map.h"

#include "map.h"
#include "game.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game { namespace Search {

/** Construct Cartographer object. Requires game settings, factions & cell map to have been loaded.
  */
Cartographer::Cartographer(World *world)
		: world(world) {
	theLogger.add("Cartographer", true);

	cellMap = world->getMap();
	int w = cellMap->getW(), h = cellMap->getH();

	cout << "NodeMap SearchEngine\n";
	nodeMap = new NodeMap(w, h);
	nmSearchEngine = new SearchEngine<NodeMap, GridNeighbours>();
	nmSearchEngine->setStorage(nodeMap);
	nmSearchEngine->setInvalidKey(Vec2i(-1));
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);

	cout << "Annotated Map\n";
	masterMap = new AnnotatedMap(world);
	
	cout << "Cluster Map\n";
	clusterMap = new ClusterMap(masterMap, this);

	cout << "Exploration Maps\n";
	// team search and visibility maps
	set<int> teams;
	for (int i=0; i < world->getFactionCount(); ++i) {
		const Faction *f = world->getFaction(i);
		int team = f->getTeam();
		if (teams.find(team) == teams.end()) {
			teams.insert(team);
			// AnnotatedMap needs to be 'bound' to explored status
			explorationMaps[team] = new ExplorationMap(cellMap);
			//teamMaps[team] = new AnnotatedMap(world, explorationMaps[team]);
		}
	}

	// Todo: more preprocessing. Discover all harvester units, 
	// check field, harvest reange and resource types...
	//
	cout << "Resource Catalog\n";
	// find and catalog all resources...
	for ( int x=0; x < cellMap->getTileW() - 1; ++x ) {
		for ( int y=0; y < cellMap->getTileH() - 1; ++y ) {
			const Resource * const r = cellMap->getTile(x,y)->getResource();
			if ( r ) {
				resourceLocations[r->getType()].push_back(Vec2i(x,y));
			}
		}
	}

	const TechTree *tt = world->getTechTree();
	for (int i=0; i < tt->getResourceTypeCount(); ++i) {
		const ResourceType *rt = tt->getResourceType(i);
		if (rt->getClass() == ResourceClass::TECHTREE || rt->getClass() == ResourceClass::TILESET) {
			Rectangle rect(0, 0, cellMap->getW() - 2, cellMap->getH() - 2);
			PatchMap<1> *pMap = new PatchMap<1>(rect, 0);
			initResourceMap(rt, pMap);
			resourceMaps[rt] = pMap;
		}
	}

	/*
	// construct resource influence maps for each team
	for ( int i=0; i < world->getTechTree()->getResourceTypeCount(); ++i ) {
		const ResourceType* rt = world->getTechTree()->getResourceType( i );
		if ( rt->getClass() == ResourceClass::TECH || rt->getClass() == ResourceClass::TILESET ) {
			for ( int j=0; j < world->getFactionCount(); ++j ) {
				int team = world->getFaction(j)->getTeam();
				if ( teamResourceMaps[team].find(rt) == teamResourceMaps[team].end() ) {
					teamResourceMaps[team][rt] = new TypeMap<float>(Rect(0,0,cellMap->getW(),cellMap->getH()), -1.f);
				}
			}
		}
	}
	*/
}

/** Destruct */
Cartographer::~Cartographer() {
	delete masterMap;
	delete nmSearchEngine;
	delete nodeMap;

	// Team Annotated Maps
	/*map<int,AnnotatedMap*>::iterator aMapIt = teamMaps.begin();
	for ( ; aMapIt != teamMaps.end(); ++aMapIt ) {
		delete aMapIt->second;
	}
	teamMaps.clear();*/

	// Exploration Maps
	map<int,ExplorationMap*>::iterator eMapIt = explorationMaps.begin();
	for ( ; eMapIt != explorationMaps.end(); ++eMapIt ) {
		delete eMapIt->second;
	}
	explorationMaps.clear();
	
	// Resource Maps
	/*
	map<int,map<const ResourceType*, TypeMap<float>*>>::iterator teamIt;
	map<const ResourceType*, TypeMap<float>*>::iterator iMapIt;
	teamIt = teamResourceMaps.begin();
	for ( ; teamIt != teamResourceMaps.end(); ++teamIt ) {
		iMapIt = teamIt->second.begin();
		for ( ; iMapIt != teamIt->second.end(); ++iMapIt ) {
			delete iMapIt->second;
		}
	}
	teamResourceMaps.clear();*/
}

/** WIP */
void Cartographer::updateResourceMaps() {
	/*set< int > seen;
	for ( int j=0; j < world->getFactionCount(); ++j ) {
		int team = world->getFaction( j )->getTeam();
		if ( seen.find( team ) != seen.end() ) {
			continue;
		}
		seen.insert( team );
		for ( int i=0; i < world->getTechTree()->getResourceTypeCount(); ++i ) {
			const ResourceType* rt = world->getTechTree()->getResourceType( i );
			if ( rt->getClass() == ResourceClass::TECH || rt->getClass() == ResourceClass::TILESET ) {
				initResourceMap( team, rt, teamResourceMaps[team][rt] );
			}
		}
	}*/
}

/** custom goal function for building resource map */
class ResourceMapBuilderGoal {
private:
	float range;
	PatchMap<1> *pMap;
public:
	ResourceMapBuilderGoal(float range, PatchMap<1> *pMap)
			: range(range), pMap(pMap) {}
	bool operator()(const Vec2i &pos, const float costSoFar) const {
		if (costSoFar > range) {
			return true;
		}
		if (costSoFar == 0.f) {
			return false;
		}
		// else in range
		pMap->setInfluence(pos, 1);
		return false;
	}
};

/** custom cost function for building resource map */
class ResourceMapBuilderCost {
private:
	Field field;
	int size;
	const AnnotatedMap *aMap;

public:
	ResourceMapBuilderCost(const Field field, const int size, const AnnotatedMap *aMap )
		: field(field), size(size), aMap(aMap) {}

	float operator()(const Vec2i &p1, const Vec2i &p2) const {
		if (!aMap->canOccupy(p2, size, field)) {
			return numeric_limits<float>::infinity();
		}
		if (p1.x != p2.x && p1.y != p2.y) {
			return SQRT2;
		}
		return 1.f;
	}
};

#define BUILD_RESOURCE_MAP aStar<ResourceMapBuilderGoal, ResourceMapBuilderCost, ZeroHeuristic>

/** WIP */
void Cartographer::initResourceMap(const ResourceType *rt, PatchMap<1> *pMap) {
	
	static char buf[512];
	char *ptr = buf;
	ptr += sprintf(ptr, "Initialising resource map : %s.\n", rt->getName().c_str());
	int64 time = Chrono::getCurMillis();

	pMap->zeroMap();
	nmSearchEngine->setNodeLimit(-1);
	nmSearchEngine->reset();
	vector<Vec2i>::iterator it = resourceLocations[rt].begin();
	int nt = 0;
	for ( ; it != resourceLocations[rt].end(); ++it) {
		Vec2i pos = *it * Map::cellScale;
		nmSearchEngine->setOpen(pos, 0.f);
		nmSearchEngine->setOpen(pos + Vec2i(0,1), 0.f);
		nmSearchEngine->setOpen(pos + Vec2i(1,0), 0.f);
		nmSearchEngine->setOpen(pos + Vec2i(1,1), 0.f);
		++nt;
	}
	ResourceMapBuilderGoal goal(1.5f, pMap);
	ResourceMapBuilderCost cost(Field::LAND, 1, masterMap);
	nmSearchEngine->BUILD_RESOURCE_MAP(goal, cost, ZeroHeuristic());

	time = Chrono::getCurMillis() - time;
	ptr += sprintf(ptr, "Expanded %d nodes, took %dms\n", nmSearchEngine->getExpandedLastRun(), time);
	theLogger.add(buf);
}

/** Initialise annotated maps for each team, call after initial units visibility is applied */
void Cartographer::initTeamMaps() {
	map<int, ExplorationMap*>::iterator it = explorationMaps.begin();
	for ( ; it != explorationMaps.end(); ++it ) {
		AnnotatedMap *aMap = getAnnotatedMap(it->first);
		ExplorationMap *eMap = it->second;

	}
}

/** Maintains visibility on a per team basis. Adds or removes a unit's visibility
  * to (or from) its team exploration map, using a Dijkstra search.
  * @param unit the unit to remove or add visibility for
  * @param add true to add this units visibility to its team map, false to remove
  */
void Cartographer::maintainUnitVisibility(Unit *unit, bool add) {
	// set up goal function
	VisibilityMaintainerGoal goalFunc((float)unit->getSight(), explorationMaps[unit->getTeam()], add);
	// set up search engine
	GridNeighbours::setSearchSpace(SearchSpace::TILEMAP);
	nmSearchEngine->setNodeLimit(-1);
	nmSearchEngine->reset();
	nmSearchEngine->setOpen(Map::toTileCoords(unit->getCenteredPos()), 0.f);
	// zap
	nmSearchEngine->aStar<VisibilityMaintainerGoal,DistanceCost,ZeroHeuristic>
						 (goalFunc,DistanceCost(),ZeroHeuristic());
	// reset search space
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
}

}}}
