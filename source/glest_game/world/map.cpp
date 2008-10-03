// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "map.h"

#include <cassert>

#include "world.h"
#include "tileset.h"
#include "unit.h"
#include "resource.h"
#include "logger.h"
#include "tech_tree.h"
#include "config.h"
#include "leak_dumper.h"
#include "socket.h"
#include "selection.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Cell
// =====================================================


// ==================== misc ====================

//returns if the cell is free
bool Cell::isFree(Field field) const {
	return getUnit(field)==NULL || getUnit(field)->isPutrefacting();
}

// =====================================================
// 	class SurfaceCell
// =====================================================

SurfaceCell::SurfaceCell(){
	object= NULL;
	vertex= Vec3f(0.f);
	normal= Vec3f(0.f, 1.f, 0.f);
	surfaceType= -1;
	surfaceTexture= NULL;
}

SurfaceCell::~SurfaceCell(){
	if(object) {
		delete object;
	}
}

void SurfaceCell::deleteResource() {
	delete object;
	object= NULL;
}

/*
This version of the function does it the right way by creating the new object if it doesn't exist,
etc.
void SurfaceCell::load(const XmlNode *node, World *world) {
	XmlNode *n;
	Vec2i pos;
	pos.x = node->getAttribute("x")->getIntValue();
	pos.y = node->getAttribute("y")->getIntValue();
	n = node->getChild("object", 0, false);
	if (n) {
		int objectClass = n->getAttribute("value")->getIntValue();
		ObjectType *objectType = world->getTileset()->getObjectType(objectClass);
		object = new Object(objectType, vertex);
	}
	n = node->getChild("resource", 0, false);
	if (n) {
		if (!object) {
			object = new Object(NULL, vertex);
		}
		object->setResource(world->getTechTree()->getResourceType(n->getAttribute("type")->getValue()), world->getMap()->toUnitCoords(pos));
		object->getResource()->setAmount(n->getAttribute("amount")->getIntValue());
	}
	n = node->getChild("explored", 0, false);
	if (n) {
		for (int i = 0; i < n->getChildCount(); i++) {
			explored[n->getChildIntValue("team")] = true;
		}
	}
}
*/

void SurfaceCell::read(NetworkDataBuffer &buf) {
	uint8 a;
	buf.read(a);
	explored[0] = a & 0x01;
	explored[1] = a & 0x02;
	explored[2] = a & 0x04;
	explored[3] = a & 0x08;

	if(a & 0x10) {
		/* We're not using object or resource type right now
		if(a & 0x20) {
			uint8 objectType;
			buf.read(objectType);
		}

		if(a & 0x40) {
			uint8 resourceTypeId;
			buf.read(resourceTypeId);

			if(a & 0x80) {
				uint32 resourceAmount;
				buf.read(resourceAmount);
				assert(object && object->getResource());
				if(object && object->getResource()) {
					object->getResource()->setAmount(resourceAmount);
				}
			}
		}*/

		// If there is an object, with a resource and that amount differs from the default
		if((a & 0x70) == 0x70) {
			// sanity check, let's have a real exception rather than crash if these don't match.
			if(!object) {
				throw runtime_error("Expected SurfaceCell at " + floatToStr(vertex.x) + ", "
						+ floatToStr(vertex.z) + " to have an object, but it did not.  This "
						"probably means that you modified the map since this game was saved or if "
						"this is a network game, that you have a different map than the server.");
			}
			if(!object->getResource()) {
				throw runtime_error("Expected SurfaceCell at " + floatToStr(vertex.x) + ", "
						+ floatToStr(vertex.z) + " to have a resource, but it did not.  This "
						"probably means that you modified the map since this game was saved or if "
						"this is a network game, that you have a different map than the server.");
			}

			if(a & 0x80) {
				uint32 amount32;
				buf.read(amount32);
				object->getResource()->setAmount(amount32);
			} else {
				uint16 amount16;
				buf.read(amount16);
				object->getResource()->setAmount(amount16);
			}
		}
	} else if(object) {
		delete object;
		object = NULL;
	}
}

