// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "program.h"

#include "sound.h"
#include "renderer.h"
#include "config.h"
#include "game.h"
#include "main_menu.h"
#include "intro.h"
#include "world.h"
#include "main.h"
#include "sound_renderer.h"
#include "logger.h"
#include "profiler.h"
#include "core_data.h"
#include "metrics.h"
#include "network_manager.h"
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"

#include "leak_dumper.h"

#include <CEGUI/CEGUIDefaultResourceProvider.h>
#include "CEGUI/RendererModules/OpenGL/CEGUIOpenGLRenderer.h"

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;


namespace Glest { namespace Game {

// =====================================================
// 	class Program::CrashProgramState
// =====================================================

Program::CrashProgramState::CrashProgramState(Program &program, const exception *e) :
		ProgramState(program) {
	msgBox.init("", "Exit");
	if(e) {
		fprintf(stderr, "%s\n", e->what());
		msgBox.setText(string("Exception: ") + e->what());
	} else {
		msgBox.setText("Glest Advanced Engine has crashed.  Please help us improve GAE by emailing "
				" the file gae-crash.txt to " + gaeMailString + ".");
	}
	mouse2dAnim = mouseY = mouseX = 0;
	this->e = e;
}

void Program::CrashProgramState::render() {
	Renderer &renderer= Renderer::getInstance();
	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderMessageBox(&msgBox);
	renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
	renderer.swapBuffers();
}

void Program::CrashProgramState::mouseDownLeft(int x, int y) {
	if(msgBox.mouseClick(x,y)) {
		program.exit();
	}
}

void Program::CrashProgramState::mouseMove(int x, int y, const MouseState &mouseState) {
	mouseX = x;
	mouseY = y;
	msgBox.mouseMove(x, y);
}

void Program::CrashProgramState::update() {
	mouse2dAnim = (mouse2dAnim +1) % Renderer::maxMouse2dAnim;
}

// =====================================================
// 	class Program
// =====================================================

const int Program::maxTimes = 5;
Program *Program::singleton = NULL;

// ===================== PUBLIC ========================

Program::Program(Config &config, int argc, char** argv) :
		renderTimer(config.getRenderFpsMax(), 1),
		tickTimer(1.f, maxTimes, -1),
		updateTimer(config.getGsWorldUpdateFps(), maxTimes, 2),
		updateCameraTimer(GameConstants::cameraFps, maxTimes, 10),
		programState(NULL),
		crashed(false),
		keymap(getInput(), "keymap.ini") {

	//set video mode
	setDisplaySettings();

	//window
	setText("Glest Advanced Engine");
	setStyle(config.getDisplayWindowed() ? wsWindowedFixed: wsFullscreen);
	setPos(0, 0);
	setSize(config.getDisplayWidth(), config.getDisplayHeight());
	create();

	//log start
	Logger &logger= Logger::getInstance();
	//logger.setFile("glest.log");
	logger.clear();
	Logger::getServerLog().clear();
	Logger::getClientLog().clear();
	Logger::getErrorLog().clear();

	//lang
	Lang &lang= Lang::getInstance();
	lang.setLocale(config.getUiLocale());

	//render
	Renderer &renderer= Renderer::getInstance();

	initGl(config.getRenderColorBits(), config.getRenderDepthBits(), config.getRenderStencilBits());
	makeCurrentGl();

	//coreData, needs renderer, but must load before renderer init
	CoreData &coreData= CoreData::getInstance();
	coreData.load();

	//init renderer (load global textures)
	renderer.init();

	//sound
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.init(this);

	keymap.save("keymap.ini");

	loadGui();

	// startup and immediately host a game
	if(argc == 2 && string(argv[1]) == "-server") {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateNewGame(*this, mainMenu, true));

	// startup and immediately connect to server
	} else if(argc == 3 && string(argv[1]) == "-client") {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateJoinGame(*this, mainMenu, true, Ip(argv[2])));

	// normal startup
	} else {
		setState(new Intro(*this));
	}

	singleton = this;
}

Program::~Program() {
	singleton = NULL;

	if(programState) {
		delete programState;
	}

	Renderer::getInstance().end();

	//restore video mode
	restoreDisplaySettings();

	//delete guiRenderer;
	//delete guiSystem;
}

void Program::loop() {
	while(handleEvent()) {
		size_t sleepTime = renderTimer.timeToWait();

		sleepTime = sleepTime < updateCameraTimer.timeToWait() ? sleepTime : updateCameraTimer.timeToWait();
		sleepTime = sleepTime < updateTimer.timeToWait() ? sleepTime : updateTimer.timeToWait();
		sleepTime = sleepTime < tickTimer.timeToWait() ? sleepTime : tickTimer.timeToWait();

		if(sleepTime) {
			Shared::Platform::sleep(sleepTime);
		}

		//render
		while(renderTimer.isTime()){
			programState->render();
		}

		//update camera
		while(updateCameraTimer.isTime()){
			programState->updateCamera();
		}

		//update world
		while(updateTimer.isTime()){
			GraphicComponent::update();
			programState->update();
			SoundRenderer::getInstance().update();
			NetworkManager::getInstance().update();
		}

		//tick timer
		while(tickTimer.isTime()){
			programState->tick();
		}
	}
}

