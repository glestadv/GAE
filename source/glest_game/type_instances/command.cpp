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

#ifdef USE_PCH
	#include "pch.h"
#endif

#include "command.h"

#include "command_type.h"
#include "leak_dumper.h"
#include "faction_type.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class Command::ResourceAllocation
// =====================================================
/*
// hmm, this doesn't serialize well :(
Command::ResourceAllocation::ResourceAllocation(const XmlNode *node) {
	refCount = node->getAttribute("refCount")->getIntValue();
	state = (ResourceAllocationState)node->getAttribute("state")->getIntValue();
}

void Command::ResourceAllocation::save(XmlNode *node) const {
	node->addAttribute("refCount", refCount);
	node->addAttribute("state", state);
}
*/
// =====================================================
// 	class Command
// =====================================================

Command::Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos){
	this->commandType = ct;
	this->flags = flags;
	this->pos = pos;
	this->pos2 = Vec2i(-1);
	this->waypoint = Vec2i(-1);
	this->unitRef = NULL;
	this->unitRef2 = NULL;
	unitType = NULL;
	//resourceAllocation = NULL;
}

Command::Command(const CommandType *ct, CommandFlags flags, Unit* unit) {
	this->commandType = ct;
	this->flags = flags;
	this->pos = Vec2i(-1);
	this->pos2 = Vec2i(-1);
	this->waypoint = Vec2i(-1);
	this->unitRef = unit;
	this->unitRef2 = NULL;
	unitType = NULL;
	//resourceAllocation = NULL;

	if(unit) {
		unit->resetHighlight();
		pos = unit->getCellPos();
	}
}

Command::Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos, const UnitType *unitType/*, ResourceAllocation *resourceAllocation*/) {
	this->commandType = ct;
	this->flags = flags;
	this->pos = pos;
	this->pos2 = Vec2i(-1);
	this->waypoint = Vec2i(-1);
	this->unitRef = NULL;
	this->unitRef2 = NULL;
	this->unitType = unitType;
	//this->resourceAllocation = resourceAllocation;
}

Command::Command(const XmlNode *node, const UnitType *ut, const FactionType *ft) :
		unitRef(node->getChild("unitRef")),
		unitRef2(node->getChild("unitRef2")) {
	commandType = ut->getCommandType(node->getChildStringValue("commandType"));
	flags.flags = node->getChildIntValue("flags");
	pos = node->getChildVec2iValue("pos");
	pos2 = node->getChildVec2iValue("pos2");
	waypoint = node->getChildVec2iValue("waypoint");

	string unitTypeName = node->getChildStringValue("unitType");
	unitType = unitTypeName == "none" ? NULL : ft->getUnitType(unitTypeName);
/*
	XmlNode *resourceAllocationNode = node->getChild("resourceAllocation", 0, false);
	if(resourceAllocationNode) {
		resourceAllocation = new ResourceAllocation(resourceAllocationNode);
	} else {
		resourceAllocationNode = NULL;
	}
*/
}

void Command::save(XmlNode *node) const {
	node->addChild("commandType", commandType->getName());
	node->addChild("flags", flags.flags);
	node->addChild("pos", pos);
	node->addChild("pos2", pos2);
	node->addChild("waypoint", waypoint);
	unitRef.save(node->addChild("unitRef"));
	unitRef2.save(node->addChild("unitRef2"));
	node->addChild("unitType", unitType ? unitType->getName() : "none");
/*
	if(resourceAllocation) {
		resourceAllocation->save(node->addChild("resourceAllocation"));
	}*/
}

// =============== misc ===============

void Command::swap() {
	UnitReference tmpUnitRef = unitRef;
	unitRef = unitRef2;
	unitRef2 = tmpUnitRef;

	Vec2i tmpPos = pos;
	pos = pos2;
	pos2 = tmpPos;
}

}}//end namespace
