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

#ifndef _GLEST_GAME_COMMAND_H_
#define _GLEST_GAME_COMMAND_H_

#include <cstdlib>

#include "unit.h"
#include "vec.h"
#include "types.h"
#include "flags.h"
#include "prototypes_enums.h"

using Shared::Math::Vec2i;
using Shared::Platform::int16;
using Glest::ProtoTypes::CommandType;

namespace Glest { namespace Entities {

typedef Flags<CmdProps, CmdProps::COUNT, uint8> CmdFlags;

class TargetList {
private:
	ConstUnitVector m_units;

public:
	TargetList(const ConstUnitVector &units) : m_units(units) { }

	const Unit* getClosest(const Unit *source);
	const Unit* getClosest(const Unit *source, const string &tag);
	const Unit* getClosest(const Unit *source, const set<string> &tags);
};

typedef vector<TargetList> TargetLists;

// =====================================================
// 	class Command
//
///	A unit command
// =====================================================

class Command {
	friend class EntityFactory<Command>;

public:
	static const Vec2i invalidPos;
	static int lastId;

private:
	int m_id;
	CmdDirective m_archetype;
	const CommandType *m_type;
	CmdFlags m_flags;
	Vec2i m_pos;
	Vec2i m_pos2;			// for patrol command, the position traveling away from.
	UnitId m_unitRef;		// target unit, used to move and attack optinally
	UnitId m_unitRef2;	// for patrol command, the unit traveling away from.
	const ProducibleType *m_prodType;	// used for build, produce, upgrade, morph and generate
	CardinalDir m_facing;
	Unit *m_commandedUnit;

public:

	struct CreateParamsArch {
		CmdDirective archetype;
		CmdFlags flags;
		const Vec2i pos;
		Unit *commandedUnit;

		CreateParamsArch(CmdDirective archetype, CmdFlags flags, const Vec2i &pos = invalidPos, Unit *commandedUnit = NULL)
			: archetype(archetype), flags(flags), pos(pos), commandedUnit(commandedUnit) { }
	};
	
	struct CreateParamsPos { // create params for order with target position
		const CommandType *type;
		CmdFlags flags;
		Vec2i pos;
		Unit *commandedUnit;

		CreateParamsPos(const CommandType *type, CmdFlags flags, const Vec2i &pos = invalidPos, Unit *commandedUnit = NULL) 
			: type(type), flags(flags), pos(pos), commandedUnit(commandedUnit) { }
	};

	struct CreateParamsUnit { // create params for order with target unit
		const CommandType *type;
		CmdFlags flags;
		Unit *unit;
		Unit *commandedUnit;

		CreateParamsUnit(const CommandType *type, CmdFlags flags, Unit *unit, Unit *commandedUnit = NULL) 
			: type(type), flags(flags), unit(unit), commandedUnit(commandedUnit) { }
	};

	struct CreateParamsProd { // create params for order with producible
		const CommandType *type;
		CmdFlags flags;
		Vec2i pos;
		const ProducibleType *prodType;
		CardinalDir facing;
		Unit *commandedUnit;

		CreateParamsProd(const CommandType *type, CmdFlags flags, const Vec2i &pos, const ProducibleType *prodType, CardinalDir facing, Unit *commandedUnit = NULL) 
			: type(type), flags(flags), pos(pos), prodType(prodType), facing(facing), commandedUnit(commandedUnit) { }
	};

	struct CreateParamsLoad { // create params to de-serialise
		const XmlNode *node;
		const UnitType *ut;
		const FactionType *ft;

		CreateParamsLoad(const XmlNode *node, const UnitType *ut, const FactionType *ft) 
			: node(node), ut(ut), ft(ft) { }
	};

private:
	//constructor
	Command(CreateParamsArch params);
	Command(CreateParamsPos params);
	Command(CreateParamsUnit params);
	Command(CreateParamsProd params);
	Command(CreateParamsLoad params);

