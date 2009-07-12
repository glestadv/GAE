// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//               2009 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Does not use pre-compiled header because it's optimized in debug build.

// Currently DOES use precompiled header, because it's no longer optimized in debug
// because debugging optimized code is not much fun :-)

// Actually, no need for this one to be optimized anymore, when path finding is stable again, 
// path_finder_llsr.cpp should get this treatment, that's where the 'hard work' is done now.

#include "pch.h"

#include "path_finder.h"
#include "graph_search.h"

#include "config.h"
#include "map.h"
#include "unit.h"
#include "unit_type.h"
#include "world.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{ namespace PathFinder {


// =====================================================
// 	class PathFinder
// =====================================================

// ===================== PUBLIC ========================

PathFinder* PathFinder::singleton = NULL;

PathFinder* PathFinder::getInstance ()
{
   if ( ! singleton )
      singleton = new PathFinder ();
   return singleton;
}

PathFinder::PathFinder()
{
   search = new GraphSearch ();
   annotatedMap = NULL;
   singleton = this;
}

PathFinder::~PathFinder ()
{
   delete search;
   delete annotatedMap;
}

void PathFinder::init ( Map *map )
{
	this->map= map;
   delete annotatedMap;
   annotatedMap = new AnnotatedMap ( map );
   search->init ( map, annotatedMap );
}

bool PathFinder::isLegalMove ( Unit *unit, const Vec2i &pos2 ) const
{
   if ( unit->getPos().dist ( pos2 ) > 1.5 ) return false; // shouldn't really need this....

   const Vec2i &pos1 = unit->getPos ();
   const int &size = unit->getSize ();
   const Field &field = unit->getCurrField ();
   Zone cellField = field == FieldAir ? ZoneAir : ZoneSurface;
   Tile *sc = map->getTile ( map->toTileCoords ( pos2 ) );

   if ( ! annotatedMap->canOccupy ( pos2, size, field ) )
      return false;
   if ( pos1.x != pos2.x && pos1.y != pos2.y )
   {  // Proposed move is diagonal, check if cells either 'side' are free.
      Vec2i diag1, diag2;
      getDiags ( pos1, pos2, size, diag1, diag2 );
      if ( ! annotatedMap->canOccupy (diag1, 1, field) 
      ||   ! annotatedMap->canOccupy (diag2, 1, field) ) 
		   return false; // obstruction, can not move to pos2
      if ( ! map->getCell (diag1)->isFree (cellField)
      ||   ! map->getCell (diag2)->isFree (cellField) )
         return false; // other unit in the way
   }
   for ( int i = pos2.x; i < unit->getSize () + pos2.x; ++i )
      for ( int j = pos2.y; j < unit->getSize () + pos2.y; ++j )
         if ( map->getCell (i,j)->getUnit (cellField) != unit
         &&   ! map->isFreeCell (Vec2i(i,j), field) ) //! map->getCell (i,j)->isFree (cellField)
            return false;
   // pos2 is free, and nothing is in the way
	return true;
}

