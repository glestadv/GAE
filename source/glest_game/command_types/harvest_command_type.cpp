// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008-2009 Daniel Santos
//				  2009 James McCulloch <silnarm at gmail>
//				  2009 Nathan Turner <hailstone3 at sourceforge>
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

// unit cache
Resource *HarvestCommandType::resource = NULL;

bool HarvestCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = MoveBaseCommandType::load(n, dir, tt, ft);
	string skillName;
	//harvest
	try {
		skillName= n->getChild("harvest-skill")->getAttribute("value")->getRestrictedValue();
		harvestSkillType= static_cast<const HarvestSkillType*>(unitType->getSkillType(skillName, scHarvest));
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	//stop loaded
	try { 
		skillName= n->getChild("stop-loaded-skill")->getAttribute("value")->getRestrictedValue();
		stopLoadedSkillType= static_cast<const StopSkillType*>(unitType->getSkillType(skillName, scStop));
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}

	//move loaded
	try {
		skillName= n->getChild("move-loaded-skill")->getAttribute("value")->getRestrictedValue();
		moveLoadedSkillType= static_cast<const MoveSkillType*>(unitType->getSkillType(skillName, scMove));
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	//resources can harvest
	try { 
		const XmlNode *resourcesNode= n->getChild("harvested-resources");
		for(int i=0; i<resourcesNode->getChildCount(); ++i){
			const XmlNode *resourceNode= resourcesNode->getChild("resource", i);
			harvestedResources.push_back(tt->getResourceType(resourceNode->getAttribute("name")->getRestrictedValue()));
		}
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	try { maxLoad= n->getChild("max-load")->getAttribute("value")->getIntValue(); }
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	try { hitsPerUnit= n->getChild("hits-per-unit")->getAttribute("value")->getIntValue(); }
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	return loadOk;
}

void HarvestCommandType::cacheUnit(Unit *u) const {
	CommandType::cacheUnit ( u );
	resource = map->getTile(Map::toTileCoords(command->getPos()))->getResource();
}

void HarvestCommandType::update(Unit *unit) const {
	cacheUnit( unit );

	if (unit->getCurrSkill()->getClass() != scHarvest) { // Not Harvesting
		if (!unit->getLoadCount()) { // Load == 0
			if (resource && canHarvest(resource->getType())) {
				if ( !attemptToHarvest() ) {
					moveToResource();
				}
			} 
			else {
				//if can't harvest, search for another resource
				unit->setCurrSkill(scStop);
				if (!searchForResource(unit, world)) {
					unit->finishCommand();
				}
			}
		} 
		else { // Loaded
			returnToStore();
		}
	} 
	else { // Harvesting
		continueHarvesting();
	}
}

// returns true if harvesting can commence
bool HarvestCommandType::attemptToHarvest() const {
	Vec2i targetPos;

	//if can harvest dest. pos
	if (unit->getPos().dist(command->getPos()) < harvestDistance &&
		map->isResourceNear(unit->getPos(), resource->getType(), targetPos)) {
		
		//if it finds resources it starts harvesting
		unit->setCurrSkill(this->getHarvestSkillType());
		unit->setTargetPos(targetPos);
		unit->face(targetPos);
		unit->setLoadCount(0);
		unit->setLoadType(map->getTile(Map::toTileCoords(targetPos))->getResource()->getType());	
		return true;
	}
	return false;
}

void HarvestCommandType::moveToResource() const {
	switch (pathFinder->findPathToResource(unit, command->getPos(), resource->getType())) {
		case Search::tsOnTheWay:
			unit->setCurrSkill(this->getMoveSkillType());
			unit->face(unit->getNextPos());
			break;
		case Search::tsArrived:
			for ( int i=0; i < 8; ++i ) { // reset target
				Vec2i cPos = unit->getPos() + Search::OffsetsSize1Dist1[i];
				if ( resource && canHarvest (resource->getType()) ) {
					command->setPos ( cPos );
					unit->setTargetPos ( cPos );
					break;
				}
			}
		case Search::tsBlocked:
			break;
		default:
			break;
	}
}

void HarvestCommandType::returnToStore() const {
	//TODO remove unconditional call to World::nearestStore()
	Unit *store = world->nearestStore(unit->getPos(), unit->getFaction()->getIndex(), unit->getLoadType());
	if (store) {
		//TODO remove unconditional call to Map::getNearestOcupiedCell()
		switch (pathFinder->findPathToStore(unit, store->getNearestOccupiedCell(unit->getPos()),store)) {
			case Search::tsOnTheWay:
				unit->setCurrSkill(moveLoadedSkillType);
				unit->face(unit->getNextPos());
				break;
			default:
				break;
		}

		if (map->isNextTo(unit->getPos(), store)) {
			//update resources
			int resourceAmount = unit->getLoadCount();
			if (unit->getFaction()->getCpuUltraControl()) {
				resourceAmount *= ultraResourceFactor;
			}
			unit->getFaction()->incResourceAmount(unit->getLoadType(), resourceAmount);
			world->getStats().harvest(unit->getFactionIndex(), resourceAmount);

			//fire script event
			scriptManager->onResourceHarvested();

			//if next to a store unload resources
			unit->getPath()->clear();
			unit->setCurrSkill(scStop);
			unit->setLoadCount(0);
			if (net->isNetworkServer()) {
				// FIXME: wasteful full update here
				net->getServerInterface()->unitUpdate(unit);
				net->getServerInterface()->updateFactions();
			}
		}
	} else {
		unit->finishCommand();
	}
}

void HarvestCommandType::continueHarvesting() const {
	//can these be replaced with cache resource? it uses command pos and this uses unitTargetPos - hailstone	
	// yes, but you still need to check it... it may have been exhausted... -silnarm
	
	Tile *sc = map->getTile(Map::toTileCoords(unit->getTargetPos()));
	resource = sc->getResource();
	if (resource != NULL) {
		//if there is a resource, continue working, until loaded
		unit->update2();
		if (unit->getProgress2() >= hitsPerUnit) {
			unit->setProgress2(0);
			unit->setLoadCount(unit->getLoadCount() + 1);

			//if resource exausted, then delete it and stop
			if (resource->decAmount(1)) {
				Vec2i rPos = resource->getPos ();
				sc->deleteResource();
				// let the pathfinder know
				//FIXME: make Field friendly ??? resources in other fields ???
				//FIXME: UPDATE BASED ON ZONE... this effects multiple fields... doh!
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
		} 
		else {
			unit->finishCommand();
			unit->setCurrSkill(scStop);
		}
	}
}

//looks for a resource of type rt, if rt==NULL looks for any
//resource the unit can harvest
bool HarvestCommandType::searchForResource( Unit *unit, World *world ) const {
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
