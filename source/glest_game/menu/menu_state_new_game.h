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

#ifndef _GLEST_GAME_MENUSTATENEWGAME_H_
#define _GLEST_GAME_MENUSTATENEWGAME_H_

#include "menu_state_start_game_base.h"

namespace Glest{ namespace Game{

// ===============================
// 	class PlayerSlot
// ===============================

class PlayerSlot : public Gooey::Panel {
	// Texts
	Gooey::StaticText txtPlayer;
	Gooey::StaticText txtNetStatus;

	// Combo boxes
	Gooey::ComboBox cmbControl;
	Gooey::ComboBox cmbFaction;
	Gooey::SpinBox spinnerTeam;

	/*
	//GraphicLabel labelPlayers[GameConstants::maxPlayers];
	//GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	*/
	static int humanIndex1, humanIndex2;

	void initComboBox(Gooey::ComboBox *cmb) {
		cmb->setSize(Gooey::Core::Vector2(156, 32));

		this->addChildWindow(cmb);
	}

public:
	PlayerSlot();

	void setControl(ControlType ct);
	ControlType getControl();
	int getTeam();
	int getFactionIndex();
	void reloadFactions(vector<std::string> factions);

public:
// Events
// - controlSelected

};

// ===============================
// 	class MenuStateNewGame
// ===============================

class MenuStateNewGame: public MenuStateStartGameBase, public Gooey::Panel {
private:
	/* Buttons
	//GraphicButton buttonReturn;
	//GraphicButton buttonPlayNow;
	*/
	Gooey::Button btnReturn;
	Gooey::Button btnPlayNow;

	/* File Locations */
	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;

	vector<PlayerSlot*> playerSlots;
	
	/* ComboBoxes */
	Gooey::ComboBox cmbMap;
	Gooey::ComboBox cmbTechTree;
	Gooey::ComboBox cmbTileset;
	
	/* CheckBoxes */
	Gooey::CheckBox chkRandomize;
	

	/* Label Slot Headings */
	Gooey::StaticText txtControlHeading;
	Gooey::StaticText txtFactionHeading;
	Gooey::StaticText txtTeamHeading;

	/* StaticText */
	Gooey::StaticText txtMap;
	Gooey::StaticText txtTechTree;
	Gooey::StaticText txtTileset;
	Gooey::StaticText txtNetwork;
	Gooey::StaticText txtRandomize;
	Gooey::StaticText txtMapInfo; //WARN: might not be multi-line
	
	MapInfo mapInfo;
	//GraphicMessageBox *msgBox;


public:
	MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots = false);
	~MenuStateNewGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState &mouseState);
	void render();
	void update();

private:
    void loadGameSettings(GameSettings *gameSettings);
	//void loadMapInfo(string file, MapInfo *mapInfo);
	//void reloadFactions();
	void updateControllers();
	bool isUnconnectedSlots();
	void updateNetworkSlots();

	void initButton(Gooey::Button *btn, const std::string &text);
	void initFileComboBox(
		Gooey::ComboBox *cmb, 
		const std::string &path, 
		vector<std::string> &files, 
		const std::string &errorMsg, 
		bool cutExtension = false);

	// Events
	// - button press
	// -- start game
	// -- return
	// - reload player slots

	void buttonClicked();
	void mapSelected();
	void techSelected();
};

}}//end namespace

#endif
