// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATESCENARIO_H_
#define _GLEST_GAME_MENUSTATESCENARIO_H_

#include "main_menu.h"
#include "compound_widgets.h"

namespace Glest { namespace Menu {
using namespace Widgets;

// ===============================
// 	class MenuStateScenario
// ===============================

class MenuStateScenario: public MenuState {
private:
	WRAPPED_ENUM( Transition, RETURN, PLAY );
	WRAPPED_ENUM( Difficulty,
        VERY_EASY,
        EASY,
        MEDIUM,
        HARD,
        VERY_HARD,
        INSANE
    );

private:
	Transition			m_targetTansition;

	Button::Ptr			m_returnButton, 
						m_playNowButton;

	DropList::Ptr		m_categoryList,
						m_scenarioList;

	StaticText::Ptr		m_infoLabel;

	MessageDialog::Ptr	m_messageDialog;

    vector<string> categories;
	vector<string> scenarioFiles;
    ScenarioInfo scenarioInfo;


	void onButtonClick(Button::Ptr);
	void onCategoryChanged(ListBase::Ptr);
	void onScenarioChanged(ListBase::Ptr);

	void onConfirmReturn(MessageDialog::Ptr);

	void updateConfig();

public:
	MenuStateScenario(Program &program, MainMenu *mainMenu);

	void update();

	void launchGame();
	void setScenario(int i);
	int getScenarioCount() const	{ return scenarioFiles.size(); }

	MenuStates getIndex() const { return MenuStates::SCENARIO; }

private:
	void updateScenarioList(const string &category, bool selectDefault = false);
};


}}//end namespace

#endif
