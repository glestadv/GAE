// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEROOT_H_
#define _GLEST_GAME_MENUSTATEROOT_H_

#include "main_menu.h"

namespace Glest { namespace Game {

// ===============================
// 	class MenuStateRoot  
// ===============================

class MenuStateRoot: public MenuState {
private:
	GraphicLabel labelVersion;

	CEGUI::Window* btnNewGame;
	CEGUI::Window* btnJoinGame;
	CEGUI::Window* btnScenario;
	CEGUI::Window* btnLoadGame;
	CEGUI::Window* btnOptions;
	CEGUI::Window* btnAbout;
	CEGUI::Window* btnExit;

private:
	MenuStateRoot(const MenuStateRoot &);
	const MenuStateRoot &operator =(const MenuStateRoot &);

	void registerButtonEvent(CEGUI::Window *button);
	bool MenuStateRoot::handleButtonClick(const CEGUI::EventArgs& ea);

public:
	MenuStateRoot(Program &program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState &mouseState);
	void init();
	void render();
	void update();
};


}}//end namespace

#endif
