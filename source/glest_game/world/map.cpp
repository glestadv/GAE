// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
	delete object;
}

void SurfaceCell::deleteResource() {
	delete object;
	object= NULL;
}

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

void SurfaceCell::save(XmlNode *node, int x, int y) const {
	XmlNode *n;
	// If you change the maxPlayers, you have to rework this code.
	assert(GameConstants::maxPlayers == 4);
/*	int exp = 0;
	for (int i = 0; i < GameConstants::maxPlayers; i++) {
		if (explored[i]) {
			exp++;
		}
	}*/
	if (object/* || exp*/) {
		node = node->addChild("surfacecell");
		node->addAttribute("x", x);
		node->addAttribute("y", y);
		if (object) {
			if (object->getType()) {
				node->addChild("object", object->getType()->getClass());
			}
			if (object->getResource()) {
				n = node->addChild("resource");
				n->addAttribute("type", object->getResource()->getType()->getName());
				n->addAttribute("amount",  object->getResource()->getAmount());
			}
		}
/*		if (exp) {
			n = node->addChild("explored");
			for (int i = 0; i < GameConstants::maxPlayers; i++) {
				if (explored[i]) {
					n->addChild("team", i);
				}
			}
		}*/
	}
}
void SurfaceCell::read(NetworkDataBuffer &buf) {
	uint8 a;
	//uint16 b;
	buf.read(a);
	//buf.read(b);
	explored[0] = a & 1;
	explored[1] = a & 2;
	explored[2] = a & 4;
	explored[3] = a & 8;
	/*
	uint8 resource = a >> 4;
	if(resource) {
		if (!object) {
			object = new Object(NULL, vertex);
		} else {
		}*/

			/*
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

		getResourceType
	} else {
	}*/
}

void SurfaceCell::write(NetworkDataBuffer &buf) const {
	uint8 a = (explored[0] ? 1 : 0)
			| (explored[1] ? 2 : 0)
			| (explored[2] ? 4 : 0)
			| (explored[3] ? 8 : 0);
	uint8 b = 0;
	uint16 c = 0;
	if(object) {/*
		int objectType = 0;
		int resourceType = 0;
		ObjectType *ot = object->getType();
		Resource *r = object->getResource();
		if(ot &&
				 getClass()
		rt->getClass()==rcTileset
				tileset->getObjectType(objNumber-1)
		if(object->*/
		/*if(
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

					if(object->getType() && object->getType()->getClass()) {
			// will be less than Tileset::objCount (16)
			b = object->getType()->getClass() + 1;
		} else if (object->getResource()) {
			int resource = object->getResource()->getType()->getTilesetObject() + 1;
			int amount = object->getResource()->getAmount();

			assert(object->getResource()->getType()->getClass() == rcTileset);
			assert(resource >= 0 && resource < 16);
			assert(amount >= 0 && amount < USHRT_MAX);

			a = a | (resource & 0xf) << 4;
			b = amount > USHRT_MAX ? USHRT_MAX : amount;
		}*/
	}
	buf.write(a);
	//buf.write(b);
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
	assert(Tileset::objCount < 16);
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

void Map::load(const XmlNode *node, World *world) {
	NetworkDataBuffer buf(32768);
	uint8 goofy;
	string uglyhack;

	for(int y = 0; y < surfaceH; ++y) {
		XmlNode const *row = node->getChild("surface", y);
		uglyhack = row->getAttribute("data")->getValue();
		buf.clear();
		for(int x = 0; x < surfaceW; ++x) {
			goofy = uglyhack[x] - 'a';
			buf.write(goofy);
			getSurfaceCell(x, y)->read(buf);
		}
	}
	/*
	int x, y;
	XmlNode *n;

	for(int j=0; j<surfaceH; ++j) {
		for(int i=0; i<surfaceW; ++i) {
			getSurfaceCell(i, j)->setObject(NULL);
		}
	}
	for(int i=0; i<node->getChildCount(); ++i) {
		n = node->getChild("surfacecell", i);
		x = n->getAttribute("x")->getIntValue();
		y = n->getAttribute("y")->getIntValue();
		getSurfaceCell(x, y)->load(n, world);
	}*/
}

void Map::save(XmlNode *node) const {
	NetworkDataBuffer buf(32768);
	uint8 goofy;
	string uglyhack;
	for(int y = 0; y < surfaceH; ++y) {
		XmlNode *row = node->addChild("surface");
		row->addAttribute("row", y);
		buf.clear();
		uglyhack.clear();
		for(int x = 0; x < surfaceW; ++x) {
			getSurfaceCell(x, y)->write(buf);
			buf.read(goofy);
			uglyhack += (char)('a' + goofy);
		}

		row->addAttribute("data", uglyhack);
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
			Vec2i currPos = pos + Vec2i(x, y);
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

void Map::clampPos(Vec2i &pos) const{
	if(pos.x<0){
        pos.x=0;
	}
	if(pos.y<0){
        pos.y=0;
	}
	if(pos.x>=w){
        pos.x=w-1;
	}
	if(pos.y>=h){
        pos.y=h-1;
	}
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

	for(int i=0; i<surfaceW-1; ++i){
		for(int j=0; j<surfaceH-1; ++j){
			bool anySubmerged= false;
			for(int k=-1; k<=2; ++k){
				for(int l=-1; l<=2; ++l){
					Vec2i pos= Vec2i(i+k, j+l);
					if(isInsideSurface(pos)){
						if(getSubmerged(getSurfaceCell(pos)))
							anySubmerged= true;
					}
				}
			}
			getSurfaceCell(i, j)->setNearSubmerged(anySubmerged);
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