	~Command() {}
	void setId(int v) { m_id = v; }

public:
	MEMORY_CHECK_DECLARATIONS(Command)

	//get
	CmdDirective getArchetype() const               { return m_archetype;                                               }
	const CommandType *getType() const              { return m_type;                                                    }
	CmdFlags getFlags() const                       { return m_flags;                                                   }
	bool isQueue() const                            { return m_flags.get(CmdProps::QUEUE);                              }
	bool isAuto() const                             { return m_flags.get(CmdProps::AUTO);                               }
	bool isReserveResources() const                 { return !m_flags.get(CmdProps::DONT_RESERVE_RESOURCES);            }
	bool isMiscEnabled() const                      { return m_flags.get(CmdProps::MISC_ENABLE);                        }
	Vec2i getPos() const                            { return m_pos;                                                     }
	Vec2i getPos2() const                           { return m_pos2;                                                    }
	UnitId getUnitRef() const                       { return m_unitRef;                                                 }
	UnitId getUnitRef2() const                      { return m_unitRef2;                                                }
	Unit* getUnit() const;
	Unit* getUnit2() const;
	const ProducibleType* getProdType() const       { return m_prodType;                                                }
	CardinalDir getFacing() const                   { return m_facing;                                                  }
	Unit *getCommandedUnit() const                  { return m_commandedUnit;                                           }
	int getId() const                               { return m_id;                                                      }

	bool hasPos() const                             { return m_pos.x != -1;                                             }
	bool hasPos2() const                            { return m_pos2.x != -1;                                            }

	//set
	void setType(const CommandType *type)           { m_type= type;                                                     }
	void setFlags(CmdFlags flags)                   { m_flags = flags;                                                  }
	void setQueue(bool queue)                       { m_flags.set(CmdProps::QUEUE, queue);                              }
	void setAuto(bool _auto)                        { m_flags.set(CmdProps::AUTO, _auto);                               }
	void setReserveResources(bool reserveResources) { m_flags.set(CmdProps::DONT_RESERVE_RESOURCES, !reserveResources); }
	void setAutoCommandEnabled(bool enabled)        { m_flags.set(CmdProps::MISC_ENABLE, enabled);                      }
	void setPos(const Vec2i &pos)                   { m_pos = pos;                                                      }
	void setPos2(const Vec2i &pos2)                 { m_pos2 = pos2;                                                    }

	void setUnit(Unit *unit)                        { m_unitRef = unit ? unit->getId() : 0;                             }
	void setUnit2(Unit *unit2)                      { m_unitRef2 = unit2 ? unit2->getId() : 0;                          }
	void setProdType(const ProducibleType* pType)   { m_prodType = pType;                                               }
	void setCommandedUnit(Unit *commandedUnit)      { m_commandedUnit = commandedUnit;                                  }

	//misc
	void swap();
	void popPos()                                   { m_pos = m_pos2; m_pos2 = invalidPos;                              }
	void save(XmlNode *node) const;
};

inline ostream& operator<<(ostream &stream, const Command &command) {
	stream << "[Command id:" << command.getId() << "|";
	if (command.getArchetype() == CmdDirective::CANCEL_COMMAND) {
		stream << "Cancel command]";
	} else if (command.getArchetype() == CmdDirective::SET_AUTO_REPAIR) {
		stream << "set auto repair(" << 
			(command.getFlags().get(CmdProps::MISC_ENABLE) ? "true" : "false") << ")]";
	} else if (command.getArchetype() == CmdDirective::SET_AUTO_ATTACK) {
		stream << "set auto attack(" << 
			(command.getFlags().get(CmdProps::MISC_ENABLE) ? "true" : "false") << ")]";
	} else if (command.getArchetype() == CmdDirective::SET_AUTO_FLEE) {
		stream << "set auto flee(" << 
			(command.getFlags().get(CmdProps::MISC_ENABLE) ? "true" : "false") << ")]";
	} else {
		stream << command.getType()->getName() << "]";
	}
	return stream;
}

}}//end namespace

#endif
