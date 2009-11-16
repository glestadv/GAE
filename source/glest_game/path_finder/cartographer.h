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

namespace Glest { namespace Game { namespace Search {

class AbstractMap;
class ClusterMap;

/** A map containing a visility counter and explored flag for every map tile. */
class ExplorationMap {
#	pragma pack(push, 2)
		struct ExplorationState {	/**< The exploration state of one tile for one team */			
			uint16 visCounter : 15;	/**< Visibility counter, the number of team's units that can see this tile */ 
			uint16 explored	  :  1;	/**< Explored flag */
		}; // max 32768 units per _team_
#	pragma pack(pop)
	ExplorationState *state; /**< The Data */
public:
	ExplorationMap() { /**< Construct ExplorationMap, sets everything to zero */
		state = new ExplorationState[theMap.getTileW() * theMap.getTileH()];
		memset( state, 0, sizeof(ExplorationState) * theMap.getTileW() * theMap.getTileH() );
	}
	/** @param pos cell coordinates @return number of units that can see this tile */
	int  getVisCounter(const Vec2i &pos) const	{ return state[pos.y * theMap.getTileH() + pos.x].visCounter; }
	/** @param pos tile coordinates to increase visibilty on */
	void incVisCounter(const Vec2i &pos) const	{ state[pos.y * theMap.getTileH() + pos.x].visCounter ++;		}
	/** @param pos tile coordinates to decrease visibilty on */
	void decVisCounter(const Vec2i &pos) const	{ state[pos.y * theMap.getTileH() + pos.x].visCounter --;		}
	/** @param pos tile coordinates @return true if explored. */
	bool isExplored(const Vec2i &pos)	 const	{ return state[pos.y * theMap.getTileH() + pos.x].explored;	}
	/** @param pos coordinates of tile to set as explored */
	void setExplored(const Vec2i &pos)	 const	{ state[pos.y * theMap.getTileH() + pos.x].explored = 1;		}

};

//
// Cartographer: 'Map' Manager
//
class Cartographer {
	/** Master annotated map, always correct */
	AnnotatedMap *masterMap;
	/** Team annotateded maps, 'foggy' */
	map< int, AnnotatedMap* > teamMaps;
	/**  */
	AbstractMap *abstractMap;
	ClusterMap *clusterMap;
	/** The locations of each and every resource on the map */
	map< const ResourceType*, vector< Vec2i > > resourceLocations;
	/** Inlfuence maps, for each team, describing distance to resources */
	map< int, map< const ResourceType*, TypeMap<float>* > > teamResourceMaps;
	/** Exploration maps for each team */
	map< int, ExplorationMap* > explorationMaps;

	NodeMap *nodeMap;
	SearchEngine<NodeMap,GridNeighbours> *nmSearchEngine;

	void initResourceMap( int team, const ResourceType *rt, TypeMap<float> *iMap );

	/** Custom Goal function for maintaining the exploration maps */
	class VisibilityMaintainerGoal {
	private:
		float range;			/**< range of entity sight */
		ExplorationMap *eMap;	/**< exploration map to adjust */
		bool inc;				/**< true to increment, false to decrement */
	public:
		/** Construct goal function object
		  * @param range the range of visibility
		  * @param eMap the ExplorationMap to adjust
		  * @param inc true to apply visibility, false to remove
		  */
		VisibilityMaintainerGoal(float range, ExplorationMap *eMap, bool inc)
			: range(range), eMap(eMap), inc(inc) {}

		/** The goal function 
		  * @param pos position to test
		  * @param costSoFar the cost of the shortest path to pos
		  * @return true when range is exceeded.
		  */
		bool operator()(const Vec2i &pos, const float costSoFar) const { 
			if ( costSoFar > range ) {
				return true;
			}
			if ( inc ) {
				eMap->incVisCounter(pos);
			} else {
				eMap->decVisCounter(pos);
			}
			return false; 
		}
	};
	void maintainUnitVisibility(Unit *unit, bool add);

public:
	Cartographer();
	~Cartographer();
	void updateResourceMaps();

	SearchEngine<NodeMap,GridNeighbours>* getSearchEngine() { return nmSearchEngine; }

	/** @return the number of units of team that can see a tile 
	  * @param team team index 
	  * @param pos the co-ordinates of the <b>tile</b> of interest. */
	int getTeamVisibility(int team, const Vec2i &pos) { return explorationMaps[team]->getVisCounter(pos); }
	/** Adds a unit's visibility to its team's exploration map */
	void applyUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, true); }
	/** Removes a unit's visibility from its team's exploration map */
	void removeUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, false); }

	void resourceDepleted(Resource *r);

	void initTeamMaps();

	/** Update the annotated maps when an obstacle has been added or removed from the map.
	  * Unconditionally updates the master map, updates team maps if the team can see the cells,
	  * or mark as 'dirty' if they cannot currently see the change. @todo implement team maps
	  * @param pos position (north-west most cell) of obstacle
	  * @param size size of obstacle
	  */
	void updateMapMetrics(const Vec2i &pos, const int size) { 
		masterMap->updateMapMetrics(pos, size);
		// who can see it ? update their maps too.
		// set cells as dirty for those that can't see it

	}

	TypeMap<float>* getResourceMap(int team, const ResourceType* rt) {
		return teamResourceMaps[team][rt];
	}
	TypeMap<float>* getResourceMap(Faction *faction, const ResourceType* rt) {
		return teamResourceMaps[faction->getTeam()][rt];
	}
	TypeMap<float>* getResourceMap(Unit *unit, const ResourceType* rt) {
		return teamResourceMaps[unit->getTeam()][rt];
	}

	AbstractMap* getAbstractMap() const	{ return abstractMap; }
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
