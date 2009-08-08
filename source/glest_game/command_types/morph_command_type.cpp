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
// 	class MorphCommandType
// =====================================================

bool MorphCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);
	//morph skill
	try {
		string skillName= n->getChild("morph-skill")->getAttribute("value")->getRestrictedValue();
		morphSkillType= static_cast<const MorphSkillType*>(unitType->getSkillType(skillName, scMorph));
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	//morph unit
	try {
		string morphUnitName= n->getChild("morph-unit")->getAttribute("name")->getRestrictedValue();
		morphUnit= ft->getUnitType(morphUnitName);
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	//discount
	try { discount= n->getChild("discount")->getAttribute("value")->getIntValue(); }
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	return loadOk;
}

void MorphCommandType::update(Unit *unit) const {
	CommandType::cacheUnit ( unit );

	//if subfaction becomes invalid while updating this command, then cancel it.
	if(!verifySubfaction(unit, morphUnit)) {
		return;
	}

	if(unit->getCurrSkill()->getClass() != scMorph) {
		//if not morphing, check space
		bool gotSpace = false;
		Fields mfs = morphUnit->getFields ();
		Field mf = (Field)0;
		//TODO Cleanup, remove 'fields' unit's should only have one.
		while ( mf != FieldCount ) {
			if ( mfs.get ( mf )
			&&   map->areFreeCellsOrHasUnit ( unit->getPos(), morphUnit->getSize(), mf, unit) ) {
				gotSpace = true;
				break;
			}
			mf = (Field)(mf + 1);
		}

		if ( gotSpace ) {
			unit->setCurrSkill(morphSkillType);
			unit->getFaction()->checkAdvanceSubfaction(morphUnit, false);
			unit->setCurrField ( mf );
		} 
		else {
			if(unit->getFactionIndex() == world->getThisFactionIndex()) {
				game->getConsole()->addStdMessage("InvalidPosition");
			}
			unit->cancelCurrCommand();
		}
	} 
	else { // already started
		//Logger::getInstance().add ("Updating progress2....");
		unit->update2();
		if ( unit->getProgress2() > morphUnit->getProductionTime() ) {
			bool mapUpdate = unit->isMobile () != morphUnit->isMobile ();
			//finish the command
			if(unit->morph(this)) {
				unit->finishCommand();
				if ( mapUpdate ) {
					bool adding = !morphUnit->isMobile ();
					//FIXME: make Field friendly.. ZONES, NOT FIELDS...
					pathFinder->updateMapMetrics ( unit->getPos (), unit->getSize (), adding, FieldWalkable );
				}
				if(game->getGui()->isSelected(unit)) {
					game->getGui()->onSelectionChanged();
				}
				scriptManager->onUnitCreated ( unit );
				unit->getFaction()->checkAdvanceSubfaction(morphUnit, true);
				if(net->isNetworkServer()) {
					net->getServerInterface()->unitMorph(unit);
					net->getServerInterface()->updateFactions();
				}
			} 
			else {
				unit->cancelCurrCommand();
				if(unit->getFactionIndex() == world->getThisFactionIndex()) {
					game->getConsole()->addStdMessage("InvalidPosition");
				}
			}
			unit->setCurrSkill(scStop);
		}
	}
}

void MorphCommandType::getDesc(string &str, const Unit *unit) const{
	Lang &lang= Lang::getInstance();

	morphSkillType->getDesc(str, unit);

	//discount
	if(discount!=0){
		str+= lang.get("Discount")+": "+intToStr(discount)+"%\n";
	}

	str+= "\n"+getProduced()->getReqDesc();
}

string MorphCommandType::getReqDesc() const{
	return RequirableType::getReqDesc() + "\n" + getProduced()->getReqDesc();
}

const ProducibleType *MorphCommandType::getProduced() const{
	return morphUnit;
}

}}