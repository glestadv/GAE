// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_start_game_base.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "game.h"
#include "network_manager.h"
#include "xml_parser.h"

#include "leak_dumper.h"

namespace Game {

using namespace Shared::Util;

// =====================================================
// 	class MenuStateStartGameBase
// =====================================================

MenuStateStartGameBase::MenuStateStartGameBase(Program &program, MainMenu *mainMenu, const string &stateName)
		: MenuState(program, mainMenu, stateName)
		, msgBox(NULL)
		, gs() {
}

MenuStateStartGameBase::~MenuStateStartGameBase() {
}

// ============ PROTECTED ===========================

void MenuStateStartGameBase::initGameSettings(GameInterface &gi) {
	gs = gi.getGameSettings();
}

void MenuStateStartGameBase::loadMapInfo(string file, MapInfo *mapInfo) {

	// FIXME: This is terrible.  All of this map stuff should be in the shared library.
	
	struct MapFileHeader {
		int32 version;
		int32 maxFactions;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
	};

	Lang &lang = Lang::getInstance();

	try {
		FILE *f = fopen(file.c_str(), "rb");

		if(!f) {
			throw runtime_error(strerror(errno));
		}

		MapFileHeader header;
		fread(&header, sizeof(MapFileHeader), 1, f);
		mapInfo->size.x = header.width;
		mapInfo->size.y = header.height;
		mapInfo->players = header.maxFactions;
		mapInfo->desc = lang.get("MaxPlayers") + ": " + intToStr(mapInfo->players) + "\n";
		mapInfo->desc += lang.get("Size") + ": " + intToStr(mapInfo->size.x) + " x " + intToStr(mapInfo->size.y);
		fclose(f);
	} catch (exception e) {
		throw runtime_error("Error loading map file: " + file + '\n' + e.what());
	}
}

void MenuStateStartGameBase::updateNetworkSlots() {
	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();
	//assert(getGameSettings()->isValid());

	//const GameSettings::Factions &factions = getGameSettings()->getFactions();
	//for(GameSettings::Factions::const_iterator i = factions.begin(); i != factions.end(); ++i) {
#if 0
	foreach(const shared_ptr<GameSettings::Faction> &f, getGameSettings()->getFactions()) {
		int slot = f->getMapSlot();
		int ct = f->getControlType();
		RemoteClientInterface *client = serverInterface->findClientForSlot(slot);
		if(ct == CT_NETWORK) {
			if (!client) {
				client = serverInterface->findUnslottedClient();
				if(client) {
					serverInterface->assignClientToSlot(client, slot);
				}
			}			
		} else {
			if (client) {
				serverInterface->removeClientFromSlot(client);
			}
		}
	}
#endif
}

} // end namespace
