// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/licence.txt
// ==============================================================

#ifndef _GAME_ENTITIES_CONSTANTS_
#define _GAME_ENTITIES_CONSTANTS_

#include "util.h"
using Shared::Util::EnumNames;

namespace Glest { namespace Entities {

/** upgrade states */
REGULAR_ENUM( UpgradeState,
	UPGRADING,
	UPGRADED
);

/** command properties */
REGULAR_ENUM( CommandProperties,
	QUEUE,
	AUTO,
	DONT_RESERVE_RESOURCES,
	AUTO_REPAIR_ENABLED
);

/** Command Archetypes */
REGULAR_ENUM( CommandArchetype,
	GIVE_COMMAND,
	CANCEL_COMMAND
//	SET_MEETING_POINT
//	SET_AUTO_REPAIR
);

}}

#endif
