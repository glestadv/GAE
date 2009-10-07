// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//				  2009 James McCulloch <silnarm@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_MAP_H_
#define _GAME_MAP_H_

#include <cassert>
#include <map>
#include <set>

#include "vec.h"
#include "math_util.h"
#include "command_type.h"
#include "logger.h"
#include "object.h"
#include "game_constants.h"
#include "selection.h"
#include "exceptions.h"
#include "pos_iterator.h"
#include "patterns.h"

namespace Shared { namespace Platform {
	class NetworkDataBuffer;
}}

using Shared::Graphics::Vec4f;
using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Texture2D;
using Shared::Platform::NetworkDataBuffer;
using Game::Util::PosCircularIteratorFactory;

namespace Game {

class Tileset;
class Unit;
class Resource;
class TechTree;
class World;
class UnitContainer;
class Earthquake;

// =====================================================
// 	class Cell
// =====================================================

/**
 * A map cell that holds info about units present on it.
 */
class Cell : Uncopyable {
private:
	Unit *units[ZoneCount];		/**< Units residing in each zone of this cell. */
	float height;				/**< Altitude (of ground) of this cell. */
	SurfaceType surfaceType;

public:
	/**
	 * Default ctor.  It's usually better to outline stuff like this, but when these are created,
	 * they are created in large quantities, so it's probably much better to have this ctor inlined.
	 */
	Cell() : height(0), surfaceType(SurfaceTypeLand) {
		memset(units, 0, sizeof(units));
	}

	// get
	Unit *getUnit(Zone zone) const	{return units[zone];}
	Unit *getUnit(Field field) const{return getUnit(field == FieldAir ? ZoneAir : ZoneSurface);}
	float getHeight() const			{return height;}
	SurfaceType getType() const		{return surfaceType;}

	bool isSubmerged() const		{return surfaceType != SurfaceTypeLand;}
	bool isDeepSubmerged() const	{return surfaceType == SurfaceTypeDeepWater;}

	// set
	void setUnit(Zone field, Unit *unit)	{units[field] = unit;}
	void setHeight(float h)					{height = h;}
	void setType(SurfaceType type)			{surfaceType = type;}

	//misc
	/** @returns true if the zone in the cell is not occupied. */
	bool isFree(Zone zone) const {
		const Unit *resident = getUnit(zone);
		return !resident || resident->isPutrefacting();
	}
};

// =====================================================
// 	class Tile
//
//	A heightmap cell, each Tile is composed by more than one Cell
// =====================================================

class Tile {
private:
	//geometry
	Vec3f vertex;
	Vec3f normal;
	Vec3f color;
	Vec3f originalVertex;
	float waterLevel;

	//tex coords
	Vec2f fowTexCoord;		//tex coords for TEXTURE1 when multitexturing and fogOfWar
	Vec2f surfTexCoord;		//tex coords for TEXTURE0

	//surface
	int tileType;
	const Texture2D *tileTexture;

	//object & resource
	Object *object;

	//visibility
	bool visible[GameConstants::maxFactions];
	bool explored[GameConstants::maxFactions];

	//cache
	bool nearSubmerged;

public:
	Tile();
	~Tile();

	//get
	const Vec3f &getVertex() const				{return vertex;}
	float getHeight() const						{return vertex.y;}
	const Vec3f &getNormal() const				{return normal;}
	const Vec3f &getColor() const				{return color;}
	const Vec3f &getOriginalVertex() const		{return originalVertex;}
	float getWaterLevel() const					{return waterLevel;}
	int getTileType() const						{return tileType;}
	const Texture2D *getTileTexture() const		{return tileTexture;}
	Object *getObject() const					{return object;}
	Resource *getResource() const				{return !object ? NULL : object->getResource();}
	const Vec2f &getFowTexCoord() const			{return fowTexCoord;}
	const Vec2f &getSurfTexCoord() const		{return surfTexCoord;}
	bool getNearSubmerged() const				{return nearSubmerged;}
	bool getSubmerged() const					{return vertex.y < waterLevel;}
	bool getDeepSubmerged(float heightFactor) const	{return vertex.y < waterLevel - (1.5f / heightFactor);}