TravelState PathFinder::findPath(Unit *unit, const Vec2i &finalPos)
{
   //Logger::getInstance ().add ( "findPath() Called..." );
   static int flipper = 0;
   //route cache
	UnitPath &path = *unit->getPath ();
	if( finalPos == unit->getPos () )
   {	//if arrived (where we wanted to go)
		unit->setCurrSkill ( scStop );
      //Logger::getInstance ().add ( "findPath() ... Returning, Arrived ..." );
		return tsArrived;
	}
   else if( ! path.isEmpty () )
   {	//route cache
		Vec2i pos = path.pop();
      if ( isLegalMove ( unit, pos ) )
      {
			unit->setNextPos ( pos );
         //Logger::getInstance ().add ( "findPath() ... Returning, On the way ..." );
			return tsOnTheWay;
		}
	}
   //route cache miss
	const Vec2i targetPos = computeNearestFreePos ( unit, finalPos );
   //Logger::getInstance ().add ( "findPath() ... route cache miss ..." );

   //if arrived (as close as we can get to it)
	if ( targetPos == unit->getPos () )
   {
      unit->setCurrSkill(scStop);
		return tsArrived;
   }
   // some tricks to determine if we are probably blocked on a short path, without
   // an exhuastive and expensive search through pathFindNodesMax nodes
   float dist = unit->getPos().dist ( targetPos );
   if ( unit->getCurrField () == FieldWalkable 
   &&   map->getTile (Map::toTileCoords ( targetPos ))->isVisible (unit->getTeam ()) )
   {
      int radius;
      if ( dist < 5 ) radius = 2;
      else if ( dist < 10 ) radius = 3;
      else if ( dist < 15 ) radius = 4;
      else radius = 5;
      if ( ! search->canPathOut ( targetPos, radius, FieldWalkable ) ) 
      {
         unit->getPath()->incBlockCount ();
         unit->setCurrSkill(scStop);
         //Logger::getInstance ().add ( "findPath() ... Returning, target blocked ..." );
         return tsBlocked;
      }
   }

   /*
   if ( flipper % 2 == 0 )
   {

   }
   else 
   {

   }
   */

   //flipper ++;

   SearchParams params (unit);
   params.dest = targetPos;
   list<Vec2i> pathList;
   //Logger::getInstance ().add ( "findPath()... annotating local..." );
   annotatedMap->annotateLocal ( unit->getPos (), unit->getSize (), unit->getCurrField () );
   //bool result = search->GreedySearch ( params, pathList );
   bool result = search->AStarSearch ( params, pathList );
   annotatedMap->clearLocalAnnotations ( unit->getCurrField () );
   //Logger::getInstance ().add ( "findPath()... clearing local annotations ..." );
   if ( ! result )
   {
      unit->getPath()->incBlockCount ();
      unit->setCurrSkill(scStop);
      //Logger::getInstance ().add ( "findPath() ... Returning, Search Failed ..." );
      return tsBlocked;
   }
   else
      copyToPath ( pathList, unit->getPath () );
	Vec2i pos = path.pop(); //crash point
   if ( ! isLegalMove ( unit, pos ) )
   {
		unit->setCurrSkill(scStop);
      unit->getPath()->incBlockCount ();
		return tsBlocked;
	}
   unit->setNextPos(pos);
	return tsOnTheWay;
}

void PathFinder::copyToPath ( const list<Vec2i> pathList, UnitPath *path )
{
   list<Vec2i>::const_iterator it = pathList.begin();
   // skip start pos, store rest
   for ( ++it; it != pathList.end(); ++it )
      path->push ( *it );
}

// ==================== PRIVATE ====================

// return finalPos if free, else a nearest free pos within maxFreeSearchRadius
// cells, or unit's current position if none found
Vec2i PathFinder::computeNearestFreePos (const Unit *unit, const Vec2i &finalPos){
	//unit data
	Vec2i unitPos= unit->getPos();
	int size= unit->getType()->getSize();
   Field field = unit->getCurrField();// == FieldAir ? ZoneAir : ZoneSurface;
	int teamIndex= unit->getTeam();

	//if finalPos is free return it
	
   if(map->areAproxFreeCells(finalPos, size, field, teamIndex)){
		return finalPos;
	}

	//find nearest pos
	Vec2i nearestPos= unitPos;
	float nearestDist= unitPos.dist(finalPos);
	for(int i= -maxFreeSearchRadius; i<=maxFreeSearchRadius; ++i){
		for(int j= -maxFreeSearchRadius; j<=maxFreeSearchRadius; ++j){
			Vec2i currPos= finalPos + Vec2i(i, j);
			if(map->areAproxFreeCells(currPos, size, field, teamIndex)){
				float dist= currPos.dist(finalPos);

				//if nearer from finalPos
				if(dist<nearestDist){
					nearestPos= currPos;
					nearestDist= dist;
				}
				//if the distance is the same compare distance to unit
				else if(dist==nearestDist){
					if(currPos.dist(unitPos)<nearestPos.dist(unitPos)){
						nearestPos= currPos;
					}
				}
			}
		}
	}
	return nearestPos;
}

} // end namespace Glest::Game::PathFinder

#ifdef DEBUG
// Why is this here?  Doesn't it belong in world.cpp?  It's here because we compile path_finder.cpp
// optimized in debug since it's the only possible way you can really debug and this is a dog slow
// function.
class ValidationMap {
public:
	int h;
	int w;
	char *cells;

