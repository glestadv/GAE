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
#include "unit_type.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

// =====================================================
// 	class BuildCommandType
// =====================================================

BuildCommandType::~BuildCommandType(){
	deleteValues(builtSounds.getSounds().begin(), builtSounds.getSounds().end());
	deleteValues(startSounds.getSounds().begin(), startSounds.getSounds().end());
}

void BuildCommandType::update(Unit *unit) const
{
	Command *command= unit->getCurrCommand();
	const UnitType *builtUnitType= command->getUnitType();
	Unit *builtUnit = NULL;
	Unit *target = unit->getTarget();
   Game *game = Game::getInstance ();
   World *world = game->getWorld ();
   Map *map = world->getMap ();
   PathFinder::PathFinder *pathFinder = PathFinder::PathFinder::getInstance ();
   NetworkManager &net = NetworkManager::getInstance();
	assert(command->getUnitType());

	if(unit->getCurrSkill()->getClass() != scBuild) {
		//if not building

		int buildingSize = builtUnitType->getSize();
		Vec2i waypoint;

		// find the nearest place for the builder
		if(map->getNearestAdjacentFreePos(waypoint, unit, command->getPos(), FieldWalkable, buildingSize)) {
			if(waypoint != unit->getTargetPos()) {
				unit->setTargetPos(waypoint);
				unit->getPath()->clear();
			}
		} else {

         game->getConsole()->addStdMessage("Blocked");
			unit->cancelCurrCommand();
			return;
		}

		switch (pathFinder->findPath(unit, waypoint)) {
		case PathFinder::tsOnTheWay:
			unit->setCurrSkill(this->getMoveSkillType());
			unit->face(unit->getNextPos());
			return;

		case PathFinder::tsBlocked:
			if(unit->getPath()->isBlocked()) {
				game->getConsole()->addStdMessage("Blocked");
				unit->cancelCurrCommand();
			}
			return;

		case PathFinder::tsArrived:
			if(unit->getPos() != waypoint) {
				game->getConsole()->addStdMessage("Blocked");
				unit->cancelCurrCommand();
				return;
			}
			// otherwise, we fall through
		}

		//if arrived destination
		if ( map->areFreeCells ( command->getPos(), buildingSize, FieldWalkable) 
      &&  ! ( command->getPos().x == 0 || command->getPos().y == 0 ) ) 
      {
         if(!Glest::Game::verifySubfaction(unit, builtUnitType)) {
				return;
			}


			// network client has to wait for the server to tell them to begin building.  If the
			// creates the building, we can have an id mismatch.
			if(net.isNetworkClient()) {
				unit->setCurrSkill(scWaitForServer);
				// FIXME: Might play start sound multiple times or never at all
			} else {
				// late resource allocation
				if(!command->isReserveResources()) {
					command->setReserveResources(true);
					if(unit->checkCommand(*command) != crSuccess) {
						if(unit->getFactionIndex() == world->getThisFactionIndex()){
							game->getConsole()->addStdMessage("BuildingNoRes");
						}
						unit->finishCommand();
						return;
					}
					unit->getFaction()->applyCosts(command->getUnitType());
				}

				builtUnit = new Unit(world->getNextUnitId(), command->getPos(), builtUnitType, unit->getFaction(), world->getMap());
				builtUnit->create();

				if(!builtUnitType->hasSkillClass(scBeBuilt)){
					throw runtime_error("Unit " + builtUnitType->getName() + " has no be_built skill");
				}

				builtUnit->setCurrSkill(scBeBuilt);
				unit->setCurrSkill(this->getBuildSkillType());
				unit->setTarget(builtUnit, true, true);
				map->prepareTerrain(builtUnit);
				command->setUnit(builtUnit);
				unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), false);
				if(net.isNetworkGame()) {
					net.getServerInterface()->newUnit(builtUnit);
					net.getServerInterface()->unitUpdate(unit);
					net.getServerInterface()->updateFactions();
				}
            //FIXME: make Field friendly
            if ( !builtUnit->isMobile() )
               pathFinder->updateMapMetrics ( builtUnit->getPos(), builtUnit->getSize(), true, FieldWalkable );
			}

			//play start sound
			if(unit->getFactionIndex()==world->getThisFactionIndex()){
            SoundRenderer::getInstance().playFx(
					this->getStartSound(),
					unit->getCurrVector(),
					game->getGameCamera()->getPos());
			}
		} else {
			// there are no free cells
			vector<Unit *>occupants;
			map->getOccupants(occupants, command->getPos(), buildingSize, ZoneSurface);

			// is construction already under way?
			Unit *builtUnit = occupants.size() == 1 ? occupants[0] : NULL;
			if(builtUnit && builtUnit->getType() == builtUnitType && !builtUnit->isBuilt()) {
				// if we pre-reserved the resources, we have to deallocate them now, although
				// this usually shouldn't happen.
				if(command->isReserveResources()) {
					unit->getFaction()->deApplyCosts(command->getUnitType());
					command->setReserveResources(false);
				}
				unit->setTarget(builtUnit, true, true);
				unit->setCurrSkill(this->getBuildSkillType());
				command->setUnit(builtUnit);

			} else {
				// is it not free because there are units in the way?
				if (occupants.size()) {
					// Can they get the fuck out of the way?
					vector<Unit *>::const_iterator i;
					for (i = occupants.begin();
							i != occupants.end() && (*i)->getType()->hasSkillClass(scMove); ++i) ;
					if (i == occupants.end()) {
						// they all have a move command, so we'll wait
						return;
					}
					//TODO: Check for idle units and tell them to get the fuck out of the way.
					//TODO: Possibly add a unit notification to let player know builder is waiting
				}

				// blocked by non-moving units, surface objects (trees, rocks, etc.) or build area
				// contains deeply submerged terain
				unit->cancelCurrCommand();
				unit->setCurrSkill(scStop);
				if (unit->getFactionIndex() == world->getThisFactionIndex()) {
               game->getConsole()->addStdMessage("BuildingNoPlace");
				}
			}
		}
	} else {
		//if building
		Unit *builtUnit = command->getUnit();

		if(builtUnit && builtUnit->getType() != builtUnitType) {
			unit->setCurrSkill(scStop);
		} else if(!builtUnit || builtUnit->isBuilt()) {
			unit->finishCommand();
			unit->setCurrSkill(scStop);
		} else if(builtUnit->repair()) {
			//building finished
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->checkAdvanceSubfaction(builtUnit->getType(), true);
			//FIXME: born() here or when building started ???
         //builtUnit->born();
         game->getScriptManager()->onUnitCreated ( builtUnit );

         if(unit->getFactionIndex()==world->getThisFactionIndex()) {
				SoundRenderer::getInstance().playFx(
					this->getBuiltSound(),
					unit->getCurrVector(),
               game->getGameCamera()->getPos());
			}
			if(net.isNetworkServer()) {
				net.getServerInterface()->unitUpdate(unit);
				net.getServerInterface()->unitUpdate(builtUnit);
				net.getServerInterface()->updateFactions();
			}
		}
	}
}

void BuildCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	MoveBaseCommandType::load(n, dir, tt, ft);

	//build
   	string skillName= n->getChild("build-skill")->getAttribute("value")->getRestrictedValue();
	buildSkillType= static_cast<const BuildSkillType*>(unitType->getSkillType(skillName, scBuild));

	//buildings built
	const XmlNode *buildingsNode= n->getChild("buildings");
	for(int i=0; i<buildingsNode->getChildCount(); ++i){
		const XmlNode *buildingNode= buildingsNode->getChild("building", i);
		string name= buildingNode->getAttribute("name")->getRestrictedValue();
		buildings.push_back(ft->getUnitType(name));
	}

	//start sound
	const XmlNode *startSoundNode= n->getChild("start-sound");
	if(startSoundNode->getAttribute("enabled")->getBoolValue()){
		startSounds.resize(startSoundNode->getChildCount());
		for(int i=0; i<startSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= startSoundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			startSounds[i]= sound;
		}
	}

	//built sound
	const XmlNode *builtSoundNode= n->getChild("built-sound");
	if(builtSoundNode->getAttribute("enabled")->getBoolValue()){
		builtSounds.resize(builtSoundNode->getChildCount());
		for(int i=0; i<builtSoundNode->getChildCount(); ++i){
			const XmlNode *soundFileNode= builtSoundNode->getChild("sound-file", i);
			string path= soundFileNode->getAttribute("path")->getRestrictedValue();
			StaticSound *sound= new StaticSound();
			sound->load(dir + "/" + path);
			builtSounds[i]= sound;
		}
	}
}


}}
