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

#include "pch.h"
#include "game_manager.h"

#include "game.h"
#include "messenger.h"
#include "xml_parser.h"

using Shared::Xml::XmlNode;

namespace Game {

GameManager *GameManager::singleton = NULL;

GameManager::GameManager(Program &program)
		: program(program)
		, game()
		, gs()
		, messenger(new LocalMessenger())
		, savedGame()
		, stats(gs) {
	assert(!singleton);
	singleton = this;
}

GameManager::~GameManager() {
	assert(singleton);
	singleton = NULL;
}

void GameManager::freeSavedGameData() {
	savedGame.reset();
}

} // end namespace