	ValidationMap(int h, int w) : h(h), w(w), cells(new char[h * w * ZoneCount]) {
		reset();
	}

	void reset() {
		memset(cells, 0, h * w * ZoneCount);
	}

	void validate(int x, int y, Field field) {
		assert(!getCell(x, y, field));
		getCell(x, y, field) = 1;
	}

	char &getCell(int x, int y, Field field) {
		assert(x >= 0 && x < w);
		assert(y >= 0 && y < h);
		assert(field >= 0 && field < ZoneCount);
		return cells[field * h * w + x * w + y];
	}
};

void World::assertConsistiency() {
	// go through each map cell and make sure each unit in cells are supposed to be there
	// go through each unit and make sure they are in the cells they are supposed to be in
	// iterate through unit references: pets, master, target and make sure they are valid or
	//		null-references
	// make sure alive/dead states of all units is good.
	// whatever else I can think of

	static ValidationMap validationMap(map.getH(), map.getW());
	validationMap.reset();

	// make sure that every unit is in their cells and mark those as validated.
	for(Factions::iterator fi = factions.begin(); fi != factions.end(); ++fi) {
		for(int ui = 0; ui < fi->getUnitCount(); ++ui) {
			Unit *unit = fi->getUnit(ui);
/*
			if(!((unit->getHp() == 0 && unit->isDead()) || (unit->getHp() > 0 && unit->isAlive()))) {
				cerr << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout << "inconsisteint dead/hp state for unit " << unit->getId()
						<< " (" << unit << ") faction " << fi->getIndex() << endl;
				cout.flush();
				assert(false);
			}
*/
			if(unit->isDead() && unit->getCurrSkill()->getClass() == scDie) {
				continue;
			}

			const UnitType *ut = unit->getType();
			int size = ut->getSize();
			Field field = unit->getCurrField();
			const Vec2i &pos = unit->getPos();

			for(int x = 0; x < size; ++x) {
				for(int y = 0; y < size; ++y) {
					Vec2i currPos = pos + Vec2i(x, y);
					assert(map.isInside(currPos));

					if(!ut->hasCellMap() || ut->getCellMapCell(x, y)) {
						Unit *unitInCell = map.getCell(currPos)->getUnit(field);
						if(unitInCell != unit) {
							cerr << "Unit id " << unit->getId()
									<< " from faction " << fi->getIndex()
									<< "(type = " << unit->getType()->getName() << ")"
									<< " not in cells (" << currPos.x << ", " << currPos.y << ", " << field << ")";
							if(unitInCell == NULL && !unit->getHp()) {
								cerr << " but has zero HP and is not executing scDie." << endl;
							} else {
								cerr << endl;
								assert(false);
							}
						}
						validationMap.validate(currPos.x, currPos.y, field);
					}
				}
			}
		}
	}

	// make sure that every cell that was not validated is empty
	for(int x = 0; x < map.getW(); ++x) {
		for(int y = 0; y < map.getH(); ++y ) {
			for(int field = 0; field < ZoneCount; ++field) {
				if(!validationMap.getCell(x, y, (Field)field)) {
					Cell *cell = map.getCell(x, y);
					if(cell->getUnit(field)) {
						cerr << "Cell not empty at " << x << ", " << y << ", " << field << endl;
						cerr << "Cell has pointer to unit object at " << cell->getUnit(field) << endl;

						assert(false);
					}
				}
			}
		}
	}
}
#else
void World::assertConsistiency() {}
#endif

void World::doHackyCleanUp() {
	int h = map.getH();
	int w = map.getW();
	for(int x = 0; x < w; ++x) {
		for(int y = 0; y < h; ++y) {
			Cell *cell = map.getCell(x, y);
			for(Units::const_iterator u = newlydead.begin(); u != newlydead.end(); ++u) {
				for(int f = 0; f < ZoneCount; ++f) {
					if(cell->getUnit((Zone)f) == *u) {
						cell->setUnit((Zone)f, NULL);
					}
				}
			}
		}
	}
	newlydead.clear();
}

}} //end namespace