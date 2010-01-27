// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "network_types.h"
#include "command.h"
#include "world.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

// =====================================================
//	class NetworkCommand // NETWORK: this isn't used
// =====================================================

NetworkCommand::NetworkCommand(Command *command) {
	this->networkCommandType= command->getArchetype();
	this->unitId= command->getCommandedUnit()->getId();
	this->commandTypeId= command->getType()->getId();
	this->positionX= command->getPos().x;
	this->positionY= command->getPos().y;

	if (command->getUnitType()) {
		this->unitTypeId= command->getUnitType()->getId();
	} else {
		this->unitTypeId= command->getCommandedUnit()->getType()->getId();
	}
	
	if (command->getUnit()) {
		this->targetId= command->getUnit()->getId();
	} else {
		this->targetId= -1;
	}
}

NetworkCommand::NetworkCommand(int networkCommandType, int unitId, int commandTypeId, const Vec2i &pos, int unitTypeId, int targetId){
	this->networkCommandType= networkCommandType;
	this->unitId= unitId;
	this->commandTypeId= commandTypeId;
	this->positionX= pos.x;
	this->positionY= pos.y;
	this->unitTypeId= unitTypeId;
	this->targetId= targetId;
}

Command *NetworkCommand::toCommand() const {
	// from Commander::buildCommand
	assert(networkCommandType==CommandArchetype::GIVE_COMMAND);
	
	World &world = World::getInstance();
	Unit* target= NULL;
	const CommandType* ct= NULL;
	Unit* unit= world.findUnitById(unitId);
	const UnitType* unitType= world.findUnitTypeById(unit->getFaction()->getType(), unitTypeId);
	
	//validate unit
	if(!unit){
		throw runtime_error("Can not find unit with id: " + intToStr(unitId) + ". Game out of synch.");
	}

	ct= unit->getType()->findCommandTypeById(commandTypeId);

	//validate command type
	if(!ct){
		throw runtime_error("Can not find command type with id: " + intToStr(commandTypeId) + " in unit: " + unit->getType()->getName() + ". Game out of synch.");
	}

	//get target, the target might be dead due to lag, cope with it
	if(targetId != Unit::invalidId){
		target= world.findUnitById(targetId);
	}

	//create command
	Command *command= NULL;
	if (target) {
		command= new Command(ct, CommandFlags(), target, unit);
	} else if(unitType){
		command= new Command(ct, CommandFlags(), Vec2i(positionX, positionY), unitType, unit);
	} /*else if(!target){
		command= new Command(ct, CommandFlags(), Vec2i(positionX, positionY));
	} else{
		command= new Command(ct, CommandFlags(), target);
	}*/

	return command;
}


}}//end namespace