void Program::eventResize(SizeState sizeState) {

	switch(sizeState){
	case ssMinimized:
		//restoreVideoMode();
		break;
	case ssMaximized:
	case ssRestored:
		//setDisplaySettings();
		//renderer.reloadResources();
		break;
	}
}

// ==================== misc ====================

void Program::setState(ProgramState *programState){

	if(programState) {
		delete this->programState;
	}

	this->programState= programState;
	programState->load();
	programState->init();

	renderTimer.reset();
	updateTimer.reset();
	updateCameraTimer.reset();
	tickTimer.reset();
}

void Program::exit() {
	destroy();
}

void Program::resetTimers() {
	renderTimer.reset();
	tickTimer.reset();
	updateTimer.reset();
	updateCameraTimer.reset();
}

// ==================== PRIVATE ====================

void Program::loadGui() {
	// TODO: move to gui when it's not as reliant on game
	const Metrics &metrics = Metrics::getInstance();

	using namespace CEGUI;

	try
	{
		guiRenderer = &CEGUI::OpenGLRenderer::create(CEGUI::Size(metrics.getScreenW(), metrics.getScreenH()));//new CEGUI::OpenGLRenderer(CEGUI::Size(metrics.getScreenW(), metrics.getScreenH()), 0);	
		//guiSystem = new CEGUI::System(guiRenderer);
		CEGUI::System::create(*guiRenderer);
		
		CEGUI::String skin = "TaharezLook"; // TODO: replace this with a value from config

		// Specify Resource Groups
		DefaultResourceProvider* rp = static_cast<DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());
		
		rp->setResourceGroupDirectory("skins", "./data/gui/skins/" + skin + "/"); // schemes, imagesets, looknfeels
		/*rp->setResourceGroupDirectory("schemes", "./data/gui/skins/" + skin + "/");
		rp->setResourceGroupDirectory("imagesets", "./data/gui/skins/" + skin + "/");
		rp->setResourceGroupDirectory("looknfeels", "./data/gui/skins/" + skin + "/");*/

		rp->setResourceGroupDirectory("fonts", "./data/gui/fonts/");
		rp->setResourceGroupDirectory("layouts", "./data/gui/layouts/");
		rp->setResourceGroupDirectory("lua_scripts", "./data/gui/scripts/");

		// Assign DefaultResourceGroups
		Imageset::setDefaultResourceGroup("skins");
		WidgetLookManager::setDefaultResourceGroup("skins");
		Scheme::setDefaultResourceGroup("skins");
		CEGUI::Font::setDefaultResourceGroup("fonts");
		CEGUI::WindowManager::setDefaultResourceGroup("layouts");
		CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");

		// Load basic files and specify defaults
		SchemeManager::getSingleton().create(skin + ".scheme");
		//Imageset taharezImages = ImagesetManager::getSingleton().create(skin, skin + ".imageset");
		System::getSingleton().setDefaultMouseCursor(skin, "MouseArrow");//&taharezImages.getImage("MouseArrow"));
		CEGUI::FontManager::getSingleton().create("Commonwealth-10.font");
		WidgetLookManager::getSingleton().parseLookNFeelSpecification(skin + ".looknfeel");
	}
	catch (CEGUI::Exception& e)
	{
		fprintf(stderr,"CEGUI Exception occured - Game: %s", e.getMessage().c_str());
	}
}

void Program::setDisplaySettings(){

	Config &config= Config::getInstance();
	// bool multisamplingSupported = isGlExtensionSupported("WGL_ARB_multisample");

	if(!config.getDisplayWindowed()){

		int freq= config.getDisplayRefreshFrequency();
		int colorBits= config.getRenderColorBits();
		int screenWidth= config.getDisplayWidth();
		int screenHeight= config.getDisplayHeight();

		if(!(changeVideoMode(screenWidth, screenHeight, colorBits, freq) ||
			changeVideoMode(screenWidth, screenHeight, colorBits, 0)))
		{
			throw runtime_error(
				"Error setting video mode: " +
				intToStr(screenWidth) + "x" + intToStr(screenHeight) + "x" + intToStr(colorBits));
		}
	}
}

void Program::restoreDisplaySettings(){
	Config &config= Config::getInstance();

	if(!config.getDisplayWindowed()){
		restoreVideoMode();
	}
}

void Program::crash(const exception *e) {
	// if we've already crashed then we just try to exit
	if(!crashed) {
		crashed = true;

		if(programState) {
			delete programState;
		}

		programState = new CrashProgramState(*this, e);
		loop();
	} else {
		exit();
	}
}

}}//end namespace
