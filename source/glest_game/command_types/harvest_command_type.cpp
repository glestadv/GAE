// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//               2008-2009 Daniel Santos
//               2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#include "pch.h"
#include "command_type.h"


#include "upgrade_type.h"
#include "unit_type.h"
#include "world.h"
#include "sound.h"
#include "util.h"
#include "leak_dumper.h"
#include "graphics_interface.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "game.h"
#include "renderer.h"
#include "sound_renderer.h"

#include "leak_dumper.h"


namespace Glest { namespace Game {


// =====================================================
// 	class HarvestCommandType
// =====================================================

void HarvestCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	MoveBaseCommandType::load(n, dir, tt, ft);

	//harvest
   	string skillName= n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
	harvestSkillType= static_cast<const HarvestSkillType*>(unitType->getSkillType(skillName, scHarvest));

	//stop loaded
   	skillName= n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
	stopLoadedSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, scStop));

	//move loaded
   	skillName= n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
	moveLoadedSkillType= static_cast<const MoveSkillType*>(unitType->getSkillType(skillName, scMove));

	//resources can harvest
	const XmlNode *resourcesNode= n->getChild("harvested-resources");
	for(int i=0; i<resourcesNode->getChildCount(); ++i){
		const XmlNode *resourceNode= resourcesNode->getChild("resource", i);
		harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
	}

	maxLoad= n->getChild("max-load")->getAttribute("value")->getIntValue();
	hitsPerUnit= n->getChild("hits-per-unit")->getAttribute("value")->getIntValue();
}

void HarvestCommandType::update(Unit *unit) const
{
	Command *command = unit->getCurrCommand();
	Vec2i targetPos;
   Game *game = Game::getInstance ();
   World *world = game->getWorld ();
   Map *map = world->getMap ();
   PathFinder::PathFinder *pathFinder = PathFinder::PathFinder::getInstance ();
   NetworkManager &net = NetworkManager::getInstance();

	if (unit->getCurrSkill()->getClass() != scHarvest) {
		//if not working
		if (!unit->getLoadCount()) {
			//if not loaded go for resources
			Resource *r = map->getTile(Map::toTileCoords(command->getPos()))->getResource();
			if (r && this->canHarvest(r->getType())) {
				//if can harvest dest. pos
				if (unit->getPos().dist(command->getPos()) < harvestDistance &&
						map->isResourceNear(unit->getPos(), r->getType(), targetPos)) {
					//if it finds resources it starts harvesting
					unit->setCurrSkill(this->getHarvestSkillType());
					unit->setTargetPos(targetPos);
					unit->face(targetPos);
					unit->setLoadCount(0);
					unit->setLoadType(map->getTile(Map::toTileCoords(targetPos))->getResource()->getType());
				} else {
					//if not continue walking
					switch (pathFinder->findPath(unit, command->getPos())) {
					case PathFinder::tsOnTheWay:
						unit->setCurrSkill(this->getMoveSkillType());
						unit->face(unit->getNextPos());
						break;
               case PathFinder::tsBlocked:
                  // couldn't get to the goods, look for other resources of the same type nearby.
                  // If can't find any, we're probably standing somewhere between the store 
                  // and the resources we couldn't get to (due to congestion most likely). 
                  // Move out of the way (perpendicular to store.pos - resource.pos)
                  /*
                  if ( ! searchForResource(unit, hct)) 
                  {
                     if ( unit->getCommands ().size () <= 1 )
                     {
                        Unit *store = world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
                        Vec2i storePos = store->getPos();
                        storePos.x += store->getSize() / 2;
                        storePos.y += store->getSize() / 2;
                        GetClear ( unit, storePos, command->getPos() );
                     }
                     // else the unit has something else to do anyway...
                     unit->finishCommand();
      				}
                  */
                  break;
					default:
						break;
					}
				}
			} else {
				//if can't harvest, search for another resource
				unit->setCurrSkill(scStop);
				if (!searchForResource(unit, world)) {
					unit->finishCommand();
				}
			}
		} else {
			//if loaded, return to store
			Unit *store = world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
			if (store) {
				switch (pathFinder->findPath(unit, store->getNearestOccupiedCell(unit->getPos()))) {
				case PathFinder::tsOnTheWay:
					unit->setCurrSkill(this->getMoveLoadedSkillType());
					unit->face(unit->getNextPos());
					break;
				default:
					break;
				}

				//world->changePosCells(unit,unit->getPos()+unit->getDest());
				if (map->isNextTo(unit->getPos(), store)) {

					//update resources
					int resourceAmount = unit->getLoadCount();
					if (unit->getFaction()->getCpuUltraControl()) {
						resourceAmount *= ultraResourceFactor;
					}
					unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
					world->getStats().harvest(unit->getFactionIndex(), resourceAmount);
               game->getScriptManager()->onResourceHarvested ();

					//if next to a store unload resources
					unit->getPath()->clear();
					unit->setCurrSkill(scStop);
					unit->setLoadCount(0);
					if (net.isNetworkServer()) {
						// FIXME: wasteful full update here
						net.getServerInterface()->unitUpdate(unit);
						net.getServerInterface()->updateFactions();
					}
				}
			} else {
				unit->finishCommand();
			}
		}
	} else {
		//if working
		Tile *sc = map->getTile(Map::toTileCoords(unit->getTargetPos()));
		Resource *r = sc->getResource();
		if (r != NULL) {
			//if there is a resource, continue working, until loaded
			unit->update2();
			if (unit->getProgress2() >= this->getHitsPerUnit()) {
				unit->setProgress2(0);
				unit->setLoadCount(unit->getLoadCount() + 1);

				//if resource exausted, then delete it and stop
				if (r->decAmount(1)) {
               Vec2i rPos = r->getPos ();
					sc->deleteResource();
               // let the pathfinder know
               //FIXME: make Field friendly ??? resources in other fields ???
               pathFinder->updateMapMetrics ( rPos, 2, false, FieldWalkable );
					unit->setCurrSkill(this->getStopLoadedSkillType());
				}

				if (unit->getLoadCount() == this->getMaxLoad()) {
					unit->setCurrSkill(this->getStopLoadedSkillType());
					unit->getPath()->clear();
				}
			}
		} else {
			//if there is no resource
			if (unit->getLoadCount()) {
				unit->setCurrSkill(this->getStopLoadedSkillType());
			} else {
				unit->finishCommand();
				unit->setCurrSkill(scStop);
			}
		}
	}
}


