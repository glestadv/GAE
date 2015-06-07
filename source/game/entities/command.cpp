// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "command.h"

#include "world.h"
#include "command_type.h"
#include "faction_type.h"
#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"

namespace Glest { namespace Entities {

// =====================================================
// 	class Command
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Command)

const Vec2i Command::invalidPos = Vec2i(-1);

Command::Command(CreateParamsArch params)//CmdDirective archetype, CmdFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: m_id(-1)
		, m_archetype(params.archetype)
		, m_type(NULL)
		, m_flags(params.flags)
		, m_pos(params.pos)
		, m_pos2(-1)
		, m_unitRef(-1)
		, m_unitRef2(-1)
		, m_prodType(NULL)
		, m_commandedUnit(params.commandedUnit) {
}

Command::Command(CreateParamsPos params)//const CommandType *type, CmdFlags flags, const Vec2i &pos, Unit *commandedUnit)
		: m_id(-1)
		, m_archetype(CmdDirective::GIVE_COMMAND)
		, m_type(params.type)
		, m_flags(params.flags)
		, m_pos(params.pos)
		, m_pos2(-1)
		, m_unitRef(-1)
		, m_unitRef2(-1)
		, m_prodType(0)
		, m_commandedUnit(params.commandedUnit) {
}

Command::Command(CreateParamsUnit params)//const CommandType *type, CmdFlags flags, Unit* unit, Unit *commandedUnit)
		: m_id(-1)
		, m_archetype(CmdDirective::GIVE_COMMAND)
		, m_type(params.type)
		, m_flags(params.flags)
		, m_pos(-1)
		, m_pos2(-1)
		, m_prodType(0)
		, m_commandedUnit(params.commandedUnit) {
	m_unitRef = params.unit ? params.unit->getId() : -1;
	m_unitRef2 = -1;
	
	if (params.unit) {
		m_pos = params.unit->getCenteredPos();
	}
	if (params.unit && !isAuto() && m_commandedUnit && m_commandedUnit->getFaction()->isThisFaction()) {
		params.unit->resetHighlight();
	}
}


Command::Command(CreateParamsProd params)//const CommandType *type, CmdFlags flags, const Vec2i &pos, 
				 //const ProducibleType *prodType, CardinalDir facing, Unit *commandedUnit)
		: m_id(-1)
		, m_archetype(CmdDirective::GIVE_COMMAND)
		, m_type(params.type)
		, m_flags(params.flags)
		, m_pos(params.pos)
		, m_pos2(-1)
		, m_unitRef(-1)
		, m_unitRef2(-1)
		, m_prodType(params.prodType)
		, m_facing(params.facing)
		, m_commandedUnit(params.commandedUnit) {
}

Command::Command(CreateParamsLoad params) {// const XmlNode *node, const UnitType *ut, const FactionType *ft) {
	const XmlNode *node = params.node;

	m_id = node->getChildIntValue("id");
	m_unitRef = node->getOptionalIntValue("unitRef", -1);
	m_unitRef2 = node->getOptionalIntValue("unitRef2", -1);
	m_archetype = CmdDirective(node->getChildIntValue("archetype"));
	m_type = params.ut->getCommandType(node->getChildStringValue("type"));
	m_flags.flags = node->getChildIntValue("flags");
	m_pos = node->getChildVec2iValue("pos");
	m_pos2 = node->getChildVec2iValue("pos2");
	int prodTypeId = node->getChildIntValue("prodType");
	if (prodTypeId == -1) {
		m_prodType = 0;
	} else {
		m_prodType = g_prototypeFactory.getProdType(prodTypeId);
	}
	if (node->getOptionalChild("facing") ) {
		m_facing = enum_cast<CardinalDir>(node->getChildIntValue("facing"));
	}
}

Unit* Command::getUnit() const {
	return g_world.getUnit(m_unitRef);
}

Unit* Command::getUnit2() const {
	return g_world.getUnit(m_unitRef2);
}

void Command::save(XmlNode *node) const {
	node->addChild("id", m_id);
	node->addChild("archetype", m_archetype);
	node->addChild("type", m_type->getName());
	node->addChild("flags", (int)m_flags.flags);
	node->addChild("pos", m_pos);
	node->addChild("pos2", m_pos2);
	node->addChild("unitRef", m_unitRef);
	node->addChild("unitRef2", m_unitRef2);
	node->addChild("prodType", m_prodType ? m_prodType->getId() : -1);
	node->addChild("facing", int(m_facing));
}

// =============== misc ===============

void Command::swap() {
	std::swap(m_unitRef, m_unitRef2);
	std::swap(m_pos, m_pos2);
}

}}//end namespace
