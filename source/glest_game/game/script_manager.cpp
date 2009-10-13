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

#include "pch.h"

#include <stdarg.h>

#include "script_manager.h"

#include "world.h"
#include "lang.h"
#include "game_camera.h"
#include "game.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Lua;

namespace Glest{ namespace Game{

// =====================================================
//	class ScriptTimer
// =====================================================

bool ScriptTimer::ready() {
	if ( real ) {
		return Chrono::getCurMillis() >= targetTime;
	}
	return theWorld.getFrameCount() >= targetTime;
}

void ScriptTimer::reset() {
	if ( real ) {
		targetTime = Chrono::getCurMillis () + interval * 1000;
	} else {
		targetTime = theWorld.getFrameCount() + interval;
	}
}

// =====================================================
//	class LocationEventManager
// =====================================================

void LocationEventManager::unitMoved( Unit *unit ) {
	multimap<int,string>::iterator it;
	
	// check id
	it = unitIdTriggers.lower_bound(unit->getId());
	while ( it != unitIdTriggers.upper_bound(unit->getId()) ) {
		if ( regions[events[it->second]]->isInside(unit->getPos()) ) {
			ScriptManager::onTrigger(it->second);
			// remove trigger ??
			it = unitIdTriggers.erase(it);
		} else {
			++it;
		}
	}
	// check faction index
	it = factionIndexTriggers.lower_bound(unit->getFactionIndex());
	while ( it != factionIndexTriggers.upper_bound(unit->getFactionIndex()) ) {
		if ( regions[events[it->second]]->isInside(unit->getPos()) ) {
			ScriptManager::onTrigger(it->second);
			// remove ?
			it = factionIndexTriggers.erase(it);
		} else {
			++it;
		}
	}
	// check team index
	it = teamIndexTriggers.lower_bound(unit->getTeam());
	while ( it != teamIndexTriggers.upper_bound(unit->getTeam()) ) {
		if ( regions[events[it->second]]->isInside(unit->getPos()) ) {
			ScriptManager::onTrigger(it->second);
			// remove ?
			it = teamIndexTriggers.erase(it);
		} else {
			++it;
		}
	}
	// check unit type for faction index
}

LocationEventManager ScriptManager::locationEventManager;

// =====================================================
//	class ScriptManager
// =====================================================

// ========== statics ==========

//ScriptManager* ScriptManager::thisScriptManager= NULL;
const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

string				ScriptManager::code;
LuaScript			ScriptManager::luaScript;
GraphicMessageBox	ScriptManager::messageBox;
string				ScriptManager::displayText;
string				ScriptManager::lastCreatedUnitName;
int					ScriptManager::lastCreatedUnitId;
string				ScriptManager::lastDeadUnitName;
int					ScriptManager::lastDeadUnitId;
bool				ScriptManager::gameOver;
PlayerModifiers		ScriptManager::playerModifiers[GameConstants::maxPlayers];
vector<ScriptTimer> ScriptManager::timers;
vector<ScriptTimer> ScriptManager::newTimerQueue;
set<string>			ScriptManager::definedEvents;
ScriptManager::MessageQueue ScriptManager::messageQueue;


void ScriptManager::init () {
	const Scenario*	scenario = theWorld.getScenario();
	assert( scenario );
	luaScript.startUp();
	assert ( ! luaScript.isDefined ( "startup" ) );

	//register functions
	luaScript.registerFunction(setTimer, "setTimer");
	luaScript.registerFunction(stopTimer, "stopTimer");
	luaScript.registerFunction(registerRegion, "registerRegion");
	luaScript.registerFunction(registerEvent, "registerEvent");
	luaScript.registerFunction(setUnitTrigger, "setUnitTrigger");
	luaScript.registerFunction(setFactionTrigger, "setFactionTrigger");
	luaScript.registerFunction(setTeamTrigger, "setTeamTrigger");
	luaScript.registerFunction(showMessage, "showMessage");
	luaScript.registerFunction(setDisplayText, "setDisplayText");
	luaScript.registerFunction(clearDisplayText, "clearDisplayText");
	luaScript.registerFunction(setCameraPosition, "setCameraPosition");
	luaScript.registerFunction(createUnit, "createUnit");
	luaScript.registerFunction(giveResource, "giveResource");
	luaScript.registerFunction(givePositionCommand, "givePositionCommand");
	luaScript.registerFunction(giveProductionCommand, "giveProductionCommand");
	luaScript.registerFunction(giveStopCommand, "giveStopCommand");
	luaScript.registerFunction(giveTargetCommand, "giveTargetCommand");
	luaScript.registerFunction(giveUpgradeCommand, "giveUpgradeCommand");
	luaScript.registerFunction(disableAi, "disableAi");
	luaScript.registerFunction(setPlayerAsWinner, "setPlayerAsWinner");
	luaScript.registerFunction(endGame, "endGame");
	luaScript.registerFunction(debugLog, "debugLog");
	luaScript.registerFunction(consoleMsg, "consoleMsg");

	luaScript.registerFunction(getPlayerName, "playerName");
	luaScript.registerFunction(getFactionTypeName, "factionTypeName");
	luaScript.registerFunction(getScenarioDir, "scenarioDir");
	luaScript.registerFunction(getStartLocation, "startLocation");
	luaScript.registerFunction(getUnitPosition, "unitPosition");
	luaScript.registerFunction(getUnitFaction, "unitFaction");
	luaScript.registerFunction(getResourceAmount, "resourceAmount");
	luaScript.registerFunction(getLastCreatedUnitName, "lastCreatedUnitName");
	luaScript.registerFunction(getLastCreatedUnitId, "lastCreatedUnit");
	luaScript.registerFunction(getLastDeadUnitName, "lastDeadUnitName");
	luaScript.registerFunction(getLastDeadUnitId, "lastDeadUnit");
	luaScript.registerFunction(getUnitCount, "unitCount");
	luaScript.registerFunction(getUnitCountOfType, "unitCountOfType");
	

	//load code
	for(int i= 0; i<scenario->getScriptCount(); ++i){
		const Script* script= scenario->getScript(i);
		luaScript.loadCode("function " + script->getName() + "()" + script->getCode() + " end\n", script->getName());
	}
	
	// get globs, startup, unitDied, unitDiedOfType_xxxx (archer, worker, etc...) etc. 
	//  need unit names of all loaded factions
	// put defined function names in definedEvents, check membership before doing luaCall()
	set<string> funcNames;
	funcNames.insert ( "startup" );
	funcNames.insert ( "unitDied" );
	funcNames.insert ( "unitCreated" );
	funcNames.insert ( "resourceHarvested" );
	for ( int i=0; i < theWorld.getFactionCount(); ++i ) {
		const FactionType *f = theWorld.getFaction( i )->getType();
		for ( int j=0; j < f->getUnitTypeCount(); ++j ) {
			const UnitType *ut = f->getUnitType( j );
			funcNames.insert( "unitCreatedOfType_" + ut->getName() );
		}
	}
	for ( set<string>::iterator it = funcNames.begin(); it != funcNames.end(); ++it ) {
		if ( luaScript.isDefined( *it ) ) {
			definedEvents.insert( *it );
		}
	}

	locationEventManager.reset();

	//setup message box
	messageBox.init( "", Lang::getInstance().get("Ok") );
	messageBox.setEnabled(false);

	//last created unit
	lastCreatedUnitId= -1;
	lastDeadUnitId= -1;
	gameOver= false;

	//call startup function
	luaScript.luaCall("startup");
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk() {
	Lang &lang= Lang::getInstance();
	
	if(!messageQueue.empty()){
		messageQueue.pop();
		if(!messageQueue.empty()){
			messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
			messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		}
	}

	//close the messageBox now all messages have been shown
	if ( messageQueue.empty() ) {
		messageBox.setEnabled(false);
		theGame.resume();
	}
}

void ScriptManager::onResourceHarvested(){
	if ( definedEvents.find( "resourceHarvested" ) != definedEvents.end() ) {
		luaScript.luaCall("resourceHarvested");
	}
}

void ScriptManager::onUnitCreated(const Unit* unit){
	lastCreatedUnitName= unit->getType()->getName();
	lastCreatedUnitId= unit->getId();
	if ( definedEvents.find( "unitCreated" ) != definedEvents.end() ) {
		luaScript.luaCall("unitCreated");
	}
	if ( definedEvents.find( "unitCreatedOfType_"+unit->getType()->getName() ) != definedEvents.end() ) {
		luaScript.luaCall("unitCreatedOfType_"+unit->getType()->getName());
	}
}

void ScriptManager::onUnitDied(const Unit* unit){
	lastDeadUnitName= unit->getType()->getName();
	lastDeadUnitId= unit->getId();
	if ( definedEvents.find( "unitDied" ) != definedEvents.end() ) {
		luaScript.luaCall("unitDied");
	}
}

void ScriptManager::onTrigger(const string &name) {
	luaScript.luaCall("trigger_" + name );
}

void ScriptManager::onTimer() {
	// when a timer is ready, call the corresponding lua function
	// and remove the timer, or reset to repeat.

	vector<ScriptTimer>::iterator timer;

	for (timer = timers.begin(); timer != timers.end();) {
		if ( timer->ready() ) {
			if ( timer->isAlive() ) {
				if ( ! luaScript.luaCall("timer_" + timer->getName()) ) {
					timer->kill();
					theConsole.addLine( "Error: timer_" + timer->getName() + " is not defined." );
				}
			}
			if ( timer->isPeriodic() && timer->isAlive() ) {
				timer->reset();
			} 
			else {
				timer = timers.erase( timer ); //returns next element
				continue;
			}
		}
		++timer;
	}
	for ( vector<ScriptTimer>::iterator it = newTimerQueue.begin(); it != newTimerQueue.end(); ++it ) {
		timers.push_back ( *it );
	}
	newTimerQueue.clear();
}

// =============== util ===============

string ScriptManager::wrapString(const string &str, int wrapCount){

	string returnString;

	int letterCount= 0;
	for(int i= 0; i<str.size(); ++i){
		if(letterCount>wrapCount && str[i]==' '){
			returnString+= '\n';
			letterCount= 0;
		}
		else {
			returnString+= str[i];
		}
		++letterCount;
	}

	return returnString;
}

// =============== Error handling bits ===============

/** Extracts arguments for Lua callbacks.
  * <p>uses a string description of the expected arguments in arg_desc, then pointers the 
  * corresponding variables to populate the results.</p>
  * @param luaArgs the LuaArguments object
  * @param caller name of the lua callback, in case of errors
  * @param arg_desc argument descriptor, a string of the form "[xxx[,xxx[,etc]]]" where xxx can be any of
  * <ul><li>'int' for an integer</li>
  *		<li>'str' for a string</li>
  *		<li>'bln' for a boolean</li>
  *		<li>'v2i' for a Vec2i</li>
  *		<li>'v4i' for a Vec4i</li></ul>
  * @param ... a pointer to the appropriate type for each argument specified in arg_desc
  * @return true if arguments were extracted successfully, false if there was an error
  */
bool ScriptManager::extractArgs(LuaArguments &luaArgs, const char *caller, const char *arg_desc, ...) {
	if ( !*arg_desc ) {
		if ( luaArgs.getArgumentCount() ) {
			theConsole.addLine( "Error: " + string(caller) + "() expected 0 arguments, got " 
					+ intToStr(luaArgs.getArgumentCount()) );
			return false;
		} else {
			return true;
		}
	}
	const char *ptr = arg_desc;
	int expected = 1;
	while ( *ptr ) {
		if ( *ptr++ == ',' ) expected++;
	}
	if ( expected != luaArgs.getArgumentCount() ) {
		theConsole.addLine("Error: " + string(caller) + "() expected " + intToStr(expected) 
			+ " arguments, got " + intToStr(luaArgs.getArgumentCount()));
		return false;
	}
	va_list vArgs;
	va_start(vArgs, arg_desc);
	char *tmp = strcpy (new char[strlen(arg_desc)], arg_desc);
	char *tok = strtok(tmp, ",");
	try {
		while ( tok ) {
			if ( strcmp(tok,"int") == 0 ) {
				*va_arg(vArgs,int*) = luaArgs.getInt(-expected);
			} else if ( strcmp(tok,"str") == 0 ) {
				*va_arg(vArgs,string*) = luaArgs.getString(-expected);
			} else if ( strcmp(tok,"bln") == 0 ) {
				*va_arg(vArgs,bool*) = luaArgs.getBoolean(-expected);
			} else if ( strcmp(tok,"v2i") == 0 ) {
				*va_arg(vArgs,Vec2i*) = luaArgs.getVec2i(-expected);
			} else if ( strcmp(tok,"v4i") == 0 ) {
				*va_arg(vArgs,Vec4i*) = luaArgs.getVec4i(-expected);
			} else {
				throw runtime_error("ScriptManager::extractArgs() passed bad arg_desc string");
			}
			expected--;
			tok = strtok(NULL, ",");
		}
	} catch ( LuaError e ) {
		theConsole.addLine("Error: " + string(caller) + "() " + e.desc());
		va_end(vArgs);
		return false;
	}
	return true;
}

// ========================== lua callbacks ===============================================

int ScriptManager::debugLog(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string msg;
	if ( extractArgs(args, "debugLog", "str", &msg) ) {
		Logger::getErrorLog().add(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::consoleMsg (LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string msg;
	if ( extractArgs(args, "consoleMsg", "str", &msg) ) {
		theConsole.addLine(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::setTimer(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name, type;
	int period;
	bool repeat;
	if ( extractArgs(args, "setTimer", "str,str,int,bln", &name, &type, &period, &repeat) ) {
		if ( type == "real" ) {
			newTimerQueue.push_back(ScriptTimer(name, true, period, repeat));
		} else if ( type == "game" ) {
			newTimerQueue.push_back(ScriptTimer(name, false, period, repeat));
		} else {
			theConsole.addLine( "Error: setTimer() called with illegal second parameter." );
		}
	}
	return args.getReturnCount(); // == 0
}

int ScriptManager::stopTimer(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string name;
	if ( extractArgs(args, "stopTimer", "str", &name) ) {
		vector<ScriptTimer>::iterator i;
		for (i = timers.begin(); i != timers.end(); ++i) {
			if (i->getName() == name) {
				i->kill ();
				break;
			}
		}
	} 
	return args.getReturnCount();
}

int ScriptManager::registerRegion(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name;
	Vec4i rect;
	if ( extractArgs(args, "registerRegion", "str,v4i", &name, &rect) ) {
		locationEventManager.registerRegion(name, rect);
	}
	return args.getReturnCount();
}

int ScriptManager::registerEvent(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name, condition;
	if ( extractArgs(args, "registerEvent", "str,str", &name, &condition) ) {
		if ( condition == "attacked" ) {
		} else if ( condition == "enemy_sighted" ) {
		} else { // 'complex' conditions
			size_t ePos = condition.find('=');
			if ( ePos != string::npos ) {
				string key = condition.substr(0, ePos);
				string val = condition.substr(ePos+1);
				if ( key == "hp_below" ) {
				} else if ( key == "region" ) { // look, this one does something!
					locationEventManager.registerEvent(name,val);
				}
			}
		}
	}
	return args.getReturnCount();
}

int ScriptManager::setUnitTrigger(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string name;
	if ( extractArgs(args, "setUnitTrigger", "int,str", &id, &name) ) {
		locationEventManager.addUnitIdTrigger(id, name);
	}
	return args.getReturnCount();
}

int ScriptManager::setFactionTrigger(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	string name;
	if ( extractArgs(args, "setFactionTrigger", "int,str", &fNdx, &name) ) {
		locationEventManager.addFactionTrigger(fNdx, name);
	}
	return args.getReturnCount();
}

int ScriptManager::setTeamTrigger(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int tNdx;
	string name;
	if ( extractArgs(args, "setTeamTrigger", "int,str", &tNdx, &name) ) {
		locationEventManager.addTeamTrigger(tNdx, name);
	}
	return args.getReturnCount();
}

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	Lang &lang= Lang::getInstance();
	string txt, hdr;
	if ( extractArgs(args, "showMessage", "str,str", &txt, &hdr) ) {
		theGame.pause ();
		ScriptManagerMessage msg(txt, hdr);
		messageQueue.push ( msg );
		if ( !messageBox.getEnabled() ) {
			messageBox.setEnabled ( true );
			messageBox.setText ( wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount) );
			messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::setDisplayText(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string txt;
	if ( extractArgs(args,"setDisplayText", "str", &txt) ) {
		displayText= wrapString(Lang::getInstance().getScenarioString(txt), displayTextWrapCount);
	}
	return args.getReturnCount();
}

int ScriptManager::clearDisplayText(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	displayText= "";
	return args.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	Vec2i pos;
	if ( extractArgs(args, "setCameraPosition", "v2i", &pos) ) {
		theCamera.centerXZ((float)pos.x, (float)pos.y);
	}
	return args.getReturnCount();
}

int ScriptManager::createUnit(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string type;
	int fNdx;
	Vec2i pos;
	if ( extractArgs(args, "createUnit", "str,int,v2i", &type, &fNdx, &pos) ) {
		theWorld.createUnit(type, fNdx, pos);
	}
	return args.getReturnCount(); // == 0  Why not return ID ?
}

int ScriptManager::giveResource(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string resource;
	int fNdx, amount;
	if ( extractArgs(args, "giveResource", "str,int,int", &resource, &fNdx, &amount) ) {
		theWorld.giveResource(resource, fNdx, amount);
	}
	return args.getReturnCount();
}

int ScriptManager::givePositionCommand(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	Vec2i pos;
	if ( extractArgs(args, "givePositionCommand", "int,str,v2i", &id, &cmd, &pos) ) {
		theWorld.givePositionCommand(id, cmd, pos);
	}
	return args.getReturnCount();
}

int ScriptManager::giveTargetCommand ( LuaHandle * luaHandle ) {
	LuaArguments args(luaHandle);
	int id, id2;
	string cmd;
	if ( extractArgs(args, "giveTargetCommand", "int,str,int", &id, &cmd, &id2) ) {
		theWorld.giveTargetCommand(id, cmd, id2);
	}
	return args.getReturnCount();
}

int ScriptManager::giveStopCommand ( LuaHandle * luaHandle ) {
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	if ( extractArgs(args, "giveStopCommand", "int,str", &id, &cmd) ) {
		theWorld.giveStopCommand(id, cmd);
	}
	return args.getReturnCount();	
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	if ( extractArgs(args, "giveProductionCommand", "int,str", &id, &cmd) ) {
		theWorld.giveProductionCommand(id, cmd);
	}
	return args.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	if ( extractArgs(args, "giveUpgradeCommand", "int,str", &id, &cmd) ) {
		theWorld.giveUpgradeCommand(id, cmd);
	}
	return args.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "disableAi", "int", &fNdx) ) {
		if ( fNdx < GameConstants::maxPlayers ) {
			playerModifiers[fNdx].disableAi();
		}
	}
	return args.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "setPlayerAsWinner", "int", &fNdx) ) {
		if ( fNdx < GameConstants::maxPlayers ) {
			playerModifiers[fNdx].setAsWinner();
		}
	}
	return args.getReturnCount();
}

int ScriptManager::endGame(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	gameOver = true;
	return args.getReturnCount();
}

// Queries
int ScriptManager::getPlayerName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "getPlayerName", "int", &fNdx) ) {
		string playerName= theGameSettings.getPlayerName(fNdx);
		args.returnString(playerName);
	}
	return args.getReturnCount();
}

int ScriptManager::getFactionTypeName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "getFactionTypeName", "int", &fNdx) ) {
		string factionTypeName= theGameSettings.getFactionTypeName(fNdx);
		args.returnString(factionTypeName);
	}
	return args.getReturnCount();
}

