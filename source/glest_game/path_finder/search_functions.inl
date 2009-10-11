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
	PosGoal(const Vec2i &target) : target(target) {}
	/** search target */
	Vec2i target; 
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
	RangeGoal(const Vec2i &target, float range) : target(target), range(range) {}
	/** search target */
	Vec2i target;
	/** range to get within */
	float range;
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
	InfluenceGoal(const InfluenceMap *iMap, float threshold) : iMap(iMap), threshold(threshold) {}
	/** InfluenceMap to use */
	const InfluenceMap *iMap;
	/** influence 'threshold' of goal */
	float threshold;
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
	FreeCellGoal(Field field) : field(field) {}
	/** field to find a free cell in */
	Field field;
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
	FreePosGoal(AnnotatedMap *aMap, Field field, int size) : aMap(aMap), field(field), size(size) {}
	/** Annotated Map to use */
	AnnotatedMap *aMap;
	/** field to find position in */
	Field field;
	/** size of unit to find position for */
	int size;
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
	NoGoal(){}
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
	DistanceBuilderGoal(float cutOff, InfluenceMap *iMap) : cutOff(cutOff), iMap(iMap) {}
	/** a 'cutoff' distance, search ends after this is reached. */
	float cutOff;
	/** inluence map to write distance data into. */
	InfluenceMap *iMap;
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
	InfluenceBuilderGoal(float cutOff, InfluenceMap *iMap) : cutOff(cutOff), iMap(iMap) {}
	/** WIP */
	float cutOff;
	/** WIP */
	InfluenceMap *iMap;
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
	UniformCost(float cost) : cost(cost) {}
	/** The uniform cost to return */
	float cost;
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
	DistanceCost(){}
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
	MoveCost(const Unit *unit, const AnnotatedMap *aMap) : unit(unit), aMap(aMap) {}
	/** unit wanting to move */
	const Unit *unit;
	/** map to search on */
	const AnnotatedMap *aMap;
	/** The cost function
	  * @param p1 position 1
	  * @param p2 position 2 ('adjacent' p1)
	  * @return cost of move, possibly infinite
	  */
	float operator () ( const Vec2i &p1, const Vec2i &p2 ) const {
		assert ( p1.dist(p2) < 1.5 && p1 != p2 );
		if ( ! aMap->canOccupy( p2, unit->getSize(), unit->getCurrField() ) ) {
			return numeric_limits<float>::infinity();
		}
		if ( p1.x != p2.x && p1.y != p2.y ) {
			Vec2i d1, d2;
			getDiags( p1, p2, unit->getSize(), d1, d2 );
			if ( !aMap->canOccupy( d1, 1, unit->getCurrField() ) 
			||	 !aMap->canOccupy( d2, 1, unit->getCurrField() ) ) {
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
	DiagonalDistance(const Vec2i &target) : target(target) {}
	/** search target */
	Vec2i target;	
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
	OverEstimate(const Vec2i &target) : target(target) {}
	/** search target */
	Vec2i target;
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
	ZeroHeuristic(){}
	/** The 'no heuristic' function.
	  * @param pos the position to ignore
	  * @return 0.f
	  */
	float operator () ( const Vec2i &pos ) const { return 0.0f; }
};
