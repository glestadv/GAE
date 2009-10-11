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

/** Goal function for 'normal' search */
class PosGoal {
public:
	static Vec2i target; /** search target */
	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is target, else false
	  */
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return pos == target; 
	}
};

/** Goal function for 'get within x of' searches */
class RangeGoal {
public:
	static Vec2i target; /** search target */
	static float range;  /** range to get within */
	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is within range of target, else false
	  */
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return pos.dist( target ) <= range; 
	}
};

/** Goal function using influence map */
class InfluenceGoal {
public:
	const static InfluenceMap *iMap;	/** InfluenceMap to use */
	static float threshold;				/** influence 'threshold' of goal */
	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if influence at pos on iMap is greater than threashold, else false
	  */
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return iMap->getInfluence( pos ) > threshold; 
	}
};

/** Goal function for free cell search */
class FreeCellGoal {
public:
	static Field field; /** field to find a free cell in */
	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is free, else false
	  */
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return theMap.isFreeCell( pos, field ); 
	}
};

/** Goal function to find a free position that a unit of 'size' can occupy. */
class FreePosGoal {
public:
	static AnnotatedMap *aMap;	/** Annotated Map to use */
	static Field field;			/** field to find position in */
	static int size;			/** size of unit to find position for */
	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if a unit of size can occupy pos in field (according to aMap), else false
	  */
	bool operator()(const Vec2i &pos, const float costSoFar) const {
		return aMap->canOccupy(pos, size, field);
	}
};

/** The 'No Goal' function. Just returns false. Use with care! Use Cost function to control termination
  * by exhausting the open list. */
class NoGoal {
public:
	/** The goal function 
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return false
	  */
	bool operator () ( const Vec2i &pos, const float costSoFar ) const { 
		return false; 
	}
};

/** Helper goal, used to build distance maps. */
class DistanceBuilderGoal {
public:
	static float cutOff;		/** a 'cutoff' distance, search ends after this is reached. */
	static InfluenceMap *iMap;	/** inluence map to write distance data into. */
	/** The goal function, writes ( cutOff - costSoFar ) into the influence map.
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if costSoFar exceeeds cutOff, else false.
	  */
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

/** Helper goal, used to build influence maps. */
class InfluenceBuilderGoal {
public:
	static float cutOff;		/** WIP */
	static InfluenceMap *iMap;	/** WIP */
	/** The goal function, WIP
	  * @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return 
	  */
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

/** A uniform cost function */
class UniformCost {
public:
	static float cost; /** The uniform cost to return */
	/** The cost function
	  * @param p1 position 1
	  * @param p2 position 2 ('adjacent' p1)
	  * @return cost
	  */
	float operator () ( const Vec2i &p1, const Vec2i &p2 ) const { return 0.f; }
};

/** distance cost, no obstacle checks */
class DistanceCost {
public:
	/** The cost function
	  * @param p1 position 1
	  * @param p2 position 2 ('adjacent' p1)
	  * @return 1.0 if p1 and p2 are 'in line', else SQRT2
	  */
	float operator () ( const Vec2i &p1, const Vec2i &p2 ) const {
		assert ( p1.dist( p2 ) < 1.5 );
		if ( p1.x != p2.x && p1.y != p2.y ) {
			return SQRT2;
		}
		return 1.0f;
	}
};

/** The movement cost function */
class MoveCost {
public:
	const static Unit *unit;		/** unit wanting to move */
	const static AnnotatedMap *map; /** map to search on */
	/** The cost function
	  * @param p1 position 1
	  * @param p2 position 2 ('adjacent' p1)
	  * @return cost of move, possibly infinite
	  */
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

/** Diaginal Distance Heuristic */
class DiagonalDistance {
public:
	static Vec2i target;	/** search target */
	/** The heuristic function.
	  * @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target
	  */
	float operator () ( const Vec2i &pos ) const {
		float dx = (float)abs( pos.x - target.x ), 
			  dy = (float)abs( pos.y - target.y );
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return 1.4 * diag + straight;
	}
};

/** Diagonal Distance Overestimating Heuristic */
class OverEstimate {
public:
	static Vec2i target; /** search target */
	/** The heuristic function.
	  * @param pos the position to calculate the heuristic for
	  * @return an (over) estimate of the cost to target
	  */
	float operator () ( const Vec2i &pos ) const {
		float dx = (float)abs( pos.x - target.x ), 
			  dy = (float)abs( pos.y - target.y );
		float diag = dx < dy ? dx : dy;
		float estimate = 1.4 * diag + ( dx + dy - 2 * diag );
		estimate *= 1.25;
		return estimate;
	}
};

/** The Zero Heuristic, for doing Dijkstra searches */
class ZeroHeuristic {
public:
	/** The 'no heuristic' function.
	  * @param pos the position to ignore
	  * @return 0.f
	  */
	float operator () ( const Vec2i &pos ) const { return 0.0f; }
};
