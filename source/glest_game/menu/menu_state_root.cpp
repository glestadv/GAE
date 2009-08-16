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
class GUIConsole : public FrameWindow
{
public:
    GUIConsole() 
		: FrameWindow(Core::Rectangle(50, 50, 400, 400), 0, "Console")
		, panel_(Core::Rectangle(10, 10, 20, 20), 0)
		, btnNewGame(0, "New Game")
		, btnJoinGame(0, "Join Game")
		, btnScenario(0, "Scenario")
		, btnLoadGame(0, "Load Game")
		, btnOptions(0, "Options")
		, btnExit(0, "Exit")
		, labelVersion(0, "What button is pressed?")
    {
        // Leave this out and the user will not be able to drag the window around by its frame
        enableMovement();

        // set the edit field's size
        /*editField_.setSize(Core::Vector2(220, 30));

        // add some dummy strings to the list box
        for(int i = 0; i < 20; ++i)
        {
            listBox_.addString("testing" + Core::toString(i));
        }

        // set the "done" buttton's size
        doneButton_.setSize(Core::Vector2(55, 30));

        // wire up the signal/slot connections - pressing Return in the edit field should have
        // the same effect as pressing the done button
        doneButton_.pressed.connect(this, &GUIConsole::doneButtonPressed);
        editField_.returnPressed.connect(this, &GUIConsole::doneButtonPressed);

        // put the controls in the panel
        panel_
            .addChildWindow(&listBox_)
            .addChildWindow(&editField_)
            .addChildWindow(&doneButton_)
            ;

        // put the panel in the frame window
        setClientWindow(&panel_);

        // arrange the panel's children according to the default flow layouter
        panel_.arrangeChildren();*/

		/*btnNewGame.setSize(Core::Vector2(55, 30));
		btnJoinGame.setSize(Core::Vector2(55, 30));
		btnScenario.setSize(Core::Vector2(55, 30));*/
		
		labelVersion.setSize(Core::Vector2(350, 30));

		btnNewGame.pressed.connect(this, &GUIConsole::buttonPressed);
		btnJoinGame.pressed.connect(this, &GUIConsole::buttonPressed);
		btnScenario.pressed.connect(this, &GUIConsole::buttonPressed);

		panel_.setLayouter(new BoxLayouter(BoxLayouter::vertical, 5.0f, true));

		panel_
            .addChildWindow(&btnNewGame)
            .addChildWindow(&btnJoinGame)
            .addChildWindow(&btnScenario)
			.addChildWindow(&labelVersion)
            ;

		// put the panel in the frame window
		setClientWindow(&panel_);

		// arrange the panel's children according to the default flow layouter
        panel_.arrangeChildren();
    }


public:
    // this is the slot that responds to the button being pressed
    void buttonPressed()
    {
        /*// get the edit field's text, add it to the list box, then clear the edit field
        std::string str = editField_.text();
        if(str != "") listBox_.addString(str);
        editField_.setText("");
		*/

		if (btnNewGame.state() == Button::State::down) {
			labelVersion.setText("new game pressed");
		} else if (btnJoinGame.state() == Button::State::down) {
			labelVersion.setText("join game pressed");
		}  else if (btnScenario.state() == Button::State::down) {
			labelVersion.setText("scenario pressed");
		}
    }

private:
    // the controls
    Panel       panel_;
    /*EditField   editField_;
    ListBox     listBox_;
    Button      doneButton_;
	*/
	Button btnNewGame;
	Button btnJoinGame;
	Button btnScenario;
	Button btnLoadGame;
	Button btnOptions;
	Button btnExit;
	StaticText labelVersion;
};
//END NEWGUI

// =====================================================
// 	class MenuStateRoot
// =====================================================

//NEWGUI
// This has been made global for simplicity!
GUIConsole* console = 0;

