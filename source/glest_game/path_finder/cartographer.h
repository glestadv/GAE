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

namespace Glest{ namespace Game{

namespace Search {

class ExplorationMap {
#	pragma pack(push, 2)
		struct ExplorationState {
			uint16 visCounter : 14; // max 16384 units per _team_
			uint16 dirty	  :  1; // ?? no, is at 'cell' scale, use AnnotatedMap cell _pad_bit
			uint16 explored	  :  1;
		};
#	pragma pack(pop)

	ExplorationState *state;
public:
	ExplorationMap() {
		state = new ExplorationState[theMap.getTileW()*theMap.getTileH()];
		memset( state, 0, sizeof(ExplorationState)*theMap.getTileW()*theMap.getTileH() );
	}
};

template<int bits=2> struct PatchMap {
#	pragma pack(push, 1)
		struct PatchSection {
			static const int sectionSize = 8 / bits;
			uint8 get( int ndx ) {
				assert( ndx < sectionSize );
				const int shift = 8 - (ndx + 1) * bits;
				return( (byte >> shift) & 3 );
			}
		private:
			uint8 byte : bits;
		};
#	pragma pack(pop)

	PatchMap( int x, int y, int w, int h ) 
			: offsetX(x)
			, offsetY(y)
			, width(w)
			, height(h) {
		assert( x >= 0 && y >= 0 && x + w <= theMap.getW() && y + h <= theMap.getH() );
		// height and width must be (positive, non zero) multiples of 8
		assert( w && w & 3 == 0 && h && h & 3 == 0 );
		
	}
	uint8 getValue( const int x, const int y ) const {

	}

private:
	int sectionStride;
	int offsetX, offsetY, width, height;
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

	// 
	// only redo if changed TILE...
	// 

	// call after Map::clearUnitCells()
	void clearUnitVisibility( Unit *unit );
	// call after Map::putUnitCells()
	void applyUnitVisibility( Unit *unit );

	void resourceDepleted( Resource *r );

	// update the annotated map at pos 
	void updateMapMetrics( const Vec2i &pos, const int size, bool adding, Field field ) { 
		PROFILE_START("AnnotatedMap::updateMapMetrics()");
		masterMap->updateMapMetrics( pos, size );
		PROFILE_STOP("AnnotatedMap::updateMapMetrics()");

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

	AnnotatedMap* getAnnotatedMap( int team )		  { return teamMaps[team]; }
	AnnotatedMap* getAnnotatedMap( Faction *faction ) { return getAnnotatedMap( faction->getTeam() ); }
	AnnotatedMap* getAnnotatedMap( Unit *unit )		  { return getAnnotatedMap( unit->getTeam() );	  }
};

class Surveyor {
};

}}}

#endif
