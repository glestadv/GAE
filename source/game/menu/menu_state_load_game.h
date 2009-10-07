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

/**
 * @file
 * Theory: Saved games can potentially take a long time to load.  Forcing the GUI's thread to wait
 * is unreasonable, as it will tie up the main thread.  Thus, this menu spawns a background loader
 * thread using objects of the class SavedGamePreviewLoader.
 */

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
private:
	MenuStateLoadGame &menu;
	shared_ptr<string> fileName;		// read and modify with mutex locked
	bool seeJaneRun;
	Mutex mutex;

public:
	/** Primary constructor.  Creation of this object will automatically start the worker thread. */
	SavedGamePreviewLoader(MenuStateLoadGame &menu);

	/** Tells this SavedGamePreviewLoader to stop executing and prepare to be deleted. */
	void goAway();

	/**
	 * Instructs the SavedGamePreviewLoader to load the specified saved game file in its background
	 * worker thread.  When the load is completed, the setGameInfo() method of the MenuStateLoadGame
	 * object will be called, unless a subsequent call to load() is made prior to completion of
	 * loading the original saved game file (in which case, the original saved game info is
	 * discarded and never reported).
	 */
	void load(const string &fileName);

protected:
	/** Main thread loop (called by Thread). */
	virtual void execute();

private:
	/** Called only by private thread: Called to load a preview of the specified saved game file. */
	void _loadInternal(const shared_ptr<string> &fileName);
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
	shared_ptr<const XmlNode> savedGame;
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
	void setGameInfo(const string &fileName, shared_ptr<const XmlNode> &root, const string &err);

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
