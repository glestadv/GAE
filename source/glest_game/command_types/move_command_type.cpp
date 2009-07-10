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
// 	class MoveBaseCommandType
// =====================================================

void MoveBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	CommandType::load(n, dir, tt, ft);

	//move
   	string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
	moveSkillType= static_cast<const MoveSkillType*>(unitType->getSkillType(skillName, scMove));
}

// =====================================================
// 	class MoveCommandType
// =====================================================

void MoveCommandType::update(UnitUpdater *unitUpdater, Unit *unit) const
{
	Command *command= unit->getCurrCommand();
	Vec2i pos;
	if ( command->getUnit () )
   {
		pos = command->getUnit()->getCenteredPos();
		if(!command->getUnit()->isAlive()) 
      {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	} 
   else 
		pos = command->getPos();

	switch(unitUpdater->pathFinder->findPath(unit, pos)) 
   {
	   case PathFinder::tsOnTheWay:
		   unit->setCurrSkill ( this->getMoveSkillType() );
		   unit->face ( unit->getNextPos() );
		   break;

	   case PathFinder::tsBlocked:
		   if ( unit->getPath()->isBlocked() && !command->getUnit() )
			   unit->finishCommand ();
		   break;

	   default: // tsArrived
		   unit->finishCommand ();
	}
	// if we're doing an auto command, let's make sure we still want to do it
   unitUpdater->updateAutoCommand ( unit );
}

}}
