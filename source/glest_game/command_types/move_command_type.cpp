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
// 	class MoveBaseCommandType
// =====================================================

bool MoveBaseCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool loadOk = CommandType::load(n, dir, tt, ft);

	//move
	try { 
		string skillName= n->getChild("move-skill")->getAttribute("value")->getRestrictedValue();
		const SkillType *st = unitType->getSkillType(skillName, scMove);
		moveSkillType= static_cast<const MoveSkillType*>(st);
	}
	catch ( runtime_error e ) {
		Logger::getErrorLog().addXmlError ( dir, e.what() );
		loadOk = false;
	}
	return loadOk;}

// =====================================================
// 	class MoveCommandType
// =====================================================

void MoveCommandType::update(Unit *unit) const
{
	CommandType::cacheUnit ( unit );
	Vec2i pos;
	if ( command->getUnit () ) {
		pos = command->getUnit()->getCenteredPos();
		if(!command->getUnit()->isAlive())  {
			command->setPos(pos);
			command->setUnit(NULL);
		}
	} 
	else 
		pos = command->getPos();

	switch(pathFinder->findPath(unit, pos)) {
		case Search::tsOnTheWay:
			unit->setCurrSkill ( this->getMoveSkillType() );
			unit->face ( unit->getNextPos() );
			break;

		case Search::tsBlocked:
			if ( unit->getPath()->isBlocked() && !command->getUnit() ) {
				unit->finishCommand ();
			}
			break;

		default: // tsArrived
			unit->finishCommand ();
	}
	// if we're doing an auto command, let's make sure we still want to do it
	updateAutoCommand ( unit );
}

//REFACTOR ( move to MoveCommandType ?? ) 
Command *MoveCommandType::doAutoFlee(Unit *unit) {
	Map *map = Game::getInstance ()->getWorld ()->getMap ();
	Unit *sighted = NULL;
	if( unit->getType()->hasCommandClass(ccMove) && attackerOnSight(unit, &sighted))  {
		//if there is a friendly military unit that we can heal/repair and is
		//rougly between us, then be brave
		if(unit->getType()->hasCommandClass(ccRepair)) {
			Vec2f myCenter = unit->getFloatCenteredPos();
			Vec2f signtedCenter = sighted->getFloatCenteredPos();
			Vec2f fcenter = (myCenter + signtedCenter) / 2.f;
			Unit *myHero = NULL;

			// calculate the real distance to hostile by subtracting half of the size of each.
			float actualDistance = myCenter.dist(signtedCenter)
				- (unit->getType()->getSize() + sighted->getType()->getSize()) / 2.f;

			// allow friendly combat unit to be within a radius of 65% of the actual distance to
			// hostile, starting from the half way intersection of the repairer and hostile.
			int searchRadius = (int)roundf(actualDistance 
				* RepairCommandType::repairerToFriendlySearchRadius / 2.f);
			Vec2i center((int)roundf(fcenter.x), (int)roundf(fcenter.y));

			//try all of our repair commands
			for (int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
				const CommandType *ct= unit->getType()->getCommandType(i);
				if(ct->getClass() != ccRepair) {
					continue;
				}
				const RepairCommandType *rct = (const RepairCommandType*)ct;
				const RepairSkillType *rst = rct->getRepairSkillType();

				if(rct->repairableOnRange( center, 1, &myHero, searchRadius, false, true, false)) {
					return NULL;
				}
			}
		}
		Vec2i escapePos = unit->getPos() * 2 - sighted->getPos();
		return new Command(unit->getType()->getFirstCtOfClass(ccMove), CommandFlags(cpAuto), escapePos);
	}
	return NULL;
}

}}