//looks for a resource of type rt, if rt==NULL looks for any
//resource the unit can harvest
bool HarvestCommandType::searchForResource ( Unit *unit, World *world ) const 
{
	Vec2i pos;

	PosCircularIteratorOrdered pci(*world->getMap(), unit->getCurrCommand()->getPos(),
			world->getPosIteratorFactory().getInsideOutIterator(1, maxResSearchRadius));

	while(pci.getNext(pos)) {
		Resource *r = world->getMap()->getTile(Map::toTileCoords(pos))->getResource();
		if (r && this->canHarvest(r->getType())) {
			unit->getCurrCommand()->setPos(pos);
			return true;
		}
	}
	return false;
}

void HarvestCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	str+= lang.get("HarvestSpeed")+": "+ intToStr(harvestSkillType->getSpeed()/hitsPerUnit);
	EnhancementTypeBase::describeModifier(str, (unit->getSpeed(harvestSkillType) - harvestSkillType->getSpeed())/hitsPerUnit);
	str+= "\n";
	str+= lang.get("MaxLoad")+": "+ intToStr(maxLoad)+"\n";
	str+= lang.get("LoadedSpeed")+": "+ intToStr(moveLoadedSkillType->getSpeed())+"\n";
	if(harvestSkillType->getEpCost()!=0){
		str+= lang.get("EpCost")+": "+intToStr(harvestSkillType->getEpCost())+"\n";
	}
	str+=lang.get("Resources")+":\n";
	for(int i=0; i<getHarvestedResourceCount(); ++i){
		str+= getHarvestedResource(i)->getName()+"\n";
	}
}

bool HarvestCommandType::canHarvest(const ResourceType *resourceType) const{
	return find(harvestedResources.begin(), harvestedResources.end(), resourceType) != harvestedResources.end();
}


}}
