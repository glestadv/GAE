// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_GAMEMANAGER_H_
#define _GAME_GAMEMANAGER_H_

#include "util.h"
#include "stats.h"

namespace Game {


class Program;
class Messenger;
class GameSettings;
class XmlNode;
class Game;

class GameManager {
private:
	Program &program;
	shared_ptr<Game> game;
	GameSettings gs;
	shared_ptr<Messenger> messenger;
	shared_ptr<XmlNode> savedGame;
	Stats stats;

public:
	GameManager(Program &program);

	Program &getProgram()				{return program;}
	Game &getGame()						{assert(*game); return *game;}
	GameSettings &getGameSettings()		{return gs;}
	Messenger &getMessenger()			{assert(*messenger); return *messenger;}
	XmlNode *getSavedGame()				{return savedGame;}
	Stats &getStats()					{return stats;}
	bool hasGame() const				{return game.get();}


	void freeSavedGameData();
};

} // end namespace

#endif // _GAME_GAMEMANAGER_H_