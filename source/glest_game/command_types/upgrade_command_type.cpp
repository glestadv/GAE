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
#include "unit_updater.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "unit_type.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {


// =====================================================
// 	class UpgradeCommandType
// =====================================================

void UpgradeCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){

	CommandType::load(n, dir, tt, ft);

	//upgrade
   	string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
	upgradeSkillType= static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, scUpgrade));

	string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
	producedUpgrade= ft->getUpgradeType(producedUpgradeName);

}

void UpgradeCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const
{
	Command *command = unit->getCurrCommand();

	//if subfaction becomes invalid while updating this command, then cancel it.

	if(!verifySubfaction(unit, this->getProduced())) {
		unit->cancelCommand();
		unit->setCurrSkill(scStop);
		return;
	}

	if(unit->getCurrSkill()->getClass() != scUpgrade) {
		//if not producing
		unit->setCurrSkill(this->getUpgradeSkillType());
		unit->getFaction()->checkAdvanceSubfaction(this->getProducedUpgrade(), false);
	} else {
		//if producing
		if(unit->getProgress2() < this->getProduced()->getProductionTime()) {
			unit->update2();
		}

		if(unit->getProgress2() >= this->getProduced()->getProductionTime()) {
			if(unitUpdater->isNetworkClient()) {
				// clients will wait for the server to tell them that the upgrade is finished
				return;
			}
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(this->getProducedUpgrade());
			unit->getFaction()->checkAdvanceSubfaction(this->getProducedUpgrade(), true);
			if(unitUpdater->isNetworkServer()) {
				unitUpdater->getServerInterface()->unitUpdate(unit);
				unitUpdater->getServerInterface()->updateFactions();
			}
		}
	}
}

string UpgradeCommandType::getReqDesc() const{
	return RequirableType::getReqDesc()+"\n"+getProducedUpgrade()->getReqDesc();
}

const ProducibleType *UpgradeCommandType::getProduced() const{
	return producedUpgrade;
}

}}
