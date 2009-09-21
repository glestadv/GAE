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

#include "leak_dumper.h"

using namespace Gooey;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu, "root")
		, Panel(/*Core::*/VRectangle(250, 250, 500, 500 /*800, 600*/), 0, "")
		, btnNewGame(0, "")
		, btnJoinGame(0, "")
		, btnScenario(0, "")
		, btnLoadGame(0, "")
		, btnOptions(0, "")
		, btnAbout(0, "")
		, btnExit(0, "") {

	// end network interface
	NetworkManager::getInstance().end();

	Lang &lang = Lang::getInstance();

	initButton(&btnNewGame, lang.get("NewGame"));
	initButton(&btnJoinGame,lang.get("JoinGame"));
	initButton(&btnScenario,lang.get("Scenario"));
	initButton(&btnLoadGame,lang.get("LoadGame"));
	initButton(&btnOptions, lang.get("Options"));
	initButton(&btnAbout, lang.get("About"));
	initButton(&btnExit, lang.get("Exit"));

	// arrange the panel's children according to the default flow layouter
	this->arrangeChildren();

    WindowManager::instance().addWindow(this);
	//WindowManager::instance().setMouseCapture(this);

	labelVersion.init(520, 460);
	labelVersion.setText("Advanced Engine " + gaeVersionString);
}

void MenuStateRoot::mouseClick(int x, int y, MouseButton mouseButton) {
	
	labelVersion.setText("X: " + intToStr(x) + ",Y: " + intToStr(y) + ",vX: " + 
		intToStr(Metrics::getInstance().toVirtualX(x)) + ",vY: " + intToStr(Metrics::getInstance().toVirtualY(y)));
}

MenuStateRoot::~MenuStateRoot() {
	//NOTE: must be done before removing the window
	//WindowManager::instance().releaseMouseCapture();
	WindowManager::instance().removeWindow(this);
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
	//TOOD: add AutoTest to config
	/*if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}*/
}

void MenuStateRoot::initButton(Button *b, const std::string text) {
	// size, text, appearance, slot, add to panel
	b->setSize(Core::Vector2(256, 32));
	b->setText(text);
	b->loadAppearance("data/gui/default/buttons.xml", "standard");
	b->pressed.connect(this, &MenuStateRoot::buttonPressed);
	this->addChildWindow(b);
}

// Slots
void MenuStateRoot::buttonPressed() {
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	// this can be done because we know a button is being pressed,
	// not just a mouse click.
	if ( btnExit.state() == Button::State::down ) {
		soundRenderer.playFx(coreData.getClickSoundA());
		program.exit();
	} else {
		soundRenderer.playFx(coreData.getClickSoundB());
	}

	// handle the other buttons
	if ( btnNewGame.state() == Button::State::down ) {
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
	} 
	else if ( btnJoinGame.state() == Button::State::down ) {
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
	} 
	else if ( btnScenario.state() == Button::State::down ) {
		mainMenu->setState(new MenuStateScenario(program, mainMenu));
	} 
	else if ( btnOptions.state() == Button::State::down ) {
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	} 
	else if ( btnLoadGame.state() == Button::State::down ) {
		mainMenu->setState(new MenuStateLoadGame(program, mainMenu));
	} 
	else if ( btnAbout.state() == Button::State::down ) {
		mainMenu->setState(new MenuStateAbout(program, mainMenu));
	}
}

}}//end namespace