	bool isVisible(int teamIndex) const			{return visible[teamIndex];}
	bool isExplored(int teamIndex) const		{return explored[teamIndex];}

	//set
	void setVertex(const Vec3f &v)				{originalVertex = vertex = v;}
	void setHeight(float v)						{originalVertex.y = vertex.y = v;}
	void setNormal(const Vec3f &v)				{normal = v;}
	void setColor(const Vec3f &v)				{color = v;}
	void setWaterLevel(float v)					{waterLevel = v;}
	void setTileType(int v)						{tileType = v;}
	void setTileTexture(const Texture2D *v)		{tileTexture = v;}
	void setObject(Object *v)					{object = v;}
	void setFowTexCoord(const Vec2f &v)			{fowTexCoord = v;}
	void setTileTexCoord(const Vec2f &v)		{surfTexCoord = v;}
	void setExplored(int teamIndex, bool v)		{explored[teamIndex] = v;}
	void setVisible(int teamIndex, bool v)		{visible[teamIndex] = v;}
	void setNearSubmerged(bool v)				{nearSubmerged = v;}

	//misc
	void deleteResource();
	bool isFree() const								{return object == NULL || object->getWalkable();}
	void resetVertex()								{vertex = originalVertex;}
	void alterVertex(const Vec3f &offset)			{vertex += offset;}
	void updateObjectVertex() {
		if (object) {
			object->setPos(vertex); // should be centered ???
		}
	}

	// I know it looks stupid using NetworkDataBuffer to save these, but then I
	// get my multi-byte values in platform portable format, so that saved game
	// files will work across platforms (especially when resuming an interrupted
	// network game).
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
};

// =====================================================
// 	class Map
//
///	Represents the game map (and loads it from a gbm file)
// =====================================================

class Map : Uncopyable {
public:
	static const int cellScale = 2;	//number of cells per tile
	static const int mapScale = 2;	//horizontal scale of surface
	typedef vector<Earthquake*> Earthquakes;

private:
	string title;
	float waterLevel;
	float heightFactor;
	int w;
	int h;
	int tileW;
	int tileH;
	int maxFactions;
	Cell *cells;
	Tile *tiles;
	Vec2i *startLocations;
	float *surfaceHeights;

	Earthquakes earthquakes;

public:
	Map();
	~Map();

	void init();
	void load(const string &path, TechTree *techTree, Tileset *tileset);

	//get
	Cell *getCell(int x, int y) const		{assert(isInside(x, y)); return &cells[y * w + x];}
	Cell *getCell(const Vec2i &pos) const	{return getCell(pos.x, pos.y);}
	Tile *getTile(int sx, int sy) const		{assert(isInsideTile(sx, sy)); return &tiles[sy * tileW + sx];}
	Tile *getTile(const Vec2i &sPos) const	{return getTile(sPos.x, sPos.y);}

	int getW() const									{return w;}
	int getH() const									{return h;}
	int getTileW() const								{return tileW;}
	int getTileH() const								{return tileH;}
	int getMaxFactions() const							{return maxFactions;}
	float getHeightFactor() const						{return heightFactor;}
	float getWaterLevel() const							{return waterLevel;}
	Vec2i getStartLocation(int loactionIndex) const		{return startLocations[loactionIndex];}

	// these should be in their respective cell classes...
	//bool getSubmerged(const Tile *sc) const				{return sc->getSubmerged();}
	//bool getSubmerged(const Cell *c) const				{return c->getHeight()<waterLevel;}
	bool getDeepSubmerged(const Tile *sc) const			{return sc->getDeepSubmerged(heightFactor);}
	//bool getDeepSubmerged(const Cell *c) const			{return c->getHeight()<waterLevel-(1.5f/heightFactor);}
	//float getSurfaceHeight(const Vec2i &pos) const;

