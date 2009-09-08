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

#ifndef _GAME_STATS_H_
#define _GAME_STATS_H_

#include <string>

#include "game_constants.h"
#include "faction.h"
#include "xml_parser.h"
#include "game_settings.h"

using std::string;
using Shared::Xml::XmlNode;

namespace Game {

// =====================================================
// 	class Stats
// =====================================================

/** Player statistics that are shown after the game ends */
class Stats {
public:
	struct FactionStats {
		FactionStats();

		bool victory;
		int kills;
		int deaths;
		int unitsProduced;
		int resourcesHarvested;
	};

private:
	// FIXME: this guy gets deleted
	const GameSettings &gs;
	FactionStats factionStats[GameConstants::maxFactions];

public:
	Stats(const GameSettings &gs) : gs(gs), factionStats() {}
	void load(const XmlNode *n);
	void save(XmlNode *n) const;

	const GameSettings &getGameSettings() const		{return gs;}
	bool getVictory(int i) const					{return factionStats[i].victory;}
	int getKills(int i) const						{return factionStats[i].kills;}
	int getDeaths(int i) const						{return factionStats[i].deaths;}
	int getUnitsProduced(int i) const				{return factionStats[i].unitsProduced;}
	int getResourcesHarvested(int i) const			{return factionStats[i].resourcesHarvested;}


	void setVictorious(int i)						{factionStats[i].victory = true;}
	void produce(int i)								{factionStats[i].unitsProduced++;}
	void harvest(int i, int amount)					{factionStats[i].resourcesHarvested += amount;}
	void kill(int killerIndex, int killedIndex) {
		if(killerIndex != killedIndex) {
			factionStats[killerIndex].kills++;
		}
		factionStats[killedIndex].deaths++;
	}
};

} // end namespace

#endif
