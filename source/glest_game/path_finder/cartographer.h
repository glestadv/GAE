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


#ifndef _GLEST_GAME_CARTOGRAPHER_H_
#define _GLEST_GAME_CARTOGRAPHER_H_

#include "game_constants.h"
#include "influence_map.h"
#include "annotated_map.h"
#include "config.h"
#include "search_engine.h"

namespace Glest{ namespace Game{

namespace Search {

#pragma pack(push, 2)
struct ExplorationState {
	uint16 visCounter : 15; // max 32767 units per _team_
	uint16 dirty	  :  1;
};
#pragma pack(pop)

class ExplorationMap {
	ExplorationState *state;
public:
	ExplorationMap() {
		state = new ExplorationState[theMap.getW()*theMap.getH()];
		memset( state, 0, sizeof(ExplorationState)*theMap.getW()*theMap.getH() );
	}
};

//
// Cartographer: 'Map' Manager
//
class Cartographer {
	AnnotatedMap *masterMap;
	map< int, AnnotatedMap* > teamMaps;
	map< const ResourceType*, vector< Vec2i > > resourceLocations;
	map< int, map< const ResourceType*, InfluenceMap* > > teamResourceMaps;
	map< int, ExplorationMap* > explorationMaps;

	void initResourceMap( int team, const ResourceType *rt, InfluenceMap *iMap );

public:
	Cartographer();
	void updateResourceMaps();

	InfluenceMap* getResourceMap( int team, const ResourceType* rt ) {
		return teamResourceMaps[team][rt];
	}
	InfluenceMap* getResourceMap( Faction *faction, const ResourceType* rt ) {
		return teamResourceMaps[faction->getTeam()][rt];
	}
	InfluenceMap* getResourceMap( Unit *unit, const ResourceType* rt ) {
		return teamResourceMaps[unit->getTeam()][rt];
	}

	AnnotatedMap* getAnnotatedMap( int team ) { return teamMaps[team]; }
	AnnotatedMap* getAnnotatedMap( Faction *faction ) { return getAnnotatedMap( faction->getTeam() ); }
	AnnotatedMap* getAnnotatedMap( Unit *unit ) { return getAnnotatedMap( unit->getTeam() ); }
};

class Surveyor {
};

}}}

#endif