	const Earthquakes &getEarthquakes() const			{return earthquakes;}

	//is
	bool isInside(int x, int y) const					{return x >= 0 && y >= 0 && x < w && y < h;}
	bool isInside(const Vec2i &pos) const				{return isInside(pos.x, pos.y);}
	bool isInsideTile(int sx, int sy) const				{return sx >= 0 && sy >= 0 && sx < tileW && sy < tileH;}
	bool isInsideTile(const Vec2i &sPos) const			{return isInsideTile(sPos.x, sPos.y);}
	bool isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos) const;

	//free cells
	// This should just do a look up in the map metrics (currently maintained by the PathFinder object)
	// Is a cell of a given field 'free' to be occupied
	//bool isFreeCell(const Vec2i &pos, Zone field) const;
	bool isFreeCell(const Vec2i &pos, Field field) const;

	//bool areFreeCells(const Vec2i &pos, int size, Zone field) const;
	bool areFreeCells(const Vec2i &pos, int size, Field field) const;
	bool areFreeCells ( const Vec2i &pos, int size, char *fieldMap ) const;

	bool isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const;
	bool areFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const;

	bool isFreeCellOrHaveUnits(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const;
	bool areFreeCellsOrHaveUnits(const Vec2i &pos, int size, Field field, const Selection::UnitContainer &units) const;

	bool isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const;
	bool areAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const;

	void getOccupants(vector<Unit *> &results, const Vec2i &pos, int size, Zone field) const;
	//bool isFreeCell(const Vec2i &pos, Field field) const;

	bool fieldsCompatible ( Cell *cell, Field mf ) const;
	bool isFieldMapCompatible ( const Vec2i &pos, const UnitType *unitType ) const;

	// location calculations
	static Vec2i getNearestAdjacentPos(const Vec2i &start, int size, const Vec2i &target, Field field, int targetSize = 1);
	static Vec2i getNearestPos(const Vec2i &start, const Vec2i &target, int minRange, int maxRange, int targetSize = 1);
	static Vec2i getNearestPos(const Vec2i &start, const Unit *target, int minRange, int maxRange) {
		return getNearestPos(start, target->getPos(), minRange, maxRange, target->getType()->getSize());
	}

	bool getNearestAdjacentFreePos(Vec2i &result, const Unit *unit, const Vec2i &start, int size, const Vec2i &target, Field field, int targetSize = 1) const;
	bool getNearestAdjacentFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, Field field, int targetSize = 1) const {
		return getNearestAdjacentFreePos(result, unit, unit->getPos(), unit->getType()->getSize(), target, field, targetSize);
	}

	bool getNearestFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, int minRange, int maxRange, int targetSize = 1) const;
	bool getNearestFreePos(Vec2i &result, const Unit *unit, const Unit *target, int minRange, int maxRange) const {
		return getNearestFreePos(result, unit, target->getPos(), minRange, maxRange, target->getSize());
	}

	//unit placement
//	bool aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
	//bool canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const;
    void putUnitCells(Unit *unit, const Vec2i &pos);
	void clearUnitCells(Unit *unit, const Vec2i &pos);
	void evict(Unit *unit, const Vec2i &pos, vector<Unit*> &evicted);

	//misc
	bool isNextTo(const Vec2i &pos, const Unit *unit) const;
	void clampPos(Vec2i &pos) const{
		pos.clamp(0, 0, w - 1, h - 1);
	}

	void prepareTerrain(const Unit *unit);
	void flatternTerrain(const Unit *unit);

	//void flattenTerrain(const Unit *unit);
	void computeNormals();
	void computeInterpolatedHeights();

	void computeNormals(Rect2i range);
	void computeInterpolatedHeights(Rect2i range);

	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;

	void add(Earthquake *earthquake) 			{earthquakes.push_back(earthquake);}
	void update(float slice);

	//assertions
	#ifdef DEBUG
		void assertUnitCells(const Unit * unit);
	#else
		void assertUnitCells(const Unit * unit){}
	#endif

	//static
	static Vec2i toTileCoords(Vec2i unitPos)		{return unitPos/cellScale;}
	static Vec2i toUnitCoords(Vec2i surfPos)		{return surfPos * cellScale;}
	static string getMapPath(const string &mapName)	{return "maps/"+mapName+".gbm";}

