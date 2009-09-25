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
#include "renderer.h"
#include "sound_renderer.h"
#include "unit_type.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

// =====================================================
// 	class GuardCommandType
// =====================================================

bool GuardCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = AttackCommandType::load(n, dir, tt, ft);

	//distance
	try { maxDistance = n->getChild("max-distance")->getAttribute("value")->getIntValue(); }
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what () );
		loadOk = false;
	}
	return loadOk;
}

void GuardCommandType::update(Unit *unit) const {
	CommandType::cacheUnit ( unit );
	if ( unit->getCurrSkill()->getClass() == scStop 
	&&   unit->getLastCommandUpdate() < 250000 ) {
		return;
	}

	unit->resetLastCommandUpdated();
	Unit *target= command->getUnit();
	Vec2i pos;

	if ( target && target->isDead() ) {
		//if you suck ass as a body guard then you have to hang out where your client died.
		command->setUnit(NULL);
		command->setPos(target->getPos());
		target = NULL;
	}

	if(target) {
		pos = Map::getNearestPos(unit->getPos(), target, 1, this->getMaxDistance());
	}
	else {
		pos = Map::getNearestPos(unit->getPos(), command->getPos(), 1, this->getMaxDistance());
	}

	if( updateAttackGeneric () ) {
		unit->setCurrSkill(scStop);
	}
}


}}