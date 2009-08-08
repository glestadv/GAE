// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
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
// 	class DummyCommandType
// =====================================================

void DummyCommandType::load (const XmlNode *n, const string &dir, const TechTree *tt, const FactionType *ft)
{
	CommandType::load(n, dir, tt, ft);
   //skill...
	string skillName= n->getChild("dummy-skill")->getAttribute("value")->getRestrictedValue();
	dummySkillType= static_cast<const DummySkillType*>(unitType->getSkillType(skillName, scDummy));
}

void DummyCommandType::getDesc(string &str, const Unit *unit) const
{
	dummySkillType->getDesc(str, unit);
}

void DummyCommandType::update(Unit *unit) const
{
   Command *command= unit->getCurrCommand();

   if ( unit->getCurrSkill ()->getClass () != scDummy )
   {
      // start... apply costs
      if ( unit->getFaction ()->checkCosts ( unit->getCurrSkill () ) )
      {
         unit->setCurrSkill ( this->getDummySkillType () );
         unit->getFaction ()->applyCosts ( unit->getCurrSkill () );
      }
      else
         unit->cancelCurrCommand ();
   }
   else 
   {
      const DummySkillType *dst = static_cast<const DummySkillType*>(unit->getCurrSkill ());
      unit->update2 ();
      if ( unit->getProgress2() >= dst->getTime() )
      {
         // finished... apply static production
         unit->getFaction ()->applyStaticProduction ( unit->getCurrSkill () );
         unit->finishCommand ();
      }
   }
}

string DummyCommandType::getReqDesc() const{
	return RequirableType::getReqDesc();
}

}}