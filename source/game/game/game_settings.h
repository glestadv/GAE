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

#ifndef _GLEST_GAME_GAMESETTINGS_H_
#define _GLEST_GAME_GAMESETTINGS_H_

#include <string>
#include "game_constants.h"
#include "xml_parser.h"

using std::string;
using Shared::Xml::XmlNode;

namespace Glest { namespace Sim {
using namespace GameConstants;

// =====================================================
//	class GameSettings
// =====================================================

class GameSettings {
private:
	string description;
	string mapPath;
	string tilesetPath;
	string techPath;
	string factionTypeNames[maxPlayers]; //faction names
	string playerNames[maxPlayers];
	string scenarioPath;

	ControlType factionControls[maxPlayers];
	float resourceMultipliers[maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[maxPlayers];
	int startLocationIndex[maxPlayers];
	int colourIndices[maxPlayers];

	bool defaultUnits;
	bool defaultResources;
	bool defaultVictoryConditions;

	bool fogOfWar;
	bool randomStartLocs;

public:
	GameSettings() { clear(); }
	GameSettings(const XmlNode *node);

	void clear();
	void compact();
	// use default copy ctor
	//GameSettings(const GameSettings &gs);

	//get
	const string &getDescription() const			{return description;}
	const string &getMapPath() const 				{return mapPath;}
	const string &getTilesetPath() const			{return tilesetPath;}
	const string &getTechPath() const				{return techPath;}
	const string &getScenarioPath() const			{return scenarioPath;}

	const string &getFactionTypeName(int i) const	{return factionTypeNames[i];}
	const string &getPlayerName(int i) const		{return playerNames[i];}
	ControlType getFactionControl(int i) const		{return factionControls[i];}
	float getResourceMultilpier(int i) const		{return resourceMultipliers[i];}
	int getThisFactionIndex() const					{return thisFactionIndex;}
	int getFactionCount() const						{return factionCount;}
	int getTeam(int i) const						{return teams[i];}
	int getStartLocationIndex(int i) const			{return startLocationIndex[i];}
	int getColourIndex(int i) const					{return colourIndices[i];}
	
	bool getDefaultUnits() const					{return defaultUnits;}
	bool getDefaultResources() const				{return defaultResources;}
	bool getDefaultVictoryConditions() const		{return defaultVictoryConditions;}
	bool getFogOfWar() const						{return fogOfWar;}
	bool getRandomStartLocs() const					{return randomStartLocs;}

	//set
	void setDescription(const string& v)			{description = v;}
	void setMapPath(const string& v)				{mapPath = v;}
	void setTilesetPath(const string& v)			{tilesetPath = v;}
	void setTechPath(const string& v)				{techPath = v;}
	void setScenarioPath(const string& v)			{scenarioPath = v;}

	void setFactionTypeName(int i, const string& v)	{factionTypeNames[i] = v;}
	void setPlayerName(int i, const string &v)		{playerNames[i] = v;}
	void setFactionControl(int i, ControlType v)	{factionControls[i]= v;}
	void setResourceMultiplier(int i, float v)		{resourceMultipliers[i] = v;}
	void setThisFactionIndex(int v) 				{thisFactionIndex = v;}
	void setFactionCount(int v)						{factionCount = v;}
	void setTeam(int i, int v)						{teams[i] = v;}
	void setStartLocationIndex(int i, int v)		{startLocationIndex[i] = v;}
	void setColourIndex(int i, int v)				{colourIndices[i] = v;}

	void setDefaultUnits(bool v) 					{defaultUnits = v;}
	void setDefaultResources(bool v) 				{defaultResources = v;}
	void setDefaultVictoryConditions(bool v) 		{defaultVictoryConditions = v;}
	void setFogOfWar(bool v)						{fogOfWar = v;}
	void setRandomStartLocs(bool v)					{randomStartLocs = v;}

	//misc
	void randomizeLocs(int maxPlayers);
	void save(XmlNode *node) const;
};

}}//end namespace

#endif
