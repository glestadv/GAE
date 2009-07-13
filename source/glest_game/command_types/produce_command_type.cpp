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
// 	class ProduceCommandType
// =====================================================

//varios
void ProduceCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	CommandType::load(n, dir, tt, ft);

	//produce
   	string skillName= n->getChild("produce-skill")->getAttribute("value")->getRestrictedValue();
	produceSkillType= static_cast<const ProduceSkillType*>(unitType->getSkillType(skillName, scProduce));

	string producedUnitName= n->getChild("produced-unit")->getAttribute("name")->getRestrictedValue();
	producedUnit= ft->getUnitType(producedUnitName);
}

void ProduceCommandType::update(Unit *unit) const
{
	Command *command = unit->getCurrCommand();
	Unit *produced;
   Game *game = Game::getInstance ();
   World *world = game->getWorld ();
   NetworkManager &net = NetworkManager::getInstance();

	if(unit->getCurrSkill()->getClass() != scProduce) {
		//if not producing
		if(!verifySubfaction(unit, this->getProducedUnit())) {
			return;
		}

		unit->setCurrSkill(this->getProduceSkillType());
		unit->getFaction()->checkAdvanceSubfaction(this->getProducedUnit(), false);
	} else {
		unit->update2();

		if(unit->getProgress2() > getProduced()->getProductionTime()) {
			if(net.isNetworkClient()) {
				// client predict, presume the server will send us the unit soon.
				unit->finishCommand();
				unit->setCurrSkill(scStop);
				return;
			}

			produced = new Unit(world->getNextUnitId(), Vec2i(0), this->getProducedUnit(), unit->getFaction(), world->getMap());

			//if no longer a valid subfaction, let them have the unit, but make
			//sure it doesn't advance the subfaction

			if(verifySubfaction(unit, this->getProducedUnit())) {
				unit->getFaction()->checkAdvanceSubfaction(this->getProducedUnit(), true);
			}

			//place unit creates the unit
			if(!world->placeUnit(unit->getCenteredPos(), 10, produced)) {
				unit->cancelCurrCommand();
				delete produced;
			} else {
				produced->create();
				produced->born();
            game->getScriptManager()->onUnitCreated ( produced );
				world->getStats().produce(unit->getFactionIndex());
				const CommandType *ct = produced->computeCommandType(unit->getMeetingPos());

				if(ct) {
					produced->giveCommand(new Command(ct, CommandFlags(), unit->getMeetingPos()));
				}

				if( getProduceSkillType()->isPet() ) 
            {
					unit->addPet(produced);
					produced->setMaster(unit);
				}

				unit->finishCommand();

				if(net.isNetworkServer()) {
					net.getServerInterface()->newUnit(produced);
				}
			}
			unit->setCurrSkill(scStop);
		}
	}
}

void ProduceCommandType::getDesc(string &str, const Unit *unit) const {
	produceSkillType->getDesc(str, unit);
	str+= "\n" + getProducedUnit()->getReqDesc();
}

string ProduceCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUnit()->getReqDesc();
}

const ProducibleType *ProduceCommandType::getProduced() const{
	return producedUnit;
}


}}