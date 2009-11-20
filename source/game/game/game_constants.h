// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008-2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_GAMECONSTANTS_H_
#define _GAME_GAMECONSTANTS_H_

//TODO
// Rationialise singletons, make return all references if plausible,
// if not, make them ALL return pointers
//
#define theProgram		(Program::getInstance())
#define theGuiProgram	(GuiProgram::getInstance())
#define theWorld		(World::getInstance())
#define theGame			(*Game::getInstance())
#define theCamera		(theGame.getGameCamera())
#define theGameSettings (theGame.getGameSettings())
#define theConsole		(theProgram.getConsole())
#define theConfig		(theProgram.getConfig())
#define theLang			(theProgram.getLang())
#define theRenderer		(theGuiProgram.getRenderer())
#define theSoundRenderer (theGuiProgram.getSoundRenderer())
#define theCoreData		(theGuiProgram.getCoreData())
#define theMetrics		(theGuiProgram.getMetrics())


#include "util.h"

using Shared::Util::EnumNames;

#ifdef GAME_CONSTANTS_DEF
#	define STRINGY_ENUM_NAMES(name, count, ...) EnumNames name(#__VA_ARGS__, count, true, false)
#else
#	define STRINGY_ENUM_NAMES(name, count, ...) extern EnumNames name
#endif

namespace Game { namespace Net {

/**
 * Enumeration to manage the various states of a network interface.  This value is used
 * for both local and remote interfaces.  The enum value can be converted into a string
 * containing it's name by calling stateEnumNames.getName(value);
 */
STRINGY_ENUM(State, STATE_COUNT,
	STATE_UNCONNECTED,	/**< not yet connected */
	STATE_LISTENING,	/**< not yet connected, but listening for connections */
	STATE_CONNECTED,	/**< established a connection and sent (or expecting) handshake */
	STATE_NEGOTIATED,	/**< compatible protocols have been negotiated (via handshake) */
	STATE_INITIALIZED,	/**< has received game settings (still in lobby) */
	STATE_LAUNCH_READY,	/**< client is ready to launch game (when given the O.K.) */
	STATE_LAUNCHING,	/**< launching game */
	STATE_READY,		/**< ready to begin play */
	STATE_PLAY,			/**< game started (normal play state) */
	STATE_PAUSED,		/**< game paused */
	STATE_QUIT,			/**< quit game requested/initiated */
	STATE_END			/**< game terminated */
);

STRINGY_ENUM(NetworkMessageType, NMT_COUNT,
	NMT_INVALID,
	NMT_HANDSHAKE,
	NMT_PLAYER_INFO,
	NMT_GAME_INFO,
	NMT_PING,
	NMT_STATUS,
	NMT_TEXT,
	NMT_FILE_HEADER,
	NMT_FILE_FRAGMENT,
	NMT_READY,
	NMT_COMMAND_LIST,
	NMT_UPDATE,
	NMT_UPDATE_REQUEST
);

/**
 * Specifies that a game parameter is being changed or a request has been made for it to be
 * changed, the state of which is specified by ParamChange.
 */
STRINGY_ENUM(GameParam, GAME_PARAM_COUNT,
	GAME_PARAM_NONE,		/**< No game paramter changes are pending, requested or in progress. */
	GAME_PARAM_PAUSED,		/**< A pause is either pending, requested or in progress. */
	GAME_PARAM_UNPAUSED,	/**< An unpause is either pending, requested or in progress. */
	GAME_PARAM_SPEED		/**< A game speed change is either pending, requested or in progress.
							 *   The target speed is indicated with a GameSpeed value. */
);

/**
 * The state of a game paramiter change, valid only if GameParam is other than GAME_PARAM_NONE.
 */
STRINGY_ENUM(ParamChange, PARAM_CHANGE_COUNT,
								/**< T - target frame required or not */
	PARAM_CHANGE_NONE,			/**< n - no pending state changes */
	PARAM_CHANGE_REQUESTED,		/**< y - state change requested */
	PARAM_CHANGE_REQ_ACK,		/**< y - pause request acknowledged */
	PARAM_CHANGE_REQ_ACCEPTED,	/**< y - pause request accepted */
	PARAM_CHANGE_REQ_DENIED,	/**< y - pause request denied */
	PARAM_CHANGE_COMMITED		/**< n - state change committed (paused, unpaused, speed changed, etc.) */
);

} // end namespace Game::Net

STRINGY_ENUM(PlayerType, PLAYER_TYPE_COUNT,
    PLAYER_TYPE_HUMAN,		/**< Represents a human player */
	PLAYER_TYPE_AI			/**< Represents a computer player */
);

STRINGY_ENUM(GameSpeed, GAME_SPEED_COUNT,
	GAME_SPEED_SLOWEST,
	GAME_SPEED_VERY_SLOW,
	GAME_SPEED_SLOW,
	GAME_SPEED_NORMAL,
	GAME_SPEED_FAST,
	GAME_SPEED_VERY_FAST,
	GAME_SPEED_FASTEST
);
extern const char* enumGameSpeedDesc[GAME_SPEED_COUNT];

STRINGY_ENUM(ControlType, CT_COUNT,
    CT_CLOSED,		/** */
	CT_CPU,			/** */
	CT_CPU_ULTRA,	/** */
	CT_NETWORK,		/** */
	CT_HUMAN		/** */
);
extern const char* enumControlTypeDesc[CT_COUNT];

STRINGY_ENUM(NetworkRole, NR_COUNT,
	NR_IDLE,		/** */
	NR_SERVER,		/** */
	NR_CLIENT,		/** */
	NR_PEER,		/** */
	NR_OBSERVER		/** */
);

// =====================================================
//	class GameConstants
// =====================================================

class GameConstants {
public:
	static const int maxPlayers = 4;
	static const int maxFactions = 4;
	static const int cameraFps = 100;
	static const int networkExtraLatency = 200;
};

} // end namespace Game

#endif
