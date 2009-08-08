// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

bool UpgradeCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//upgrade
   try {
   	string skillName= n->getChild("upgrade-skill")->getAttribute("value")->getRestrictedValue();
	   upgradeSkillType= static_cast<const UpgradeSkillType*>(unitType->getSkillType(skillName, scUpgrade));
   }
   catch ( runtime_error e ) {
      Logger::getErrorLog().addXmlError ( dir, e.what () );
      loadOk = false;
   }
   try {
	   string producedUpgradeName= n->getChild("produced-upgrade")->getAttribute("name")->getRestrictedValue();
	   producedUpgrade= ft->getUpgradeType(producedUpgradeName);
   }
   catch ( runtime_error e ) {
      Logger::getErrorLog().addXmlError ( dir, e.what () );
      loadOk = false;
   }
   return loadOk;
}

void UpgradeCommandType::update(Unit *unit) const {
	CommandType::cacheUnit ( unit );
	//if subfaction becomes invalid while updating this command, then cancel it.
	if(!verifySubfaction(unit, producedUpgrade)) {
		unit->cancelCommand();
		unit->setCurrSkill(scStop);
		return;
	}

	if(unit->getCurrSkill()->getClass() != scUpgrade) {
		//if not producing
		unit->setCurrSkill(upgradeSkillType);
		unit->getFaction()->checkAdvanceSubfaction(producedUpgrade, false);
	} 
	else {
		//if producing
		if(unit->getProgress2() < producedUpgrade->getProductionTime()) {
			unit->update2();
		}

		if(unit->getProgress2() >= producedUpgrade->getProductionTime()) {
			if(net->isNetworkClient()) {
				// clients will wait for the server to tell them that the upgrade is finished
				return;
			}
			unit->finishCommand();
			unit->setCurrSkill(scStop);
			unit->getFaction()->finishUpgrade(producedUpgrade);
			unit->getFaction()->checkAdvanceSubfaction(producedUpgrade, true);
			if(net->isNetworkServer()) {
				net->getServerInterface()->unitUpdate(unit);
				net->getServerInterface()->updateFactions();
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