/**
 * byte 0:
 *   bits 0-3: explored states for each team (only 4 teams supported)
 *   bit 4: high if there is an object
 *   bit 5: high if object has a resource
 *   bit 6: high if resource amount differs from default (amount coming in next bytes)
 *   bit 7: high if amount is 32 bits, low if amount is 16 bits.
 */
void SurfaceCell::write(NetworkDataBuffer &buf) const {
	uint8 a = (explored[0] ? 0x01 : 0)
			| (explored[1] ? 0x02 : 0)
			| (explored[2] ? 0x04 : 0)
			| (explored[3] ? 0x08 : 0);
//	uint8 objectType = 0;
//	uint8 resourceTypeId = 0;
	uint32 amount32 = 0;
	if(object) {
		a = a | 0x10;
//		const ObjectType *ot = object->getType();
//		if(ot) {
//			objectType = ot->getClass();
//			a = a | 0x20;
//		}
		Resource *r = object->getResource();
		if(r) {
			a = a | 0x20;
			const ResourceType *rt = r->getType();
//			resourceTypeId = rt->getId();
			amount32 = r->getAmount();
			if(amount32 != rt->getDefResPerPatch()) {
				a = a | 0x40;
				if(amount32 > USHRT_MAX) {
					a = a | 0x80;
				}
			}
		}
	}
	buf.write(a);

	/*	We have no reason to store these right now, so let's leave them out but keep them in
		comments for now in case we decide we need it for something in the near future.
	if(a & 0x20) {
		buf.write(objectType);
	}

	if(a & 0x40) {
		buf.write(resourceTypeId);
	}
	*/

	// If there is an object, with a resource and that amount differs from the default
	if((a & 0x70) == 0x70) {
		// do we need 32 bits or can we use16?
		if(a & 0x80) {
			buf.write(amount32);
		} else {
			uint16 amount16 = (uint16)amount32;
			buf.write(amount16);
		}
	}
}

// =====================================================
// 	class Map
// =====================================================

// ===================== PUBLIC ========================

const int Map::cellScale= 2;
const int Map::mapScale= 2;

Map::Map(){
	cells= NULL;
	surfaceCells= NULL;
	startLocations= NULL;

	// If this is expanded, maintain SurfaceCell::read() and write()
	assert(Tileset::objCount < 256);

	// same as above
	assert(GameConstants::maxPlayers == 4);
}

Map::~Map(){
	Logger::getInstance().add("Cells", true);

	delete [] cells;
	delete [] surfaceCells;
	delete [] startLocations;
}

