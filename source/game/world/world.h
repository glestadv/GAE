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

#ifndef _GAME_WORLD_H_
#define _GAME_WORLD_H_

#include "vec.h"
#include "math_util.h"
#include "resource.h"
#include "tech_tree.h"
#include "tileset.h"
#include "console.h"
#include "map.h"
#include "minimap.h"
#include "logger.h"
#include "stats.h"
#include "time_flow.h"
#include "upgrade.h"
#include "water_effects.h"
#include "faction.h"
#include "unit_updater.h"
#include "random.h"
//#include "game_constants.h"
#include "pos_iterator.h"

using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Util::Random;
using Game::Util::PosCircularIteratorFactory;

namespace Game {

class Faction;
class Unit;
class Config;
class Game;
class GameSettings;

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class World {
public:
	typedef vector<Faction*> Factions;

public:
	static const int generationArea= 100;
	static const float airHeight;
	static const int indirectSightRange= 5;

private:
	Map map;
	Tileset tileset;
	TechTree techTree;
	TimeFlow timeFlow;
	Game &game;
	const GameSettings &gs;

	UnitUpdater unitUpdater;
    WaterEffects waterEffects;
	Minimap minimap;
    Stats stats;

	Factions factions;
//	UnitMap units;		commented out for now, but we need units mapped here and not in the factions

	Random random;

	int thisFactionIndex;
	int thisTeamIndex;
	int frameCount;
	int nextUnitId;

	//config
	bool fogOfWar;
	int fogOfWarSmoothingFrameSkip;
	bool fogOfWarSmoothing;

	static World *singleton;
	bool alive;
	
	Units newlydead;
	PosCircularIteratorFactory posIteratorFactory;

public:
	World(Game *game);
	~World();
	void end(); //to die before selection does

	//get
	const Factions &getFactions() const				{return factions;}
//	const UnitMap &getUnits() const					{return units;}

	int getMaxFactions() const						{return map.getMaxFactions();}
	int getThisFactionIndex() const					{return thisFactionIndex;}
	int getThisTeamIndex() const					{return thisTeamIndex;}
	const Faction *getThisFaction() const			{return factions[thisFactionIndex];}
	int getFactionCount() const						{return factions.size();}
	const Map *getMap() const 						{return &map;}
	const Tileset *getTileset() const 				{return &tileset;}
	const TechTree *getTechTree() const 			{return &techTree;}
	const TimeFlow *getTimeFlow() const				{return &timeFlow;}
	Tileset *getTileset() 							{return &tileset;}
	Map *getMap() 									{return &map;}
	const Faction *getFaction(int i) const			{return factions[i];}
	Faction *getFaction(int i) 						{return factions[i];}
	const Minimap *getMinimap() const				{return &minimap;}
//	const Stats &getStats() const					{return stats;}
	Stats &getStats() 								{return stats;}
	const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	int getNextUnitId()								{return nextUnitId++;}
	int getFrameCount() const						{return frameCount;}
	static World *getCurrWorld()					{return singleton;}
	bool isAlive() const							{return alive;}
	const PosCircularIteratorFactory &getPosIteratorFactory() {return posIteratorFactory;}

	//init & load
	void init(const XmlNode *worldNode = NULL);
	void loadTileset(Checksums &checksums);
	void loadTech(Checksums &checksums);
	void loadMap(Checksums &checksums);
	void save(XmlNode *node) const;

	//misc
	//void preUpdate();
	void update();
	void moveUnitCells(Unit *unit);
	Unit* findUnitById(int id);
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);
	void doKill(Unit *killer, Unit *killed);
	void assertConsistiency();
	void hackyCleanUp(Unit *unit);
	//bool toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const;
	//bool toRenderUnit(const Unit *unit) const;
	bool toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const {
		//a unit is rendered if it is in a visible cell or is attacking a unit in a visible cell
		return visibleQuad.isInside(unit->getCenteredPos()) && toRenderUnit(unit);
	}
	
	bool toRenderUnit(const Unit *unit) const {
		return map.getSurfaceCell(Map::toSurfCoords(unit->getCenteredPos()))->isVisible(thisTeamIndex)
			|| (unit->getCurrSkill()->getClass() == scAttack
			&& map.getSurfaceCell(Map::toSurfCoords(unit->getTargetPos()))->isVisible(thisTeamIndex));
	}

private:
	void initCells();
	void initSplattedTextures();
	void initFactionTypes();
	void initMinimap();
	void initUnits();
	void initMap();
	void initExplorationState();
	void initNetworkServer();

	//misc
	void updateClient();
	void updateEarthquakes(float seconds);
	void tick();
	void computeFow();
	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
	void loadSaved(const XmlNode *worldNode);
	void moveAndEvict(Unit *unit, vector<Unit*> &evicted, Vec2i *oldPos);
	void doClientUnitUpdate(XmlNode *n, bool minor, vector<Unit*> &evicted, float nextAdvanceFrames);
	bool isNetworkServer() {return NetworkManager::getInstance().isNetworkServer();}
	bool isNetworkClient() {return NetworkManager::getInstance().isNetworkClient();}
	bool isNetworkGame() {return NetworkManager::getInstance().isNetworkGame();}
	void doHackyCleanUp();
};

} // end namespace

#endif
