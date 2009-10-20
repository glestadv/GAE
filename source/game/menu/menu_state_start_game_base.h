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

#ifndef _GAME_MENUSTATESTARTGAMEBASE_H_
#define _GAME_MENUSTATESTARTGAMEBASE_H_

#include "main_menu.h"

namespace Game {

// ===============================
// 	class MenuStateNewGame
// ===============================

//TODO: Cleanup: Too much commonality between new game and loag game menus.
//Consolidate like functions
class MenuStateStartGameBase: public MenuState {
protected:
	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;

	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	MapInfo mapInfo;
	GraphicMessageBox *msgBox;
	shared_ptr<GameSettings> gs;
//	bool dirty;

public:
	MenuStateStartGameBase(Program &program, MainMenu *mainMenu, const string &stateName);
	virtual ~MenuStateStartGameBase();
//	bool isDirty() const						{return dirty;}
	//virtual const shared_ptr<GameSettings> &getGameSettings() = 0;

protected:
//	void setDirty(bool v)						{dirty = v;}
	virtual void updateGameSettings() = 0;
	//shared_ptr<GameSettings> &getGameSettings()	{return gs;}
	void initGameSettings(NetworkMessenger &gi);
	void loadMapInfo(string file, MapInfo *mapInfo);
	void updateNetworkSlots();
};

} // end namespace

#endif
