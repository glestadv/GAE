// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SCRIPT_MANAGER_H_
#define _GLEST_GAME_SCRIPT_MANAGER_H_

#include <string>
#include <queue>

#include "lua_script.h"
#include "vec.h"
#include "timer.h"

#include "components.h"
#include "game_constants.h"

using std::string;
using std::queue;
using Shared::Graphics::Vec2i;
using Shared::Lua::LuaScript;
using Shared::Lua::LuaHandle;
using Shared::Platform::Chrono;

namespace Glest{ namespace Game{

class World;
class Unit;
class GameCamera;
class GameSettings;
class Game;


// =====================================================
//	class ScriptManagerMessage
// =====================================================

class ScriptTimer {
public:
	ScriptTimer ( const string &name, bool real, int interval, bool periodic) 
			: name (name)
			, real (real)
			, periodic  (periodic)
			, interval (interval)
			, active (true) {
		reset();
	}

	bool ready();
	void reset();

	string getName() { return name; }
	bool isPeriodic() { return periodic; }
	bool isAlive () { return active; }
	void kill () { active = false; }

private:
	string name;
	bool real;
	bool periodic;
	bool active;
	int64 targetTime, interval;
};

// =====================================================
//	class ScriptManagerMessage
// =====================================================

class ScriptManagerMessage{
private:
	string text;
	string header;

public:
	ScriptManagerMessage(string text, string header)	{this->text= text, this->header= header;}
	const string &getText() const	{return text;}
	const string &getHeader() const	{return header;}
};

class PlayerModifiers{
public:
	PlayerModifiers();

	void disableAi()			{aiEnabled= false;}
	void setAsWinner()			{winner= true;}

	bool getWinner() const		{return winner;}
	bool getAiEnabled() const	{return aiEnabled;}

private:
	bool winner;
	bool aiEnabled;
};

// =====================================================
//	class ScriptManager
// =====================================================

class ScriptManager{

	friend class World; // Lua Debugging, using setDisplayTest()

private:
	typedef queue<ScriptManagerMessage> MessageQueue;

private:

	//lua
	string code;
	LuaScript luaScript;

	//world
	World *world;
	GameCamera *gameCamera;
	GameSettings *gameSettings;
	Game *game;

	//misc
	MessageQueue messageQueue;
	GraphicMessageBox messageBox;
	string displayText;
	
	//last created unit
	string lastCreatedUnitName;
	int lastCreatedUnitId;

	//last dead unit
	string lastDeadUnitName;
	int lastDeadUnitId;

	// end game state
	bool gameOver;
	PlayerModifiers playerModifiers[GameConstants::maxPlayers];

	vector<ScriptTimer> timers;

private:
	static ScriptManager* thisScriptManager;

private:
	static const int messageWrapCount;
	static const int displayTextWrapCount;

public:
	void init(Game *game, World* world, GameCamera *gameCamera, GameSettings *gameSettings);

	//message box functions
	bool getMessageBoxEnabled() const									{return !messageQueue.empty();}
	GraphicMessageBox* getMessageBox()									{return &messageBox;}
	string getDisplayText() const										{return displayText;}
	bool getGameOver() const											{return gameOver;}
	const PlayerModifiers *getPlayerModifiers(int factionIndex) const	{return &playerModifiers[factionIndex];}	

	//events
	void onMessageBoxOk();
	void onResourceHarvested();
	void onUnitCreated(const Unit* unit);
	void onUnitDied(const Unit* unit);
	void onTimer();

private:

	string wrapString(const string &str, int wrapCount);

	vector<ScriptTimer> newTimerQueue;

	//wrappers, commands
	//NEW
	void setTimer ( const string &name, const string &type, int interval, bool periodic );
	void stopTimer(const string &name);
	//NEW END
	void showMessage(const string &text, const string &header);
	void clearDisplayText();
	void setDisplayText(const string &text);
	void setCameraPosition(const Vec2i &pos);
	void createUnit(const string &unitName, int factionIndex, Vec2i pos);
	void giveResource(const string &resourceName, int factionIndex, int amount);
	void givePositionCommand(int unitId, const string &producedName, const Vec2i &pos);
	void giveProductionCommand(int unitId, const string &producedName);
	void giveUpgradeCommand(int unitId, const string &upgradeName);
	void disableAi(int factionIndex);
	void setPlayerAsWinner(int factionIndex);
	void endGame();

	//wrappers, queries
	//NEW
	const string &getPlayerName(int factionIndex);
	const string &getFactionTypeName(int factionIndex);
	const string &getScenarioDir();
	//NEW END
	Vec2i getStartLocation(int factionIndex);
	Vec2i getUnitPosition(int unitId);
	int getUnitFaction(int unitId);
	int getResourceAmount(const string &resourceName, int factionIndex);
	const string &getLastCreatedUnitName();
	int getLastCreatedUnitId();
	const string &getLastDeadUnitName();
	int getLastDeadUnitId();
	int getUnitCount(int factionIndex);
	int getUnitCountOfType(int factionIndex, const string &typeName);

	//callbacks, commands
	static int setTimer(LuaHandle* luaHandle);
	//static int setInterval(LuaHandle* luaHandle);
	static int stopTimer(LuaHandle* luaHandle);
	static int showMessage(LuaHandle* luaHandle);
	static int setDisplayText(LuaHandle* luaHandle);
	static int clearDisplayText(LuaHandle* luaHandle);
	static int setCameraPosition(LuaHandle* luaHandle);
	static int createUnit(LuaHandle* luaHandle);
	static int giveResource(LuaHandle* luaHandle);
	static int givePositionCommand(LuaHandle* luaHandle);
	static int giveProductionCommand(LuaHandle* luaHandle);
	static int giveUpgradeCommand(LuaHandle* luaHandle);
	static int disableAi(LuaHandle* luaHandle);
	static int setPlayerAsWinner(LuaHandle* luaHandle);
	static int endGame(LuaHandle* luaHandle);

	//callbacks, queries
	static int getPlayerName(LuaHandle* luaHandle);
	static int getFactionTypeName(LuaHandle* luaHandle);
	static int getScenarioDir(LuaHandle* luaHandle);
	static int getStartLocation(LuaHandle* luaHandle);
	static int getUnitPosition(LuaHandle* luaHandle);
	static int getUnitFaction(LuaHandle* luaHandle);
	static int getResourceAmount(LuaHandle* luaHandle);
	static int getLastCreatedUnitName(LuaHandle* luaHandle);
	static int getLastCreatedUnitId(LuaHandle* luaHandle);
	static int getLastDeadUnitName(LuaHandle* luaHandle);
	static int getLastDeadUnitId(LuaHandle* luaHandle);
	static int getUnitCount(LuaHandle* luaHandle);
	static int getUnitCountOfType(LuaHandle* luaHandle);
};

}}//end namespace

#endif
