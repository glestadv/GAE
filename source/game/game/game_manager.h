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

namespace Shared { namespace Xml {
	class XmlNode;
}}

namespace Game {


class Program;
class Messenger;
class GameSettings;
class Game;

class GameManager {
private:
	Program &program;
	shared_ptr<Game> game;
	GameSettings gs;
	shared_ptr<Messenger> messenger;
	shared_ptr<XmlNode> savedGame;
	Stats stats;

	static GameManager *singleton;

public:
	GameManager(Program &program);
	~GameManager();
	static GameManager &getInstance()	{assert(singleton); return *singleton;}

	Program &getProgram()				{return program;}
	Game &getGame()						{assert(game.get()); return *game;}
	GameSettings &getGameSettings()		{return gs;}
	Messenger &getMessenger()			{assert(messenger.get()); return *messenger;}
	XmlNode *getSavedGame()				{return savedGame.get();}
	Stats &getStats()					{return stats;}
	bool hasGame() const				{return game.get();}

	void freeSavedGameData();
};

} // end namespace

#endif // _GAME_GAMEMANAGER_H_