void Map::load(const string &path, TechTree *techTree, Tileset *tileset){

	struct MapFileHeader{
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
		int8 author[128];
		int8 description[256];
	};

	try{
		FILE *f= fopen(path.c_str(), "rb");
		if(f!=NULL){

			//read header
			MapFileHeader header;
			fread(&header, sizeof(MapFileHeader), 1, f);

			if(next2Power(header.width) != header.width){
				throw runtime_error("Map width is not a power of 2");
			}

			if(next2Power(header.height) != header.height){
				throw runtime_error("Map height is not a power of 2");
			}

			heightFactor= (float)header.altFactor;
			waterLevel= static_cast<float>((header.waterLevel-0.01f)/heightFactor);
			title= header.title;
			maxPlayers= header.maxPlayers;
			surfaceW= header.width;
			surfaceH= header.height;
			w= surfaceW*cellScale;
			h= surfaceH*cellScale;


			//start locations
			startLocations= new Vec2i[maxPlayers];
			for(int i=0; i<maxPlayers; ++i){
				int x, y;
				fread(&x, sizeof(int32), 1, f);
				fread(&y, sizeof(int32), 1, f);
				startLocations[i]= Vec2i(x, y)*cellScale;
			}


			//cells
			cells= new Cell[w*h];
			surfaceCells= new SurfaceCell[surfaceW*surfaceH];

			//read heightmap
			for(int j=0; j<surfaceH; ++j){
				for(int i=0; i<surfaceW; ++i){
					float32 alt;
					fread(&alt, sizeof(float32), 1, f);
					SurfaceCell *sc= getSurfaceCell(i, j);
					sc->setVertex(Vec3f((float)(i * mapScale), alt / heightFactor, (float)(j * mapScale)));
				}
			}

			//read surfaces
			for(int j=0; j<surfaceH; ++j){
				for(int i=0; i<surfaceW; ++i){
					int8 surf;
					fread(&surf, sizeof(int8), 1, f);
					getSurfaceCell(i, j)->setSurfaceType(surf-1);
				}
			}

			//read objects and resources
			for(int j=0; j<h; j+= cellScale){
				for(int i=0; i<w; i+= cellScale){

					int8 objNumber;
					fread(&objNumber, sizeof(int8), 1, f);
					SurfaceCell *sc= getSurfaceCell(toSurfCoords(Vec2i(i, j)));
					if(objNumber==0){
						sc->setObject(NULL);
					}
					else if(objNumber <= Tileset::objCount){
						Object *o= new Object(tileset->getObjectType(objNumber-1), sc->getVertex());
						sc->setObject(o);
						for(int k=0; k<techTree->getResourceTypeCount(); ++k){
							const ResourceType *rt= techTree->getResourceType(k);
							if(rt->getClass()==rcTileset && rt->getTilesetObject()==objNumber){
								o->setResource(rt, Vec2i(i, j));
							}
						}
					}
					else{
						const ResourceType *rt= techTree->getTechResourceType(objNumber - Tileset::objCount) ;
						Object *o= new Object(NULL, sc->getVertex());
						o->setResource(rt, Vec2i(i, j));
						sc->setObject(o);
					}
				}
			}
		}
		else{
			throw runtime_error("Can't open file");
		}

	}
	catch(const exception &e){
		throw runtime_error("Error loading map: "+ path+ "\n"+ e.what());
	}
}

void Map::init(){
	Logger::getInstance().add("Heightmap computations", true);
	smoothSurface();
	computeNormals();
	computeInterpolatedHeights();
	computeNearSubmerged();
	computeCellColors();
}

void Map::read(NetworkDataBuffer &buf) {

	for(int y = 0; y < surfaceH; ++y) {
		for(int x = 0; x < surfaceW; ++x) {
			getSurfaceCell(x, y)->read(buf);
		}
	}
/*
	int thisTeam = world->getThisTeamIndex();
	Minimap *minimap = (Minimap *)world->getMinimap();

	// smooth edges of fow alpha
	for(int y = 0; y < surfaceH; ++y) {
		for(int x = 0; x < surfaceW; ++x) {
			if(!getSurfaceCell(x, y)->isExplored(thisTeam)) {
				continue;
			}
			// count the number of adjacent cells that are explored, allocating .5 for diagonal
			// matches with a total max of 6.0
			float adjacent =
					  (x + 1 < surfaceW	&& getSurfaceCell(x + 1, y)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (x - 1 >= 0		&& getSurfaceCell(x - 1, y)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (y + 1 < surfaceH	&& getSurfaceCell(x, y + 1)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (y - 1 >= 0		&& getSurfaceCell(x, y - 1)->isExplored(thisTeam) ? 1.f : 0.0f)
					+ (x + 1 < surfaceW	&& y + 1 < surfaceH	&& getSurfaceCell(x + 1, y + 1)->isExplored(thisTeam) ? 0.5f : 0.0f)
					+ (x + 1 < surfaceW	&& y - 1 >= 0		&& getSurfaceCell(x + 1, y - 1)->isExplored(thisTeam) ? 0.5f : 0.0f)
					+ (x - 1 >= 0		&& y + 1 < surfaceH	&& getSurfaceCell(x - 1, y + 1)->isExplored(thisTeam) ? 0.5f : 0.0f)
					+ (x - 1 >= 0		&& y - 1 >= 0		&& getSurfaceCell(x - 1, y - 1)->isExplored(thisTeam) ? 0.5f : 0.0f);
			// if less than 5, it remains
			if(adjacent >= 5.f) {
				// alpha between 0.25 and 0.5 depending upon how many surrounding are explored
				minimap->incFowTextureAlphaSurface(Vec2i(x, y), 0.25 + (adjacent - 5.f) / 4.f);
			}
		}
	}

	minimap->updateFowTex(1.f);
*/
}

