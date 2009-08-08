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
// 	class PatrolCommandType
// =====================================================

void PatrolCommandType::update(Unit *unit) const {
	CommandType::cacheUnit ( unit );

	Unit *target = command->getUnit();
	Unit *target2 = command->getUnit2();
	Vec2i pos;

	if(target) {
		pos = target->getCenteredPos();
		if(target->isDead()) {
			command->setUnit(NULL);
			command->setPos(pos);
		}
	} 
	else {
		pos = command->getPos();
	}

	if(target) {
		pos = Map::getNearestPos(unit->getPos(), pos, 1, this->getMaxDistance());
	}

	if(target2 && target2->isDead()) {
		command->setUnit2(NULL);
		command->setPos2(target2->getCenteredPos());
	}

	// If destination reached or blocked, turn around on next frame.
	if ( updateAttackGeneric () ) {
		command->swap();
		/*
		// can't use minor update here because the command has changed and that wont give the new
		// command
		if(isNetworkGame() && isServer()) {
		getServerInterface()->minorUnitUpdate(unit);
		}*/
	}
}


}}