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

//NEWGUI
#include "glgooey/BoxLayouter.h"
using namespace Gooey;

namespace Glest{ namespace Game{

//NEWGUI
class GUIConsole : public Panel//FrameWindow
{
public:
    GUIConsole(Program &program, MainMenu *mainMenu)
		: //FrameWindow(Core::Rectangle(0, 0, 400, 400), 0, "Console")
	Panel(Core::Rectangle(50, 50, 400, 400), 0, "Console")
		, panel_(Core::Rectangle(10, 10, 20, 20), 0)
		, btnNewGame(0, "")
		, btnJoinGame(0, "")
		, btnScenario(0, "")
		, btnLoadGame(0, "")
		, btnOptions(0, "")
		, btnAbout(0, "")
		, btnExit(0, "")
		, labelVersion(0, "Advanced Engine " + gaeVersionString)
		, mainMenu(mainMenu)
		, program(program)
    {
        // Leave this out and the user will not be able to drag the window around by its frame
        //enableMovement();

		// init
		Lang &lang= Lang::getInstance();

		initButton(&btnNewGame, lang.get("NewGame"));
		initButton(&btnJoinGame,lang.get("JoinGame"));
		initButton(&btnScenario,lang.get("Scenario"));
		initButton(&btnLoadGame,lang.get("LoadGame"));
		initButton(&btnOptions, lang.get("Options"));
		initButton(&btnAbout, lang.get("About"));
		initButton(&btnExit, lang.get("Exit"));

		// Version text
		labelVersion.setSize(Core::Vector2(350, 30));
		this->addChildWindow(&labelVersion);

		// create the box layouter
		//panel_.setLayouter(new BoxLayouter(BoxLayouter::vertical, 5.0f, true));

		// put the panel in the frame window
		//setClientWindow(&panel_);

		// arrange the panel's children according to the default flow layouter
		this->arrangeChildren();
    }

	~GUIConsole() {
		// need this otherwise will cause problems when going back to this state
		// can also be put in the state destructor
		WindowManager::instance().releaseMouseCapture();
	}
private:
	void initButton(Button *b, const std::string text) {
		// size, text, appearance, slot, add to panel
		b->setSize(Core::Vector2(256, 32));
		b->setText(text);
		b->loadAppearance("data/gui/default/buttons.xml", "standard");
		b->pressed.connect(this, &GUIConsole::buttonPressed);
		this->addChildWindow(b);
	}

public:
    // this is the slot that responds to the button being pressed
    void buttonPressed()
    {
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

private:
    // the controls
    Panel  panel_;
	Button btnNewGame;
	Button btnJoinGame;
	Button btnScenario;
	Button btnLoadGame;
	Button btnOptions;
	Button btnAbout;
	Button btnExit;
	StaticText labelVersion;
	MainMenu *mainMenu;
	Program &program;
};
//END NEWGUI

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu, "root") {

	// end network interface
	NetworkManager::getInstance().end();

	//NEWGUI
	//might be able to move the stuff into this class instead of having it separate
	menuFrame = new GUIConsole(program, mainMenu);
    WindowManager::instance().addWindow(menuFrame);
	//END NEWGUI
}

//NEWGUI
MenuStateRoot::~MenuStateRoot() {
	WindowManager::instance().removeWindow(menuFrame);
    delete menuFrame;
}
//END NEWGUI

void MenuStateRoot::render(){
	
}

void MenuStateRoot::update(){
	//TOOD: add AutoTest to config
	/*if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}*/
}

}}//end namespace