void Map::write(NetworkDataBuffer &buf) const {
	for(int y = 0; y < surfaceH; ++y) {
		for(int x = 0; x < surfaceW; ++x) {
			getSurfaceCell(x, y)->write(buf);
		}
	}
}

// ==================== is ====================

//returns if there is a resource next to a unit, in "resourcePos" is stored the relative position of the resource
bool Map::isResourceNear(const Vec2i &pos, const ResourceType *rt, Vec2i &resourcePos) const{
	for(int i=-1; i<=1; ++i){
		for(int j=-1; j<=1; ++j){
			if(isInside(pos.x+i, pos.y+j)){
				Resource *r= getSurfaceCell(toSurfCoords(Vec2i(pos.x+i, pos.y+j)))->getResource();
				if(r!=NULL){
					if(r->getType()==rt){
						resourcePos= pos + Vec2i(i,j);
						return true;
					}
				}
			}
		}
	}
	return false;
}


// ==================== free cells ====================

bool Map::isFreeCell(const Vec2i &pos, Field field) const{
	return
		isInside(pos) &&
		getCell(pos)->isFree(field) &&
		(field==fAir || getSurfaceCell(toSurfCoords(pos))->isFree()) &&
		(field!=fLand || !getDeepSubmerged(getCell(pos)));
}

bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Unit *unit) const{
	if(isInside(pos)){
		Cell *c= getCell(pos);
		if(c->getUnit(unit->getCurrField())==unit){
			return true;
		}
		else{
			return isFreeCell(pos, field);
		}
	}
	return false;
}

bool Map::isFreeCellOrHasUnit(const Vec2i &pos, Field field, const Selection::UnitContainer &units) const {
	if(isInside(pos)) {
		Unit *containedUnit = getCell(pos)->getUnit(field);
		if(containedUnit) {
			Selection::UnitContainer::const_iterator i;
			for(i = units.begin(); i != units.end(); ++i) {
				if(containedUnit == *i) {
					return true;
				}
			}
		}

		return isFreeCell(pos, field);
	}
	return false;
}

bool Map::isAproxFreeCell(const Vec2i &pos, Field field, int teamIndex) const{

	if(isInside(pos)){
		const SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));

		if(sc->isVisible(teamIndex)){
			return isFreeCell(pos, field);
		}
		else if(sc->isExplored(teamIndex)){
			return field==fLand? sc->isFree() && !getDeepSubmerged(getCell(pos)): true;
		}
		else{
			return true;
		}
	}
	return false;
}

bool Map::isFreeCells(const Vec2i & pos, int size, Field field) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCell(Vec2i(i,j), field)){
                return false;
			}
		}
	}
    return true;
}

bool Map::isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Unit *unit) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCellOrHasUnit(Vec2i(i,j), field, unit)){
                return false;
			}
		}
	}
    return true;
}

bool Map::isFreeCellsOrHasUnit(const Vec2i &pos, int size, Field field, const Selection::UnitContainer &units) const {
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isFreeCellOrHasUnit(Vec2i(i,j), field, units)){
                return false;
			}
		}
	}
    return true;
}

bool Map::isAproxFreeCells(const Vec2i &pos, int size, Field field, int teamIndex) const{
	for(int i=pos.x; i<pos.x+size; ++i){
		for(int j=pos.y; j<pos.y+size; ++j){
			if(!isAproxFreeCell(Vec2i(i, j), field, teamIndex)){
                return false;
			}
		}
	}
    return true;
}

// ==================== location calculations ====================

