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


// ===============================
// 	class AttackCommandTypeBase
// ===============================

bool AttackCommandTypeBase::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft, const UnitType *unitType) {
	const AttackSkillType *ast;
	string skillName;
	const XmlNode *attackSkillNode = n->getChild("attack-skill", 0, false);
	bool loadOk = true;
	//single attack skill
	if(attackSkillNode) {
		try {
			skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
			ast = static_cast<const AttackSkillType*>(unitType->getSkillType(skillName, scAttack));
			attackSkillTypes.push_back(ast, AttackSkillPreferences());
		}
		catch ( runtime_error e ) {
			Logger::getErrorLog().addXmlError ( dir, e.what () );
			loadOk = false;
		}
	} 
	else { //multiple attack skills
		try {
			const XmlNode *flagsNode;
			const XmlNode *attackSkillsNode;

			attackSkillsNode = n->getChild("attack-skills", 0, false);
			if(!attackSkillsNode)
				throw runtime_error("Must specify either a single <attack-skill> node or an <attack-skills> node with nested <attack-skill>s.");
			int count = attackSkillsNode->getChildCount();

			for(int i = 0; i < count; ++i) {
				try {
					AttackSkillPreferences prefs;
					attackSkillNode = attackSkillsNode->getChild("attack-skill", i);
					skillName = attackSkillNode->getAttribute("value")->getRestrictedValue();
					ast = static_cast<const AttackSkillType*>(unitType->getSkillType(skillName, scAttack));
					flagsNode = attackSkillNode->getChild("flags", 0, false);
					if(flagsNode) {
						prefs.load(flagsNode, dir, tt, ft);
					}
					attackSkillTypes.push_back(ast, prefs);
				}
				catch ( runtime_error e ) {
					Logger::getErrorLog().addXmlError ( dir, e.what () );
					loadOk = false;
				}
			}
		}
		catch ( runtime_error e ) {
			Logger::getErrorLog().addXmlError ( dir, e.what () );
			loadOk = false;
		}
	}
	if ( loadOk ) attackSkillTypes.init();
	return loadOk;
}

/** Returns an attack skill for the given field if one exists. *//*
const AttackSkillType * AttackCommandTypeBase::getAttackSkillType(Field field) const {
	for(AttackSkillTypes::const_iterator i = attackSkillTypes.begin();
		   i != attackSkillTypes.end(); i++) {
		if(i->first->getField(field)) {
			return i->first;
		}
	}
	return NULL;
}
*/

//REFACTOR ( move to AttackCommandTypeBase )
Command *AttackCommandTypeBase::doAutoAttack(Unit *unit) {
	if(unit->getType()->hasCommandClass(ccAttack) || unit->getType()->hasCommandClass(ccAttackStopped)) {

		for(int i = 0; i < unit->getType()->getCommandTypeCount(); ++i) {
			const CommandType *ct = unit->getType()->getCommandType(i);

			if(!unit->getFaction()->isAvailable(ct)) {
				continue;
			}

			//look for an attack skill
			const AttackSkillType *ast = NULL;
			const AttackSkillTypes *asts = NULL;
			Unit *sighted = NULL;

			switch (ct->getClass()) {
			case ccAttack:
				asts = ((const AttackCommandType*)ct)->getAttackSkillTypes();
				break;

			case ccAttackStopped:
				asts = ((const AttackStoppedCommandType*)ct)->getAttackSkillTypes();
				break;

			default:
				break;
			}

			//use it to attack
			if(asts) 
         {
            if( CommandType::attackableOnSight(unit, &sighted, asts, NULL) ) 
            {
					Command *newCommand = new Command(ct, CommandFlags(cpAuto), sighted->getPos());
					newCommand->setPos2(unit->getPos());
					return newCommand;
				}
			}
		}
	}

	return NULL;
}

// =====================================================
// 	class AttackCommandType
// =====================================================

bool AttackCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool ok = MoveBaseCommandType::load(n, dir, tt, ft);
	return AttackCommandTypeBase::load(n, dir, tt, ft, unitType) && ok;
}

void AttackCommandType::update(Unit *unit) const {
	CommandType::cacheUnit ( unit );
	if ( updateAttackGeneric() ) {
		unit->finishCommand();
	}
}

/** Returns true when completed */
bool AttackCommandType::updateAttackGeneric () const {
	const AttackSkillType *ast = NULL;
	Unit* target = command->getUnit ();
	const Vec2i &targetPos = command->getPos ();

	if ( target && !attackSkillTypes.getZone (target->getCurrZone()) ) {
		unit->finishCommand();
	}
	//if found
	if ( attackableOnRange(unit, &target, &attackSkillTypes, &ast) ) {
		assert(ast);
		if(unit->getEp() >= ast->getEpCost()) {
			unit->setCurrSkill(ast);
			unit->setTarget(target, true, true);
		} else {
			unit->setCurrSkill(scStop);
		}
	} else {
		//compute target pos
		Vec2i pos;
		if ( attackableOnSight(unit, &target, &attackSkillTypes, NULL) )  {
			pos = target->getNearestOccupiedCell(unit->getPos());
			if (pos != unit->getTargetPos()) {
				unit->setTargetPos(pos);
				unit->getPath()->clear();
			}
		} else {
			// if no more targets and on auto command, then turn around
			if(command->isAuto() && command->hasPos2()) {
				if(Config::getInstance().getGsAutoReturnEnabled()) {
					command->popPos();
					pos = command->getPos();
					unit->getPath()->clear();
				} else {
					unit->finishCommand();
				}
			} else {
				pos = targetPos;
			}
		}

		//if unit arrives destPos order has ended
		switch( Search::PathFinder::getInstance()->findPath(unit, pos)) {
		case Search::tsOnTheWay:
			unit->setCurrSkill(this->getMoveSkillType());
			unit->face(unit->getNextPos());
			break;
		case Search::tsBlocked:
			if (unit->getPath()->isBlocked()) {
				return true;
			}
			break;
		default:
			return true;
		}
	}

	return false;
}

// =====================================================
// 	class AttackStoppedCommandType
// =====================================================

bool AttackStoppedCommandType::load(const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft){
	bool ok = StopBaseCommandType::load(n, dir, tt, ft);
	return AttackCommandTypeBase::load(n, dir, tt, ft, unitType) && ok;
}

void AttackStoppedCommandType::update(Unit *unit) const
{
	CommandType::cacheUnit ( unit );
	if ( unit->getLastCommandUpdate () < 250000 ) return;

	unit->resetLastCommandUpdated ();

	Command *command = unit->getCurrCommand();
	Unit *enemy = NULL;
	const AttackSkillType *ast = NULL;

	if ( attackableOnRange(unit, &enemy, this->getAttackSkillTypes(), &ast)) {
		assert(ast);
		unit->setCurrSkill(ast);
		unit->setTarget(enemy, true, true);
	} 
	else {
		unit->setCurrSkill(this->getStopSkillType());
	}
}


}}