private:
	//compute
	void smoothSurface();
	void computeNearSubmerged();
	void computeCellColors();
	void setCellTypes ();
	//void setCellType ( Vec2i pos );

	static void findNearest(Vec2i &result, const Vec2i &start, const Vec2i &candidate, float &minDistance);
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance) const;
	void findNearestFree(Vec2i &result, const Vec2i &start, int size, Field field, const Vec2i &candidate, float &minDistance, const Unit *unit) const;
};


// ===============================
// 	class PosCircularIteratorOrdered
// ===============================
/**
 * A position circular iterator who's results are garaunteed to be ordered by distance from the
 * center.  This iterator is slightly slower than PosCircularIteratorSimple, but can produce faster
 * calculations when either the nearest or furthest of a certain object is desired and the loop
 * can termainte as soon as a match is found, rather than iterating through all possibilities and
 * then calculating the nearest or furthest later.
 */
class PosCircularIteratorOrdered {
private:
	const Map &map;
	Vec2i center;
	Util::PosCircularIterator *i;

public:
	PosCircularIteratorOrdered(const Map &map, const Vec2i &center,
			Util::PosCircularIterator *i) : map(map), center(center), i(i) {}
	~PosCircularIteratorOrdered() {
		delete i;
	}

	bool getNext(Vec2i &result) {
		Vec2i offset;
		do {
			if(!i->getNext(offset)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}

	bool getPrev(Vec2i &result) {
		Vec2i offset;
		do {
			if(!i->getPrev(offset)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}

	bool getNext(Vec2i &result, float &dist) {
		Vec2i offset;
		do {
			if(!i->getNext(offset, dist)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}

	bool getPrev(Vec2i &result, float &dist) {
		Vec2i offset;
		do {
			if(!i->getPrev(offset, dist)) {
				return false;
			}
			result = center + offset;
		} while(!map.isInside(result));

		return true;
	}
};
// ===============================
// 	class PosCircularIteratorSimple
// ===============================
/**
 * A position circular iterator that is more primitive and light weight than the
 * PosCircularIteratorOrdered class.  It's best used when order is not important as it uses less CPU
 * cycles for a full iteration than PosCircularIteratorOrdered (and makes code smaller).
 */
class PosCircularIteratorSimple {
private:
	const Map &map;
	Vec2i center;
	int radius;
	Vec2i pos;

public:
	PosCircularIteratorSimple(const Map &map, const Vec2i &center, int radius);
	bool getNext(Vec2i &result, float &dist) {
		//iterate while dont find a cell that is inside the world
		//and at less or equal distance that the radius
		do {
			pos.x++;
			if (pos.x > center.x + radius) {
				pos.x = center.x - radius;
				pos.y++;
			}
			if (pos.y > center.y + radius) {
				return false;
			}
			result = pos;
			dist = pos.dist(center);
		} while (floor(dist) >= (radius + 1) || !map.isInside(pos));
		//while(!(pos.dist(center) <= radius && map.isInside(pos)));

		return true;
	}

	const Vec2i &getPos() {
		return pos;
	}
};

/*
class PosCircularIterator{
private:
	Vec2i center;
	int radius;
	const Map *map;
	Vec2i pos;

public:
	PosCircularIterator(const Map *map, const Vec2i &center, int radius);
	bool next(){

		//iterate while dont find a cell that is inside the world
		//and at less or equal distance that the radius
		do{
			pos.x++;
			if(pos.x > center.x+radius){
				pos.x= center.x-radius;
				pos.y++;
			}
			if(pos.y>center.y+radius)
				return false;
		}
		while(floor(pos.dist(center)) >= (radius+1) || !map->isInside(pos));
		//while(!(pos.dist(center) <= radius && map->isInside(pos)));

		return true;
	}

	const Vec2i &getPos(){
		return pos;
	}
};
*/
// ==================================
// 	class PosCircularInsideOutIterator
// ==================================

/* *
 * A position iterator which traverses (mostly) from the nearest points to the center to the
 * furthest.  The "mostly" is because it may return a position that is more diagonal before
 * returning a position that is less diagonal, but one step further out on either the x or y
 * axis.  None the less, the behavior of this iterator should be sufficient for targeting
 * purposes where small differences in distance will not have a negative effect and are acceptable
 * to reduce CPU cycles.
 */
 /*
class PosCircularInsideOutIterator {
private:
	Vec2i center;
	int radius;
	const Map *map;
	Vec2i pos;
	int step;
	int off;
	int cycle;
	bool cornerOutOfRange;

public:
	PosCircularInsideOutIterator(const Map *map, const Vec2i &center, int radius);

	bool isOutOfRange(const Vec2i &p) {
		return p.length() > radius;
	}

	bool next() {
		do {
			if(off ? cycle == 7 : cycle == 3) {
				cycle = 0;
				if(off && (off == step || (cornerOutOfRange && isOutOfRange(Vec2i(step, off))))) {
					if(step > radius) {
						return false;
					} else {
						++step;
						off = 0;
						cornerOutOfRange = isOutOfRange(Vec2i(step));
					}
				}
			} else {
				++cycle;
			}
			pos = center;
			switch(cycle) {
				case 0: pos += Vec2i( off,  -step);	break;
				case 1: pos += Vec2i( step,  off);	break;
				case 2: pos += Vec2i( off,   step);	break;
				case 3: pos += Vec2i(-step,  off);	break;
				case 4: pos += Vec2i(-off,  -step);	break;
				case 5: pos += Vec2i( step, -off);	break;
				case 7: pos += Vec2i(-off,   step);	break;
				case 8: pos += Vec2i(-step, -off);	break;
			}
		} while(!map->isInside(pos));

		return true;
	}

	bool next(float &dist) {
		do {
			if(off ? cycle == 7 : cycle == 3) {
				cycle = 0;
				dist = Vec2i(step, off).length();
				if(off == step || dist > radius) {
					if(step > radius) {
						return false;
					} else {
						++step;
						off = 0;
					}
				}
			} else {
				++cycle;
			}
			pos = center;
			switch(cycle) {
				case 0: pos += Vec2i( off,  -step);	break;
				case 1: pos += Vec2i( step,  off);	break;
				case 2: pos += Vec2i( off,   step);	break;
				case 3: pos += Vec2i(-step,  off);	break;
				case 4: pos += Vec2i(-off,  -step);	break;
				case 5: pos += Vec2i( step, -off);	break;
				case 7: pos += Vec2i(-off,   step);	break;
				case 8: pos += Vec2i(-step, -off);	break;
			}
		} while(!map->isInside(pos));

		return true;
	}

	const Vec2i &getPos() {
		return pos;
	}
};
*/

// ===============================
// 	class PosQuadIterator
// ===============================

class PosQuadIterator {
private:
	Quad2i quad;
	int step;
	Rect2i boundingRect;
	Vec2i pos;

public:
	PosQuadIterator(const Quad2i &quad, int step = 1);

	bool next() {
		do {
			pos.x += step;
			if (pos.x > boundingRect.p[1].x) {
				pos.x = (boundingRect.p[0].x / step) * step;
				pos.y += step;
			}
			if (pos.y > boundingRect.p[1].y)
				return false;
		} while (!quad.isInside(pos));

		return true;
	}

	void skipX() {
		pos.x += step;
	}

	const Vec2i &getPos() {
		return pos;
	}
};

} // end namespace

#endif