//Rect2(pos.x, pos.y, pos.x + size, pos.y + size)
/**
 * Find the nearest position from target to orig that is at least minRange from
 * the target, but no greater than maxRange.  If target is less than minRange,
 * the nearest location at least minRange is returned.  If orig and target are the
 * same and minRange is > 0, the return value will contain an undefined position.
 */
Vec2i Map::getNearestPos(const Vec2i &start, const Vec2i &target, int minRange, int maxRange, int targetSize) {
	assert(minRange >= 0);
	assert(minRange <= maxRange);
	assert(targetSize > 0);
	Vec2f ftarget;

	if(start == target) {
		return start;
/*		Vec2i ret;
		if(map->getNearestAdjacentFreePos(ret, NULL, 1, start, target, minRange, fLand)) {
			return ret;
		} else {
			return start;
		}*/
	}

	if(targetSize > 1) {
		// use a circular distance around the parameter
		int offset = targetSize - 1;

		if (start.x > target.x + offset) {
			ftarget.x = target.x + offset;
		} else if(start.x > target.x) {
			ftarget.x = start.x;
		} else {
			ftarget.x = target.x;
		}

		if (start.y > target.y + offset) {
			ftarget.y = target.y + offset;
		} else if(start.y > target.y) {
			ftarget.y = start.y;
		} else {
			ftarget.y = target.y;
		}
	} else {
		ftarget.x = target.x;
		ftarget.y = target.y;
	}

	Vec2f fstart(start.x, start.y);
	//Vec2f ftarget(target.x, target.y);
	float len = fstart.dist(ftarget);

	if((int)len >= minRange && (int)len <= maxRange) {
		return start;
	} else {
		// make sure we don't divide by zero, that's how this whole universe
		// thing got started in the first place (fyi, it wasn't a big bang).
		assert(minRange == 0 || start.x != target.x || start.y != target.y);
		Vec2f result = ftarget.lerp((float)(len <= minRange ? minRange : maxRange) / len, fstart);
 		return Vec2i((int)roundf(result.x), (int)roundf(result.y));
	}
}

bool Map::getNearestFreePos(Vec2i &result, const Unit *unit, const Vec2i &target, int minRange, int maxRange, int targetSize) const {
	throw runtime_error("not implemented");
	return false;
}

/**
 * Finds a location for unit that is adjacent to the target of the given specs.
 */
Vec2i Map::getNearestAdjacentPos(const Vec2i &start, int size, const Vec2i &target, Field field, int targetSize) {
	int sideSize = targetSize + size;
	Vec2i result;
	float noneFound = 100000.f;
	float minDistance = noneFound;

	for(int i = 0; i < sideSize; ++i) {
		float dist;

		Vec2i above(target.x - size + i, target.y - size);
		dist = above.dist(start);
		if(dist < minDistance) {
			minDistance = dist;
			result = above;
		}

		Vec2i right(target.x + targetSize + size, target.y - size + i);
		dist = right.dist(start);
		if(dist < minDistance) {
			minDistance = dist;
			result = right;
		}

		Vec2i below(target.x - size + 1 + i, target.y + targetSize + size);
		dist = below.dist(start);
		if(dist < minDistance) {
			minDistance = dist;
			result = below;
		}

		Vec2i left(target.x - size, target.y - size + 1 + i);
		dist = left.dist(start);
		if(dist < minDistance) {
			minDistance = dist;
			result = left;
		}
	}

	return result;
}


