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

class GUIConsole;

class MenuStateRoot: public MenuState, public Gooey::Panel {
private:
	Gooey::Button btnNewGame;
	Gooey::Button btnJoinGame;
	Gooey::Button btnScenario;
	Gooey::Button btnLoadGame;
	Gooey::Button btnOptions;
	Gooey::Button btnAbout;
	Gooey::Button btnExit;

	GraphicLabel labelVersion;

private:
	MenuStateRoot(const MenuStateRoot &);
	const MenuStateRoot &operator =(const MenuStateRoot &);

	void initButton(Gooey::Button *b, const std::string text);

public:
	MenuStateRoot(Program &program, MainMenu *mainMenu);
	~MenuStateRoot();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState &ms) {}
	void render();
	void update();

	//slots
	void buttonPressed();
};


}}//end namespace

#endif
