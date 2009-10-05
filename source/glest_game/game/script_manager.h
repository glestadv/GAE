// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
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
#include <set>
#include <map>
#include <limits>

#include "lua_script.h"
#include "vec.h"
#include "timer.h"

#include "components.h"
#include "game_constants.h"

using namespace std;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec4i;
using Shared::Platform::Chrono;
using namespace Shared::Lua;

namespace Glest{ namespace Game { 

class World;
class Unit;
class GameCamera;
class GameSettings;
class Game;
class UnitType;


// =====================================================
//	class ScriptTimer
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

struct Region {
	virtual bool isInside(const Vec2i &pos) const = 0;
};

struct Rect : public Region {
	int x, y, w, h; // top-left coords + width and height

	Rect() : x(0), y(0), w(0), h(0) { }
	Rect(const int v) : x(v), y(v), w(v), h(v) { }
	Rect(const Vec4i &v) : x(v.x), y(v.y), w(v.z), h(v.w) { }
	Rect(const int x, const int y, const int w, const int h) : x(x), y(y), w(w), h(h) { }

	virtual bool isInside(const Vec2i &pos) const {
		return pos.x >= x && pos.y >= y && pos.x < x + w && pos.y < y + h;
	}
};

struct Circle : public Region {
	int x, y; // centre
	float radius;

	Circle() : x(-1), y(-1), radius(numeric_limits<float>::quiet_NaN()) { }
	Circle(const Vec2i &pos, const float radius) : x(pos.x), y(pos.y), radius(radius) { }
	Circle(const int x, const int y, const float r) : x(x), y(y), radius(r) { }

	virtual bool isInside(const Vec2i &pos) const {
		return pos.dist(Vec2i(x,y)) <= radius;
	}
};

struct CompoundRegion : public Region {
	vector<Region*> regions;

	CompoundRegion() { }
	CompoundRegion(Region *ptr) { regions.push_back(ptr); }

	template<typename InIter>
	void addRegions(InIter start, InIter end) {
		copy(start,end,regions.end())
	}

	virtual bool isInside(const Vec2i &pos) const {
		for ( vector<Region*>::const_iterator it = regions.begin(); it != regions.end(); ++it ) {
			if ( (*it)->isInside(pos) ) {
				return false;
			}
		}
		return false;
	}
};

// =====================================================
//	class LocationEventManager
// =====================================================

class LocationEventManager {
	// named regions
	map<string,Region*> regions;
	
	// named events, maps to region
	map<string,string>  events;

	// maps to event name
	multimap<int, string> unitIdTriggers;
	multimap<int, string> factionIndexTriggers;
	multimap<int, string> teamIndexTriggers;
	//map<int, multimap<const UnitType*, string>> factionUnitTypeTriggers;

public:
	LocationEventManager() { reset(); }

	void reset() {
		regions.clear();
		events.clear();
		unitIdTriggers.clear();
		factionIndexTriggers.clear();
		teamIndexTriggers.clear();
		//factionUnitTypeTriggers.clear();
	}

	bool registerRegion(const string &name, const Rect &rect) {
		if ( regions.find(name) != regions.end() ) {
			return false;
		}
		Region *region = new Rect(rect);
		regions[name] = region;
		return true;
	}

	bool registerEvent(const string &name, const string &region) {
		if ( events.find(name) != events.end() || regions.find(region) == regions.end() ) {
			return false;
		}
		events[name] = region;
		return true;
	}

	// must be called any time a unit is 'put' in cells (created, moved, 
	void unitMoved(Unit *unit);
	
	void addUnitIdTrigger(int unitId, const string &event) {
		unitIdTriggers.insert(pair<int,string>(unitId,event));
	}
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
	PlayerModifiers() : winner(false), aiEnabled(true) { }

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

private:
	typedef queue<ScriptManagerMessage> MessageQueue;

	//lua
	static string code;
	static LuaScript luaScript;

	//misc
	static MessageQueue messageQueue;
	static GraphicMessageBox messageBox;
	static string displayText;
	
	//last created unit
	static string lastCreatedUnitName;
	static int lastCreatedUnitId;

	//last dead unit
	static string lastDeadUnitName;
	static int lastDeadUnitId;

	// end game state
	static bool gameOver;
	static PlayerModifiers playerModifiers[GameConstants::maxPlayers];

	static vector<ScriptTimer> timers;
	static vector<ScriptTimer> newTimerQueue;

	static set<string> definedEvents;

	static LocationEventManager locationEventManager;
	//static ScriptManager* thisScriptManager;

	static const int messageWrapCount;
	static const int displayTextWrapCount;

public:
	static void init ();

	//message box functions
	static bool getMessageBoxEnabled() 									{return !messageQueue.empty();}
	static GraphicMessageBox* getMessageBox()							{return &messageBox;}
	static string getDisplayText() 										{return displayText;}
	static bool getGameOver() 											{return gameOver;}
	static const PlayerModifiers *getPlayerModifiers(int factionIndex) 	{return &playerModifiers[factionIndex];}	

	//events
	static void onMessageBoxOk();
	static void onResourceHarvested();
	static void onUnitCreated(const Unit* unit);
	static void onUnitDied(const Unit* unit);
	static void onTimer();
	static void onTrigger(const string &name);

	static void unitMoved(Unit *unit) { locationEventManager.unitMoved(unit); }

private:
	static string wrapString(const string &str, int wrapCount);
	static string describeLuaStack ( LuaArguments &args );
	static void luaCppCallError ( const string &func, const string &expected, const string &received, 
											const string extra = "Wrong number of parameters." ) ;

	//
	// LUA callbacks
	//
	
	// commands
	static int setTimer(LuaHandle* luaHandle);
	static int stopTimer(LuaHandle* luaHandle);
	static int registerRegion(LuaHandle* luaHandle);
	static int registerEvent(LuaHandle* luaHandle);
	static int setUnitTrigger(LuaHandle* luaHandle);
	static int showMessage(LuaHandle* luaHandle);
	static int setDisplayText(LuaHandle* luaHandle);
	static int clearDisplayText(LuaHandle* luaHandle);
	static int setCameraPosition(LuaHandle* luaHandle);
	static int createUnit(LuaHandle* luaHandle);
	static int giveResource(LuaHandle* luaHandle);
	static int givePositionCommand(LuaHandle* luaHandle);
	static int giveTargetCommand ( LuaHandle * luaHandle );
	static int giveStopCommand ( LuaHandle * luaHandle );
	static int giveProductionCommand(LuaHandle* luaHandle);
	static int giveUpgradeCommand(LuaHandle* luaHandle);
	static int disableAi(LuaHandle* luaHandle);
	static int setPlayerAsWinner(LuaHandle* luaHandle);
	static int endGame(LuaHandle* luaHandle);
	static int debugLog ( LuaHandle* luaHandle );
	static int consoleMsg ( LuaHandle* luaHandle );

	// queries
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