bool Map::getNearestAdjacentFreePos(Vec2i &result, const Unit *unit, const Vec2i &start, int size,
		const Vec2i &target, Field field, int targetSize) const {
	int sideSize = targetSize + size;
	float noneFound = h + w;
	float minDistance = noneFound;

	for(int i = 0; i < sideSize; ++i) {
		float dist;

		Vec2i above(target.x - size + i, target.y - size);
		if(unit ? isFreeCellsOrHasUnit(above, size, field, unit)
				: isFreeCells(above, size, field)) {
			dist = above.dist(start);
			if(dist < minDistance) {
				minDistance = dist;
				result = above;
			}
		}

		Vec2i right(target.x + targetSize + size, target.y - size + i);
		if(unit ? isFreeCellsOrHasUnit(right, size, field, unit)
				: isFreeCells(right, size, field)) {
			dist = right.dist(start);
			if(dist < minDistance) {
				minDistance = dist;
				result = right;
			}
		}

		Vec2i below(target.x - size + 1 + i, target.y + targetSize + size);
		if(unit ? isFreeCellsOrHasUnit(below, size, field, unit)
				: isFreeCells(below, size, field)) {
			dist = below.dist(start);
			if(dist < minDistance) {
				minDistance = dist;
				result = below;
			}
		}

		Vec2i left(target.x - size, target.y - size + 1 + i);
		if(unit ? isFreeCellsOrHasUnit(left, size, field, unit)
				: isFreeCells(left, size, field)) {
			dist = left.dist(start);
			if(dist < minDistance) {
				minDistance = dist;
				result = left;
			}
		}
	}

	return minDistance < noneFound;
}


// ==================== unit placement ====================

//checks if a unit can move from between 2 cells
bool Map::canMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const{
	int size= unit->getType()->getSize();

	for(int i=pos2.x; i<pos2.x+size; ++i){
		for(int j=pos2.y; j<pos2.y+size; ++j){
			if(isInside(i, j)){
				if(getCell(i, j)->getUnit(unit->getCurrField())!=unit){
					if(!isFreeCell(Vec2i(i, j), unit->getCurrField())){
						return false;
					}
				}
			}
			else{
				return false;
			}
		}
	}
    return true;
}

//checks if a unit can move from between 2 cells using only visible cells (for pathfinding)
bool Map::aproxCanMove(const Unit *unit, const Vec2i &pos1, const Vec2i &pos2) const{
	int size= unit->getType()->getSize();
	int teamIndex= unit->getTeam();
	Field field= unit->getCurrField();

	//single cell units
	if(size==1){
		if(!isAproxFreeCell(pos2, field, teamIndex)){
			return false;
		}
		if(pos1.x!=pos2.x && pos1.y!=pos2.y){
			if(!isAproxFreeCell(Vec2i(pos1.x, pos2.y), field, teamIndex)){
				return false;
			}
			if(!isAproxFreeCell(Vec2i(pos2.x, pos1.y), field, teamIndex)){
				return false;
			}
		}
		return true;
	}

	//multi cell units
	else{
		for(int i=pos2.x; i<pos2.x+size; ++i){
			for(int j=pos2.y; j<pos2.y+size; ++j){
				if(isInside(i, j)){
					if(getCell(i, j)->getUnit(unit->getCurrField())!=unit){
						if(!isAproxFreeCell(Vec2i(i, j), field, teamIndex)){
							return false;
						}
					}
				}
				else{
					return false;
				}
			}
		}
		return true;
	}
}

//put a units into the cells
void Map::putUnitCells(Unit *unit, const Vec2i &pos){

	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Field field = unit->getCurrField();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = pos + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				assert(getCell(currPos)->getUnit(field) == NULL);
				getCell(currPos)->setUnit(field, unit);
			}
		}
	}
	unit->setPos(pos);
}

//removes a unit from cells
void Map::clearUnitCells(Unit *unit, const Vec2i &pos){

	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Field field = unit->getCurrField();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = pos + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				assert(getCell(currPos)->getUnit(field) == unit);
				getCell(currPos)->setUnit(field, NULL);
			}
		}
	}
}

//eviects current inhabitance of cells
void Map::evict(Unit *unit, const Vec2i &pos, vector<Unit *> &evicted) {

	assert(unit);
	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Field field = unit->getCurrField();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos(pos.x + x, pos.y + y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				Unit *evictedUnit = getCell(currPos)->getUnit(field);
				if(evictedUnit) {
					clearUnitCells(evictedUnit, evictedUnit->getPos());
					evicted.push_back(evictedUnit);
				}
			}
		}
	}
}