int ScriptManager::getScenarioDir(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	args.returnString(theGameSettings.getScenarioDir());
	return args.getReturnCount();
}

int ScriptManager::getStartLocation(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "getStartLocation", "int", &fNdx) ) {
		Vec2i pos= theWorld.getStartLocation(fNdx);
		args.returnVec2i(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::getUnitPosition(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int id;
	if ( extractArgs(args, "getUnitPosition", "int", &id) ) {
		Vec2i pos= theWorld.getUnitPosition(id);
		args.returnVec2i(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::getUnitFaction(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int id;
	if ( extractArgs(args, "getUnitFaction", "int", &id) ) {
		int factionIndex= theWorld.getUnitFactionIndex(id);
		args.returnInt(factionIndex);
	}
	return args.getReturnCount();
}

int ScriptManager::getResourceAmount(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string resource;
	int fNdx;
	if ( extractArgs(args, "getResourceAmount", "str,int", &resource, &fNdx) ) {
		int amount = theWorld.getResourceAmount(resource, fNdx);
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::getLastCreatedUnitName(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	args.returnString( lastCreatedUnitName );
	return args.getReturnCount();
}

int ScriptManager::getLastCreatedUnitId(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	args.returnInt(lastCreatedUnitId);
	return args.getReturnCount();
}

int ScriptManager::getLastDeadUnitName(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	args.returnString(lastDeadUnitName);
	return args.getReturnCount();
}

int ScriptManager::getLastDeadUnitId(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	args.returnInt(lastDeadUnitId);
	return args.getReturnCount();
}

int ScriptManager::getUnitCount(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "getUnitCount", "int", &fNdx) ) {
		int amount = theWorld.getUnitCount(fNdx);
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::getUnitCountOfType(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int fNdx;
	string type;
	if ( extractArgs(args, "getUnitCount", "int,str", &fNdx, &type) ) {
		int amount = theWorld.getUnitCountOfType(fNdx, type);
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

}}//end namespace
