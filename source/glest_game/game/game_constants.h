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

#ifndef _GLEST_GAME_GAMECONSTANTS_H_
#define _GLEST_GAME_GAMECONSTANTS_H_

// The 'Globals'
#define theGame				(*Game::getInstance())
#define theWorld			(World::getInstance())
#define theMap				(*World::getInstance().getMap())
#define theCamera			(*Game::getInstance()->getGameCamera())
#define theGameSettings		(Game::getInstance()->getGameSettings())
#define theGui				(*Gui::getCurrentGui())
#define theConsole			(*Game::getInstance()->getConsole())
#define theConfig			(Config::getInstance())
#define theRoutePlanner		(*Search::RoutePlanner::getInstance())
#define theRenderer			(Renderer::getInstance())
#define theNetworkManager	(NetworkManager::getInstance())
#define theSoundRenderer	(SoundRenderer::getInstance())
#define theLogger			(Logger::getInstance())

#define LOG(x) Logger::getInstance().add(x)

#include "util.h"
using Shared::Util::EnumNames;

#ifdef GAME_CONSTANTS_DEF
#	define STRINGY_ENUM_NAMES(name, count, ...) EnumNames name##Names(#__VA_ARGS__, count, true, false, #name)
#else
#	define STRINGY_ENUM_NAMES(name, count, ...) extern EnumNames name##Names
#endif

// =====================================================
//	Enumerations
// =====================================================
	 /* doxygen enum template
	  * <ul><li><b>VALUE</b> description</li>
	  *		<li><b>VALUE</b> description</li>
	  *		<li><b>VALUE</b> description</li></ul>
	  */

namespace Glest{ namespace Game{

namespace Search {
	/** result set for path finding 
	  * <ul><li><b>ARRIVED</b> Arrived at destination (or as close as unit can get to target)</li>
	  *		<li><b>ONTHEWAY</b> On the way to destination</li>
	  *		<li><b>BLOCKED</b> path is blocked</li></ul>
	  */
	ENUMERATED_TYPE( TravelState, ARRIVED, ONTHEWAY, BLOCKED );

	/** result set for aStar() 
	  * <ul><li><b>FAILED</b> No path exists
	  *		<li><b>COMPLETE</b> comlete path found</li>
	  *		<li><b>PARTIAL</b> node limit reached, partial path returned</li>
	  *		<li><b>INPROGRESS</b> search ongoing (time limit reached)</li></ul>
	  */
	ENUMERATED_TYPE( AStarResult, FAILED, COMPLETE, PARTIAL, INPROGRESS );

	/** Specifies a 'space' to search 
	  * <ul><li><b>CELLMAP</b> search on cell map</li>
	  *		<li><b>TILEMAP</b> search on tile map</li></ul>
	  */
	ENUMERATED_TYPE( SearchSpace, CELLMAP, TILEMAP );

}
/** The control type of a 'faction' (aka, player)
  * <ul><li><b>CLOSED</b> Slot closed, no faction</li>
  *		<li><b>CPU</b> CPU player</li>
  *		<li><b>CPU_ULTRA</b> Cheating CPU player</li>
  *		<li><b>NETWORK</b> Network player</li>
  *		<li><b>HUMAN</b> Local Player</li></ul>
  */
ENUMERATED_TYPE( ControlType, CLOSED, CPU, CPU_ULTRA, NETWORK, HUMAN );

/** field of movement
  * <ul><li><b>WALKABLE</b> land traveller</li>
  *		<li><b>AIR</b> flying units</li>
  *		<li><b>ANY_WATER</b> travel on water only</li>
  *		<li><b>DEEP_WATER</b> travel in deep water only</li>
  *		<li><b>AMPHIBIOUS</b> land or water</li></ul>
  */
ENUMERATED_TYPE( Field,
				   LAND,
				   AIR,
				   ANY_WATER,
				   DEEP_WATER,
				   AMPHIBIOUS
			   );

/** surface type for cells
  * <ul><li><b>LAND</b> land (above sea level)</li>
  *		<li><b>FORDABLE</b> shallow (fordable) water</li>
  *		<li><b>DEEP_WATER</b> deep (non-fordable) water</li></ul>
  */
ENUMERATED_TYPE( SurfaceType,
				   LAND, 
				   FORDABLE, 
				   DEEP_WATER
			   );

/** zones of unit occupance
  * <ul><li><b>SURFACE_PROP</b> A surface prop, not used yet.</li>
  *		<li><b>SURFACE</b> the surface zone</li>
  *		<li><b>AIR</b> the air zone</li></ul>
  */
ENUMERATED_TYPE( Zone,
				   SURFACE_PROP,
				   LAND,
				   AIR
			   );

/** unit properties
  * <ul><li><b>BURNABLE</b> can catch fire.</li>
  *		<li><b>ROTATED_CLIMB</b> currently deprecated</li>
  *		<li><b>WALL</b> is a wall</li></ul>
  */
ENUMERATED_TYPE( Property,
					BURNABLE,
					ROTATED_CLIMB,
					WALL
			   );

/** effects flags
  * <ul><li><b>VALUE</b> desc.</li>
  *		<li><b>VALUE</b> desc.</li>
  *		<li><b>VALUE</b> desc.</li></ul>
  */
ENUMERATED_TYPE( EffectTypeFlag,
					ALLY,						// effects allies
					FOE,						// effects foes
					NO_NORMAL_UNITS,			// doesn't effects normal units
					BUILDINGS,					// effects buildings
					PETS_ONLY,					// only effects pets of the originator
					NON_LIVING,					//
					SCALE_SPLASH_STRENGTH,		// decrease strength when applied from splash
					ENDS_WHEN_SOURCE_DIES,		// ends when the unit causing the effect dies
					RECOURsE_ENDS_WITH_ROOT,	// ends when root effect ends (recourse effects only)
					PERMANENT,					// the effect has an infinite duration
					ALLOW_NEGATIVE_SPEED,		//
					TICK_IMMEDIATELY,			//

					//ai hints
					AI_DAMAGED,					// use on damaged units (benificials only)
					AI_RANGED,					// use on ranged attack units
					AI_MELEE,					// use on melee units
					AI_WORKER,					// use on worker units
					AI_BUILDING,				// use on buildings
					AI_HEAVY,					// perfer to use on heavy units
					AI_SCOUT,					// useful for scouting units
					AI_COMBAT,					// don't use outside of combat (benificials only)
					AI_SPARINGLY,				// use sparingly
					AI_LIBERALLY				// use liberally
			   );

/** attack skill preferences
  * <ul><li><b>VALUE</b> desc.</li>
  *		<li><b>VALUE</b> desc.</li>
  *		<li><b>VALUE</b> desc.</li></ul>
  */
ENUMERATED_TYPE( AttackSkillPreference,
					WHENEVER_POSSIBLE,
					AT_MAX_RANGE,
					ON_LARGE,
					ON_BUILDING,
					WHEN_DAMAGED
			   );

// =====================================================
//	class GameConstants
// =====================================================

class GameConstants{
public:
	static const int maxPlayers= 4;
	static const int serverPort= 61357;
//	static const int updateFps= 40;
	static const int cameraFps= 100;
	static const int networkFramePeriod= 10;
	static const int networkExtraLatency= 200;
};

}}//end namespace

#endif