// ==================== misc ====================

//returnis if unit is next to pos
bool Map::isNextTo(const Vec2i &pos, const Unit *unit) const{

	for(int i=-1; i<=1; ++i){
		for(int j=-1; j<=1; ++j){
			if(isInside(pos.x+i, pos.y+j)) {
				if(getCell(pos.x+i, pos.y+j)->getUnit(fLand)==unit){
					return true;
				}
			}
		}
	}
    return false;
}

void Map::prepareTerrain(const Unit *unit){
	flatternTerrain(unit);
    computeNormals();
	computeInterpolatedHeights();
}

// ==================== PRIVATE ====================

// ==================== compute ====================

void Map::flatternTerrain(const Unit *unit){
	float refHeight= getSurfaceCell(toSurfCoords(unit->getCenteredPos()))->getHeight();
	for(int i=-1; i<=unit->getType()->getSize(); ++i){
        for(int j=-1; j<=unit->getType()->getSize(); ++j){
            Vec2i pos= unit->getPos()+Vec2i(i, j);
			Cell *c= getCell(pos);
			SurfaceCell *sc= getSurfaceCell(toSurfCoords(pos));
            //we change height if pos is inside world, if its free or ocupied by the currenty building
			if(isInside(pos) && sc->getObject()==NULL && (c->getUnit(fLand)==NULL || c->getUnit(fLand)==unit)){
				sc->setHeight(refHeight);
            }
        }
    }
}

//compute normals
void Map::computeNormals(){
    //compute center normals
    for(int i=1; i<surfaceW-1; ++i){
        for(int j=1; j<surfaceH-1; ++j){
            getSurfaceCell(i, j)->setNormal(
				getSurfaceCell(i, j)->getVertex().normal(getSurfaceCell(i, j-1)->getVertex(),
					getSurfaceCell(i+1, j)->getVertex(),
					getSurfaceCell(i, j+1)->getVertex(),
					getSurfaceCell(i-1, j)->getVertex()));
        }
    }
}

void Map::computeInterpolatedHeights(){

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			getCell(i, j)->setHeight(getSurfaceCell(toSurfCoords(Vec2i(i, j)))->getHeight());
		}
	}

	for(int i=1; i<surfaceW-1; ++i){
		for(int j=1; j<surfaceH-1; ++j){
			for(int k=0; k<cellScale; ++k){
				for(int l=0; l<cellScale; ++l){
					if(k==0 && l==0){
						getCell(i*cellScale, j*cellScale)->setHeight(getSurfaceCell(i, j)->getHeight());
					}
					else if(k!=0 && l==0){
						getCell(i*cellScale+k, j*cellScale)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i+1, j)->getHeight())/2.f);
					}
					else if(l!=0 && k==0){
						getCell(i*cellScale, j*cellScale+l)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i, j+1)->getHeight())/2.f);
					}
					else{
						getCell(i*cellScale+k, j*cellScale+l)->setHeight((
							getSurfaceCell(i, j)->getHeight()+
							getSurfaceCell(i, j+1)->getHeight()+
							getSurfaceCell(i+1, j)->getHeight()+
							getSurfaceCell(i+1, j+1)->getHeight())/4.f);
					}
				}
			}
		}
	}
}


void Map::smoothSurface(){

	float *oldHeights= new float[surfaceW*surfaceH];

	for(int i=0; i<surfaceW*surfaceH; ++i){
		oldHeights[i]= surfaceCells[i].getHeight();
	}

	for(int i=1; i<surfaceW-1; ++i){
		for(int j=1; j<surfaceH-1; ++j){

			float height= 0.f;
			for(int k=-1; k<=1; ++k){
				for(int l=-1; l<=1; ++l){
					height+= oldHeights[(j+k)*surfaceW+(i+l)];
				}
			}
			height/= 9.f;

			getSurfaceCell(i, j)->setHeight(height);
			Object *object= getSurfaceCell(i, j)->getObject();
			if(object!=NULL){
				object->setHeight(height);
			}
		}
	}

	delete [] oldHeights;
}

