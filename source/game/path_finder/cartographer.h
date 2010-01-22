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
#include "world.h"
#include "config.h"
#include "search_engine.h"
#include "sigslot.h"

namespace Glest { namespace Game { namespace Search {

class ClusterMap;
class RoutePlanner;

/** A map containing a visility counter and explored flag for every map tile. */
class ExplorationMap {
#	pragma pack(push, 2)
		struct ExplorationState {	/**< The exploration state of one tile for one team */			
			uint16 visCounter : 15;	/**< Visibility counter, the number of team's units that can see this tile */ 
			uint16 explored	  :  1;	/**< Explored flag */
		}; // max 32768 units per _team_
#	pragma pack(pop)
	ExplorationState *state; /**< The Data */
	Map *cellMap;
public:
	ExplorationMap(Map *cMap) : cellMap(cMap) { /**< Construct ExplorationMap, sets everything to zero */
		state = new ExplorationState[cellMap->getTileW() * cellMap->getTileH()];
		memset(state, 0, sizeof(ExplorationState) * cellMap->getTileW() * cellMap->getTileH());
	}
	/** @param pos cell coordinates @return number of units that can see this tile */
	int  getVisCounter(const Vec2i &pos) const	{ return state[pos.y * cellMap->getTileH() + pos.x].visCounter; }
	/** @param pos tile coordinates to increase visibilty on */
	void incVisCounter(const Vec2i &pos) const	{ state[pos.y * cellMap->getTileH() + pos.x].visCounter ++;		}
	/** @param pos tile coordinates to decrease visibilty on */
	void decVisCounter(const Vec2i &pos) const	{ state[pos.y * cellMap->getTileH() + pos.x].visCounter --;		}
	/** @param pos tile coordinates @return true if explored. */
	bool isExplored(const Vec2i &pos)	 const	{ return state[pos.y * cellMap->getTileH() + pos.x].explored;	}
	/** @param pos coordinates of tile to set as explored */
	void setExplored(const Vec2i &pos)	 const	{ state[pos.y * cellMap->getTileH() + pos.x].explored = 1;		}

};

//
// Cartographer: 'Map' Manager
//
class Cartographer : public sigslot::has_slots<> {
	/** Master annotated map, always correct */
	AnnotatedMap *masterMap;
	/** Team annotateded maps, 'foggy' */
	//map< int, AnnotatedMap* > teamMaps;
	/**  */
	//AbstractMap *abstractMap;
	ClusterMap *clusterMap;
	/** The locations of each and every resource on the map */
	map< const ResourceType*, vector< Vec2i > > resourceLocations;

	typedef vector< pair<Vec2i, Vec2i> > AreaList;
	map<const ResourceType*, AreaList> resDirtyAreas;
	/* /* Inlfuence maps, for each team, describing distance to resources */
	//map< int, map< const ResourceType*, TypeMap<float>* > > teamResourceMaps;

	/** Goal Maps for each tech & tileset resource */
	map< const ResourceType*, PatchMap<1>* > resourceMaps;
	
	/** Exploration maps for each team */
	map< int, ExplorationMap* > explorationMaps;

	NodeMap *nodeMap;
	SearchEngine<NodeMap,GridNeighbours> *nmSearchEngine;

	World *world;
	Map *cellMap;
	RoutePlanner *routePlanner;

	void initResourceMap(const ResourceType *rt, PatchMap<1> *pMap);
	void fixupResourceMap(const ResourceType *rt, const Vec2i &tl, const Vec2i &br);
	void onResourceDepleted(Vec2i pos);

	void maintainUnitVisibility(Unit *unit, bool add);

public:
	Cartographer(World *world);
	virtual ~Cartographer();

	SearchEngine<NodeMap,GridNeighbours>* getSearchEngine() { return nmSearchEngine; }
	RoutePlanner* getRoutePlanner() { return routePlanner; }

	/** @return the number of units of team that can see a tile 
	  * @param team team index 
	  * @param pos the co-ordinates of the <b>tile</b> of interest. */
	int getTeamVisibility(int team, const Vec2i &pos) { return explorationMaps[team]->getVisCounter(pos); }
	/** Adds a unit's visibility to its team's exploration map */
	void applyUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, true); }
	/** Removes a unit's visibility from its team's exploration map */
	void removeUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, false); }

	void initTeamMaps();

	/** Update the annotated maps when an obstacle has been added or removed from the map.
	  * Unconditionally updates the master map, updates team maps if the team can see the cells,
	  * or mark as 'dirty' if they cannot currently see the change. @todo implement team maps
	  * @param pos position (north-west most cell) of obstacle
	  * @param size size of obstacle	*/
	void updateMapMetrics(const Vec2i &pos, const int size) { 
		masterMap->updateMapMetrics(pos, size);
		// who can see it ? update their maps too.
		// set cells as dirty for those that can't see it
	}

	void tick();

	PatchMap<1>* getResourceMap(const ResourceType* rt) {
		return resourceMaps[rt];
	}

	ClusterMap* getClusterMap() const { return clusterMap; }

	AnnotatedMap* getMasterMap()				const	{ return masterMap;							  }
	AnnotatedMap* getAnnotatedMap(int team )			{ return masterMap;/*teamMaps[team];*/					  }
	AnnotatedMap* getAnnotatedMap(const Faction *faction) 	{ return getAnnotatedMap(faction->getTeam()); }
	AnnotatedMap* getAnnotatedMap(const Unit *unit)			{ return getAnnotatedMap(unit->getTeam());	  }
};

//class Surveyor {
//};

}}}

#endif