MenuStateRoot::MenuStateRoot(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu, "root")
		, FrameWindow(Core::Rectangle(50, 50, 400, 400), 0, "Console")
		, panel_(Core::Rectangle(10, 10, 20, 20), 0)
		, btnNewGame(0, "")
		, btnJoinGame(0, "")
		, btnScenario(0, "")
		, btnLoadGame(0, "")
		, btnOptions(0, "")
		, btnAbout(0, "")
		, btnExit(0, "")
		, labelVersion(0, "Advanced Engine " + gaeVersionString) {
	Lang &lang= Lang::getInstance();

	/*buttonNewGame.init(425, 370, 150);
    buttonJoinGame.init(425, 330, 150);
    buttonScenario.init(425, 290, 150);
	buttonLoadGame.init(425, 250, 150);
    buttonOptions.init(425, 210, 150);
    buttonAbout.init(425, 170, 150);
    buttonExit.init(425, 130, 150);
	labelVersion.init(520, 460);

	buttonNewGame.setText(lang.get("NewGame"));
	buttonJoinGame.setText(lang.get("JoinGame"));
	buttonScenario.setText(lang.get("Scenario"));
	buttonLoadGame.setText(lang.get("LoadGame"));
	buttonOptions.setText(lang.get("Options"));
	buttonAbout.setText(lang.get("About"));
	buttonExit.setText(lang.get("Exit"));
	labelVersion.setText("Advanced Engine " + gaeVersionString);*/

	// end network interface
	NetworkManager::getInstance().end();

	//NEWGUI
	// set text
	btnNewGame.setText(lang.get("NewGame"));
	btnJoinGame.setText(lang.get("JoinGame"));
	btnScenario.setText(lang.get("Scenario"));
	btnLoadGame.setText(lang.get("LoadGame"));
	btnOptions.setText(lang.get("Options"));
	btnAbout.setText(lang.get("About"));
	btnExit.setText(lang.get("Exit"));

	btnNewGame.setSize(Core::Vector2(256, 32));
	btnJoinGame.setSize(Core::Vector2(256, 32));
	btnScenario.setSize(Core::Vector2(256, 32));
	btnLoadGame.setSize(Core::Vector2(256, 32));
	btnOptions.setSize(Core::Vector2(256, 32));
	btnAbout.setSize(Core::Vector2(256, 32));
	btnExit.setSize(Core::Vector2(256, 32));

	btnNewGame.loadAppearance("data/gui/default/buttons.xml", "standard");
	btnJoinGame.loadAppearance("data/gui/default/buttons.xml", "standard");
	btnScenario.loadAppearance("data/gui/default/buttons.xml", "standard");
	btnLoadGame.loadAppearance("data/gui/default/buttons.xml", "standard");
	btnOptions.loadAppearance("data/gui/default/buttons.xml", "standard");
	btnAbout.loadAppearance("data/gui/default/buttons.xml", "standard");
	btnExit.loadAppearance("data/gui/default/buttons.xml", "standard");

	labelVersion.setSize(Core::Vector2(350, 30));

	// connect slots
	btnNewGame.pressed.connect(this, &MenuStateRoot::buttonPressed);
	/*btnJoinGame.pressed.connect(this, &MenuStateRoot::buttonPressed);
	btnScenario.pressed.connect(this, &MenuStateRoot::buttonPressed);
	btnLoadGame.pressed.connect(this, &MenuStateRoot::buttonPressed);
	btnOptions.pressed.connect(this, &MenuStateRoot::buttonPressed);
	btnAbout.pressed.connect(this, &MenuStateRoot::buttonPressed);
	btnExit.pressed.connect(this, &MenuStateRoot::buttonPressed);*/

	// create the box layouter
	//panel_.setLayouter(new BoxLayouter(BoxLayouter::vertical, 5.0f, true));

	// add the components to the panel
	panel_
		.addChildWindow(&btnNewGame)
		.addChildWindow(&btnJoinGame)
		.addChildWindow(&btnScenario)
		.addChildWindow(&btnLoadGame)
		.addChildWindow(&btnOptions)
		.addChildWindow(&btnAbout)
		.addChildWindow(&btnExit)
		.addChildWindow(&labelVersion)
		;

	// put the panel in the frame window
	setClientWindow(&panel_);

	// arrange the panel's children according to the default flow layouter
	panel_.arrangeChildren();

	
	//console = new GUIConsole;
    //WindowManager::instance().addWindow(console);
	WindowManager::instance().addWindow(this);
	//END NEWGUI
}

//NEWGUI
MenuStateRoot::~MenuStateRoot() {
	WindowManager::instance().removeWindow(this);
    //delete console;
}

void MenuStateRoot::buttonPressed() {
	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

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
		mainMenu->setState(new MenuStateNewGame(program, mainMenu)); // ERROR: something not closing right or something
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
//END NEWGUI

void MenuStateRoot::mouseClick(int x, int y, MouseButton mouseButton){

	/*CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonNewGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }
	else if(buttonJoinGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
    }
	else if(buttonScenario.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateScenario(program, mainMenu));
    }
    else if(buttonOptions.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
    }
	else if(buttonLoadGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateLoadGame(program, mainMenu));
	}
    else if(buttonAbout.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateAbout(program, mainMenu));
    }
    else if(buttonExit.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		program.exit();
    }*/
}

void MenuStateRoot::mouseMove(int x, int y, const MouseState &ms){
	/*buttonNewGame.mouseMove(x, y);
    buttonJoinGame.mouseMove(x, y);
    buttonScenario.mouseMove(x, y);
	buttonLoadGame.mouseMove(x, y);
    buttonOptions.mouseMove(x, y);
    buttonAbout.mouseMove(x, y);
    buttonExit.mouseMove(x,y);*/
}

void MenuStateRoot::render(){
	/*Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	int w= 300;
	int h= 150;

	renderer.renderTextureQuad(
		(metrics.getVirtualW()-w)/2, 495-h/2, w, h,
		coreData.getLogoTexture(), GraphicComponent::getFade());
	renderer.renderButton(&buttonNewGame);
	renderer.renderButton(&buttonJoinGame);
	renderer.renderButton(&buttonScenario);
	renderer.renderButton(&buttonLoadGame);
	renderer.renderButton(&buttonOptions);
	renderer.renderButton(&buttonAbout);
	renderer.renderButton(&buttonExit);
	renderer.renderLabel(&labelVersion);*/
}

void MenuStateRoot::update(){
	//TOOD: add AutoTest to config
	/*if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}*/
}

}}//end namespace
