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
#include "abstract_map.h"
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
Cartographer::Cartographer() {
	theLogger.add("Cartographer", true);
	int w = theMap.getW(), h = theMap.getH();
	nodeMap = new NodeMap(w,h);
	nmSearchEngine = new SearchEngine<NodeMap,GridNeighbours>();
	nmSearchEngine->setStorage(nodeMap);
	nmSearchEngine->setInvalidKey(Vec2i(-1));
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
	masterMap = new AnnotatedMap();
	//abstractMap = new AbstractMap(this);
	clusterMap = new ClusterMap(masterMap,this);

	// team search and visibility maps
	set<int> teams;
	//theLogger.add( "Factions: " + intToStr(theWorld.getFactionCount()));
	for ( int i=0; i < theWorld.getFactionCount(); ++i ) {
		const Faction *f = theWorld.getFaction(i);
		int team = f->getTeam();
		//theLogger.add( "Faction " + intToStr(i) + " team: " + intToStr(f->getTeam()));
		if ( teams.find(team) == teams.end() ) {
			teams.insert(team);
			theLogger.add("Team : " + intToStr(team));
			// AnnotatedMap needs to be 'bound' to explored status
			explorationMaps[team] = new ExplorationMap();
			teamMaps[team] = new AnnotatedMap(explorationMaps[team]);
		}
	}

	// find and catalog all resources...
	for ( int x=0; x < theMap.getTileW() - 1; ++x ) {
		for ( int y=0; y < theMap.getTileH() - 1; ++y ) {
			const Resource * const r = theMap.getTile(x,y)->getResource();
			if ( r ) {
				resourceLocations[r->getType()].push_back(Vec2i(x,y));
			}
		}
	}

	// construct resource influence maps for each team
	for ( int i=0; i < theWorld.getTechTree()->getResourceTypeCount(); ++i ) {
		const ResourceType* rt = theWorld.getTechTree()->getResourceType( i );
		if ( rt->getClass() == ResourceClass::TECH || rt->getClass() == ResourceClass::TILESET ) {
			for ( int j=0; j < theWorld.getFactionCount(); ++j ) {
				int team = theWorld.getFaction(j)->getTeam();
				if ( teamResourceMaps[team].find(rt) == teamResourceMaps[team].end() ) {
					teamResourceMaps[team][rt] = new TypeMap<float>(Rect(0,0,theMap.getW(),theMap.getH()), -1.f);
				}
			}
		}
	}
}

/** Destruct */
Cartographer::~Cartographer() {
	delete masterMap;
	delete nmSearchEngine;
	delete nodeMap;

	map<int,AnnotatedMap*>::iterator aMapIt = teamMaps.begin();
	for ( ; aMapIt != teamMaps.end(); ++aMapIt ) {
		delete aMapIt->second;
	}
	teamMaps.clear();
	map<int,ExplorationMap*>::iterator eMapIt = explorationMaps.begin();
	for ( ; eMapIt != explorationMaps.end(); ++eMapIt ) {
		delete eMapIt->second;
	}
	explorationMaps.clear();

	map<int,map<const ResourceType*, TypeMap<float>*>>::iterator teamIt;
	map<const ResourceType*, TypeMap<float>*>::iterator iMapIt;
	teamIt = teamResourceMaps.begin();
	for ( ; teamIt != teamResourceMaps.end(); ++teamIt ) {
		iMapIt = teamIt->second.begin();
		for ( ; iMapIt != teamIt->second.end(); ++iMapIt ) {
			delete iMapIt->second;
		}
	}
	teamResourceMaps.clear();
}

/** WIP */
void Cartographer::updateResourceMaps() {
	set< int > seen;
	for ( int j=0; j < theWorld.getFactionCount(); ++j ) {
		int team = theWorld.getFaction( j )->getTeam();
		if ( seen.find( team ) != seen.end() ) {
			continue;
		}
		seen.insert( team );
		for ( int i=0; i < theWorld.getTechTree()->getResourceTypeCount(); ++i ) {
			const ResourceType* rt = theWorld.getTechTree()->getResourceType( i );
			if ( rt->getClass() == ResourceClass::TECH || rt->getClass() == ResourceClass::TILESET ) {
				initResourceMap( team, rt, teamResourceMaps[team][rt] );
			}
		}
	}
}

/** WIP */
void Cartographer::initResourceMap(int team, const ResourceType *rt, TypeMap<float> *iMap) {
	//DEBUG
	static char buf[1024];
	char *ptr = buf;
	ptr += sprintf(ptr, "Initialising %s influence map for team %d : ", rt->getName().c_str(), team);
	theLogger.add(buf);
	int64 time = Chrono::getCurMillis();
	vector<Vec2i> knownResources;
	vector<Vec2i>::iterator it = resourceLocations[rt].begin();
	for ( ; it != resourceLocations[rt].end(); ++it ) {
		if ( theMap.getTile(*it)->isExplored(team) ) {
			knownResources.push_back(*it);
		}
	}
	iMap->zeroMap();
	nmSearchEngine->reset();
	for ( it = knownResources.begin(); it != knownResources.end(); ++it ) {
		Vec2i pos = *it * Map::cellScale;
		nmSearchEngine->setOpen(pos, 0.f);
		nmSearchEngine->setOpen(pos + Vec2i(0,1), 0.f);
		nmSearchEngine->setOpen(pos + Vec2i(1,0), 0.f);
		nmSearchEngine->setOpen(pos + Vec2i(1,1), 0.f);
	}
	nmSearchEngine->buildDistanceMap(iMap, 20.f);

	time = Chrono::getCurMillis() - time;
	ptr += sprintf(ptr, "Used %d nodes, took %dms\n", nmSearchEngine->getExpandedLastRun(), time);
	theLogger.add(buf);
	//if ( team == theWorld.getThisTeamIndex() && rt->getName() == "gold" ) {
	//	iMap->log();
	//}
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
	///@todo take unit size into account?
	nmSearchEngine->setOpen(Map::toTileCoords(unit->getPos()), 0.f);
	// zap
	nmSearchEngine->aStar<VisibilityMaintainerGoal,DistanceCost,ZeroHeuristic>
						 (goalFunc,DistanceCost(),ZeroHeuristic());
	// reset search space
	GridNeighbours::setSearchSpace(SearchSpace::CELLMAP);
}

}}}
