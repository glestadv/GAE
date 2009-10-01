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
// search_functions.inl

// Goal function for 'normal' search
class PosGoal {
public:
	static Vec2i target; // search target
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return pos == target; 
	}
};

// Goal function for 'get within x of' searches
class RangeGoal {
public:
	static Vec2i target; // search target
	static float range; // range to get within
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return pos.dist( target ) <= range; 
	}
};

// Goal function using influence map
class InfluenceGoal {
public:
	const static InfluenceMap *iMap;
	static float threshold;
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return iMap->getInfluence( pos ) > threshold; 
	}
};

// Goal function for free cell search
class FreeCellGoal {
public:
	static Field field; // field to find a free cell in
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return theMap.isFreeCell( pos, field ); 
	}
};

// The 'No Goal' function
class NoGoal {
public:
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return false; 
	}
};

class DistanceBuilderGoal {
public:
	static float cutOff;
	static InfluenceMap *iMap;
	bool operator () ( const Vec2i &pos, const float costSoFar ) const {
		if ( costSoFar > cutOff ) {
			return true;
		}
		if ( costSoFar < iMap->getInfluence( pos ) ) {
			iMap->setInfluence( pos, cutOff - costSoFar );
		}
		return false;
	}
};

class InfluenceBuilderGoal {
public:
	static float cutOff;
	static InfluenceMap *iMap;
	bool operator () ( const Vec2i &pos, const float costSoFar ) const {
		if ( costSoFar > cutOff ) {
			return true;
		}
		if ( cutOff - costSoFar > iMap->getInfluence( pos ) ) {
			iMap->setInfluence( pos, cutOff - costSoFar );
		}
		return false;
	}
};

// A uniform cost function
//template< float cost = 0.f >
class UniformCost {
public:
	float operator () ( const Vec2i &p1, const Vec2i &p2 ) const { return 0.f; }
};

// distance cost, no obstacle checks
class DistanceCost {
public:
	float operator () ( const Vec2i &p1, const Vec2i &p2 ) const {
		assert ( p1.dist( p2 ) < 1.5 );
		if ( p1.x != p2.x && p1.y != p2.y ) {
			return SQRT2;
		}
		return 1.0f;
	}
};

// The movement cost function
class MoveCost {
public:
	const static Unit *unit; // unit wanting to move
	const static AnnotatedMap *map;
	float operator () ( const Vec2i &p1, const Vec2i &p2 ) const {
		assert ( p1.dist(p2) < 1.5 && p1 != p2 );
		if ( ! map->canOccupy( p2, unit->getSize(), unit->getCurrField() ) ) {
			return numeric_limits<float>::infinity();
		}
		if ( p1.x != p2.x && p1.y != p2.y ) {
			Vec2i d1, d2;
			getDiags( p1, p2, unit->getSize(), d1, d2 );
			if ( !map->canOccupy( d1, 1, unit->getCurrField() ) 
			||	 !map->canOccupy( d2, 1, unit->getCurrField() ) ) {
				return numeric_limits<float>::infinity();
			}
			return SQRT2;
		}
		return 1.0f;
		// todo... height
	}
};

// Diaginal Distance Heuristic
class DiagonalDistance {
public:
	static Vec2i target; // search target
	float operator () ( const Vec2i &pos ) const {
		float dx = (float)abs( pos.x - target.x ), 
			  dy = (float)abs( pos.y - target.y );
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return 1.4 * diag + straight;
	}
};

// Diagonal Distance Overestimating Heuristic
class OverEstimate {
public:
	static Vec2i target; // search target
	float operator () ( const Vec2i &pos ) const {
		float dx = (float)abs( pos.x - target.x ), 
			  dy = (float)abs( pos.y - target.y );
		float diag = dx < dy ? dx : dy;
		float estimate = 1.4 * diag + ( dx + dy - 2 * diag );
		estimate *= 1.25;
		return estimate;
	}
};

// The Zero Heuristic, for doing Dijkstra searches
class ZeroHeuristic {
public:
	float operator () ( const Vec2i &pos ) const { return 0.0f; }
};
