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

#include "command.h"

#include "command_type.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class Command
// =====================================================

Command::Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos){
    this->commandType= ct;
	this->flags = flags;
    this->pos= pos;
    this->pos2= Vec2i(0);
    this->unitRef= NULL;
    this->unitRef2= NULL;
	unitType= NULL;
}

Command::Command(const CommandType *ct, CommandFlags flags, Unit* unit){
    this->commandType= ct;
	this->flags = flags;
    this->pos= Vec2i(0);
    this->pos2= Vec2i(0);
    this->unitRef= unit;
    this->unitRef2= NULL;
	unitType= NULL;
	if(unit){
		unit->resetHighlight();
		pos= unit->getCellPos();
	}
}

Command::Command(const CommandType *ct, CommandFlags flags, const Vec2i &pos, const UnitType *unitType){
    this->commandType= ct;
	this->flags = flags;
    this->pos= pos;
    this->pos2= Vec2i(0);
    this->unitRef= NULL;
    this->unitRef2= NULL;
	this->unitType= unitType;
}

// =============== set ===============


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