void Map::computeNearSubmerged(){
	for(int x = 0; x < surfaceW; ++x){
		for(int y = 0; y < surfaceH; ++y) {

			// Daniel's optimized version: +speed, +code size
			/*
			bool anySubmerged = getSubmerged(getSurfaceCell(x, y))
					|| (x + 1 < surfaceW && getSubmerged(getSurfaceCell(x + 1, y)))
					|| (x - 1 >= 0		 && getSubmerged(getSurfaceCell(x - 1, y)))
					|| (y + 1 < surfaceH && getSubmerged(getSurfaceCell(x, y + 1)))
					|| (y - 1 >= 0		 && getSubmerged(getSurfaceCell(x, y - 1)))
					|| (x + 1 < surfaceW && y + 1 < surfaceH && getSubmerged(getSurfaceCell(x + 1, y + 1)))
					|| (x + 1 < surfaceW && y - 1 >= 0		 && getSubmerged(getSurfaceCell(x + 1, y - 1)))
					|| (x - 1 >= 0		 && y + 1 < surfaceH && getSubmerged(getSurfaceCell(x - 1, y + 1)))
					|| (x - 1 >= 0		 && y - 1 >= 0		 && getSubmerged(getSurfaceCell(x - 1, y - 1)));
			*/

			// Martiño's version: slower, but more compact (altered from original)
			bool anySubmerged = false;
			for(int xoff = -1; xoff <= 2 && !anySubmerged; ++xoff) {
				for(int yoff = -1; yoff <= 2 && !anySubmerged; ++yoff) {
					Vec2i pos(x + xoff, y + yoff);
					if(isInsideSurface(pos) && getSubmerged(getSurfaceCell(pos))) {
						anySubmerged = true;
					}
				}
			}
			getSurfaceCell(x, y)->setNearSubmerged(anySubmerged);
		}
	}
}

void Map::computeCellColors(){
	for(int i=0; i<surfaceW; ++i){
		for(int j=0; j<surfaceH; ++j){
			SurfaceCell *sc= getSurfaceCell(i, j);
			if(getDeepSubmerged(sc)){
				float factor= clamp(waterLevel-sc->getHeight()*1.5f, 1.f, 1.5f);
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f)/factor);
			}
			else{
				sc->setColor(Vec3f(1.0f, 1.0f, 1.0f));
			}
		}
	}
}

#ifdef DEBUG
//makes sure a unit is in cells if alive or not if not alive
void Map::assertUnitCells(const Unit * unit) {
	assert(unit);
	// make sure alive/dead is sane
	assert(unit->getHp() == 0 && unit->isDead() || unit->getHp() > 0 && unit->isAlive());

	const UnitType *ut = unit->getType();
	int size = ut->getSize();
	Field field = unit->getCurrField();

	for(int x = 0; x < size; ++x) {
		for(int y = 0; y < size; ++y) {
			Vec2i currPos = unit->getPos() + Vec2i(x, y);
			assert(isInside(currPos));

			if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
				if(unit->isAlive()) {
					assert(getCell(currPos)->getUnit(field) == unit);
				} else {
					assert(getCell(currPos)->getUnit(field) != unit);
				}

			}
		}
	}
}
#endif


// =====================================================
// 	class PosCircularIterator
// =====================================================

PosCircularIterator::PosCircularIterator(const Map *map, const Vec2i &center, int radius){
	this->map= map;
	this->radius= radius;
	this->center= center;
	pos= center - Vec2i(radius, radius);
	pos.x-= 1;
}



// =====================================================
// 	class PosQuadIterator
// =====================================================

PosQuadIterator::PosQuadIterator(const Map *map, const Quad2i &quad, int step){
	this->map= map;
	this->quad= quad;
	this->boundingRect= quad.computeBoundingRect();
	this->step= step;
	pos= boundingRect.p[0];
	--pos.x;
	pos.x= (pos.x/step)*step;
	pos.y= (pos.y/step)*step;
}

}}//end namespace
