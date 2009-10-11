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

namespace Glest { namespace Game { namespace Search {

/** A map containing a visility counter and explored flag for every cell. */
class ExplorationMap {
#	pragma pack(push, 2)
		/** The exploration state of one cell for one team */
		struct ExplorationState {
			/** Visibility counter, the number of team's units that can see this cell */
			uint16 visCounter : 15; // max 32768 units per _team_
			uint16 explored	  :  1; /** Explored flag */
		};
#	pragma pack(pop)

	ExplorationState *state; /** The Data */
public:
	ExplorationMap() {
		state = new ExplorationState[theMap.getTileW() * theMap.getTileH()];
		memset( state, 0, sizeof(ExplorationState) * theMap.getTileW() * theMap.getTileH() );
	}

	int  getVisCounter(const Vec2i &pos) const	{ return state[pos.y * theMap.getTileH() + pos.x].visCounter; }
	void incVisCounter(const Vec2i &pos) const	{ state[pos.y * theMap.getTileH() + pos.x].visCounter ++;		}
	void decVisCounter(const Vec2i &pos) const	{ state[pos.y * theMap.getTileH() + pos.x].visCounter --;		}
	bool isExplored(const Vec2i &pos)	 const	{ return state[pos.y * theMap.getTileH() + pos.x].explored;	}
	void setExplored(const Vec2i &pos)	 const	{ state[pos.y * theMap.getTileH() + pos.x].explored = 1;		}

};

template<int bits=2> struct PatchMap {
#	pragma pack(push, 1)
		struct PatchSection {
			static const int sectionSize = 8 / bits;
			uint8 get( int ndx ) {
				assert( ndx < sectionSize );
				const int shift = 8 - (ndx + 1) * bits;
				return( (byte >> shift) & ( (1 << bits) - 1) );
			}
		private:
			uint8 byte : bits;
		};
#	pragma pack(pop)

	PatchMap( int x, int y, int w, int h ) : offsetX(x) , offsetY(y) , width(w) , height(h) {
		assert( x >= 0 && y >= 0 && x + w <= theMap.getW() && y + h <= theMap.getH() );
		// height and width must be (positive, non zero) multiples of 8
		assert( w > 0 && w & 3 == 0 && h > 0 && h & 3 == 0 );
		const int stride = w / PatchSection::sectionSize; // patches per row
		data = new PatchSection[stride*h];
	}
	uint8 getValue( const int mx, const int my ) const {
		const int x = mx - offestX;
		const int y = my - offsetY;
		if ( x < 0 || y < 0 || x >= width || y >= height ) {
			return 0;
		}
		const int stride = width / PatchSection::sectionSize;
		const int sx = x / PatchSection::sectionSize;
		const int ox = x % PatchSection::sectionSize;
		return data[y*stride+sx].get(ox);
	}

private:
	int offsetX, offsetY, width, height;
	PatchSection *data;
};



//
// Cartographer: 'Map' Manager
//
class Cartographer {
	/** Master annotated map, always correct */
	AnnotatedMap *masterMap;
	/** Team annotateded maps, 'foggy' */
	map< int, AnnotatedMap* > teamMaps;
	/** The locations of each and every resource on the map */
	map< const ResourceType*, vector< Vec2i > > resourceLocations;
	/** Inlfuence maps, for each team, describing distance to resources */
	map< int, map< const ResourceType*, InfluenceMap* > > teamResourceMaps;
	/** Exploration maps for each team */
	map< int, ExplorationMap* > explorationMaps;

	void initResourceMap( int team, const ResourceType *rt, InfluenceMap *iMap );

	/** Custom Goal function for maintaining the exploration maps */
	class VisibilityMaintainerGoal {
	public:
		static float range;				/** range of sight */
		static ExplorationMap *eMap;	/** exploration map to adjust */
		static bool inc;				/** true to increment, false to decrement */
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

	/** @return the number of units of team that can see pos */
	int getTeamVisibility(int team, const Vec2i &pos) { return explorationMaps[team]->getVisCounter(pos); }
	/** Adds a unit's visibility to its team's exploration map */
	void applyUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, true); }
	/** Removes a unit's visibility from its team's exploration map */
	void removeUnitVisibility(Unit *unit)	{ maintainUnitVisibility(unit, false); }

	void resourceDepleted( Resource *r );

	/** Update the annotated maps when an obstacle has been added or removed from the map.
	  * Unconditionally updates the master map, updates team maps if the team can see the cells,
	  * or mark as 'dirty' if they cannot currently see the change.
	  * @param pos position (north-west most cell) of obstacle
	  * @param size size of obstacle
	  */
	void updateMapMetrics( const Vec2i &pos, const int size ) { 
		masterMap->updateMapMetrics( pos, size );
		// who can see it ? update their maps too.
		// set cells as dirty for those that can't see it
	}

	InfluenceMap* getResourceMap( int team, const ResourceType* rt ) {
		return teamResourceMaps[team][rt];
	}
	InfluenceMap* getResourceMap( Faction *faction, const ResourceType* rt ) {
		return teamResourceMaps[faction->getTeam()][rt];
	}
	InfluenceMap* getResourceMap( Unit *unit, const ResourceType* rt ) {
		return teamResourceMaps[unit->getTeam()][rt];
	}

	AnnotatedMap* getMasterMap()					  { return masterMap; }
	AnnotatedMap* getAnnotatedMap( int team )		  { return masterMap;/*return teamMaps[team];*/ }
	AnnotatedMap* getAnnotatedMap( Faction *faction ) { return getAnnotatedMap( faction->getTeam() ); }
	AnnotatedMap* getAnnotatedMap( Unit *unit )		  { return getAnnotatedMap( unit->getTeam() );	  }
};

class Surveyor {
};

}}}

#endif
