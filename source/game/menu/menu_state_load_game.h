// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_MENUSTATELOADGAME_H_
#define _GAME_MENUSTATELOADGAME_H_

#include "menu_state_start_game_base.h"
#include "thread.h"

namespace Game {

class MenuStateLoadGame;

// ===============================
// 	class SavedGamePreviewLoader
// ===============================
/**
 * SavedGamePreviewLoader is a background thread loader for saved game previews.  This prevents the
 * UI thread from having to completely load a saved game and generate a preview before processing
 * user input again, thus potentially causing the user to wait WAY too much time just to find the
 * saved game they want to load.
 */
class SavedGamePreviewLoader : public Thread {
	MenuStateLoadGame &menu;
	string *fileName;		//only modify with mutex locked
	bool seeJaneRun;
	Mutex mutex;

public:
	SavedGamePreviewLoader(MenuStateLoadGame &menu) : menu(menu) {
		fileName = NULL;
		seeJaneRun = true;
		start();
	}

	/** Tells this SavedGamePreviewLoader to stop executing and prepare to be deleted. */
	void goAway() {
		MutexLock lock(mutex);
		seeJaneRun = false;
	}

	/** Sets the name of the file to generate a preview for. */
	void setFileName(const string &fileName) {
		MutexLock lock(mutex);
		if(!this->fileName) {
			this->fileName = new string(fileName);
		}
	}

protected:
	/** Main thread loop (called by Thread). */
	virtual void execute();

private:
	/** Called only by private thread: Called to load a preview of the specified saved game file. */
	void loadPreview(string *fileName);
};


// ===============================
// 	class MenuStateLoadGame
// ===============================

class MenuStateLoadGame : public MenuStateStartGameBase, Uncopyable {
private:
	SavedGamePreviewLoader loaderThread;
	//GraphicButton buttonReturn;
	GraphicButton buttonDelete;
	//GraphicButton buttonPlayNow;

	// only modify with mutex locked ==>
	GraphicLabel labelInfoHeader;
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelControls[GameConstants::maxPlayers];
	GraphicLabel labelFactions[GameConstants::maxPlayers];
	GraphicLabel labelTeams[GameConstants::maxPlayers];
	//GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	ControlType controlTypes[GameConstants::maxPlayers];
	string fileName;
	const XmlNode *savedGame;
	// Note: gs is defined in base class, but in MenuStateLoadGame, should only be read or modified
	// with mutex locked
	// <== only modify with mutex locked

	GraphicListBox listBoxGames;
	GraphicLabel labelNetwork;

	GraphicMessageBox *confirmMessageBox;
	bool criticalError;
	Mutex mutex;
	vector<string> fileNames;
	vector<string> prettyNames;

public:
	MenuStateLoadGame(Program &program, MainMenu *mainMenu);
	~MenuStateLoadGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState &mouseState);
	void render();
	void update();

	/** returns the name of the new file name if the user moved along since the load started. */
	string *setGameInfo(const string &fileName, const XmlNode *root, const string &err);
	
protected:
	void updateGameSettings() {}

private:
	bool loadGameList();
	bool loadGame();
	string getFileName();
	void selectionChanged();
	void initGameInfo();
};


} // end namespace

#endif
