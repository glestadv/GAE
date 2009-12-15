// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//							Nathan Turner <hailstone>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_root.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"
#include "menu_state_scenario.h"
#include "menu_state_load_game.h"
#include "menu_state_options.h"
#include "menu_state_about.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "auto_test.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program &program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	labelVersion.init(520, 460);
	labelVersion.setText("Advanced Engine " + gaeVersionString);

	// end network interface
	NetworkManager::getInstance().end();
}

void MenuStateRoot::init() {
	// Setup CEGUI Window
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();
	CEGUI::Window* myRoot = wmgr.loadWindowLayout("menu_state_root.layout");
	CEGUI::System::getSingleton().setGUISheet(myRoot);

	btnNewGame = wmgr.getWindow("new_game");
	btnJoinGame = wmgr.getWindow("join_game");
	btnScenario = wmgr.getWindow("scenario");
	btnLoadGame = wmgr.getWindow("load_game");
	btnOptions = wmgr.getWindow("options");
	btnAbout = wmgr.getWindow("about");
	btnExit = wmgr.getWindow("exit");

	//set text
	Lang &lang= Lang::getInstance();
	btnNewGame->setText(lang.get("NewGame"));
	btnJoinGame->setText(lang.get("JoinGame"));
	btnScenario->setText(lang.get("Scenario"));
	btnLoadGame->setText(lang.get("LoadGame"));
	btnOptions->setText(lang.get("Options"));
	btnAbout->setText(lang.get("About"));
	btnExit->setText(lang.get("Exit"));

	//setup slots
	registerButtonEvent(btnNewGame);
	registerButtonEvent(btnJoinGame);
	registerButtonEvent(btnScenario);
	registerButtonEvent(btnLoadGame);
	registerButtonEvent(btnOptions);
	registerButtonEvent(btnAbout);
	registerButtonEvent(btnExit);
}

void MenuStateRoot::registerButtonEvent(CEGUI::Window *button) {
	button->subscribeEvent(CEGUI::PushButton::EventClicked, 
		CEGUI::Event::Subscriber(&MenuStateRoot::handleButtonClick, this));
}

//Events
bool MenuStateRoot::handleButtonClick(const CEGUI::EventArgs& ea) {
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	CoreData &coreData = CoreData::getInstance();

	// find the calling component
	const CEGUI::Window *window = static_cast<const CEGUI::WindowEventArgs&>(ea).window;

	if (window == btnExit) {
		soundRenderer.playFx(coreData.getClickSoundA());
		program.exit();
		
		return true;
	}

	soundRenderer.playFx(coreData.getClickSoundB());
	
	if (window == btnNewGame) {
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
	} else if (window == btnJoinGame) {
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	} else if (window == btnScenario) {
		mainMenu->setState(new MenuStateScenario(program, mainMenu));
	} else if (window == btnLoadGame) {
		mainMenu->setState(new MenuStateLoadGame(program, mainMenu));
	} else if (window == btnOptions) {
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	} else if (window == btnAbout) {
		mainMenu->setState(new MenuStateAbout(program, mainMenu));
	}

	return true;
}

void MenuStateRoot::mouseClick(int x, int y, MouseButton mouseButton){

}

void MenuStateRoot::mouseMove(int x, int y, const MouseState &ms){

}

void MenuStateRoot::render(){
	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	int w= 300;
	int h= 150;

	renderer.renderTextureQuad(
		(metrics.getVirtualW()-w)/2, 495-h/2, w, h,
		coreData.getLogoTexture(), GraphicComponent::getFade());
	
	renderer.renderLabel(&labelVersion);
}

void MenuStateRoot::update(){
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}
}

}}//end namespace
