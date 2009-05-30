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

#include "pch.h"
#include "stats.h"

#include "leak_dumper.h"


namespace Game {

Stats::FactionStats::FactionStats() :
		victory(false),
		kills(0),
		deaths(0),
		unitsProduced(0),
		resourcesHarvested(0) {
}

// =====================================================
// class Stats
// =====================================================

void Stats::load(const XmlNode *node) {
	for(int i = 0; i < gs.getFactionCount(); ++i) {
		const XmlNode *n = node->getChild("faction", i);
		factionStats[i].victory = n->getChildBoolValue("victory");
		factionStats[i].kills = n->getChildIntValue("kills");
		factionStats[i].deaths = n->getChildIntValue("deaths");
		factionStats[i].unitsProduced = n->getChildIntValue("unitsProduced");
		factionStats[i].resourcesHarvested = n->getChildIntValue("resourcesHarvested");
	}
}

void Stats::save(XmlNode *node) const {
	for(int i = 0; i < gs.getFactionCount(); ++i) {
		XmlNode *n = node->addChild("faction");
		n->addChild("victory", factionStats[i].victory);
		n->addChild("kills", factionStats[i].kills);
		n->addChild("deaths", factionStats[i].deaths);
		n->addChild("unitsProduced", factionStats[i].unitsProduced);
		n->addChild("resourcesHarvested", factionStats[i].resourcesHarvested);
	}
}

} // end namespace
