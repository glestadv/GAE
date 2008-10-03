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

#ifdef USE_PCH
	#include "pch.h"
#endif

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

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

// =====================================================
// 	class Program
// =====================================================

namespace Glest{ namespace Game{

const int Program::maxTimes= 10;

// ===================== PUBLIC ========================

Program::Program(Config &config, int argc, char** argv) :
		renderTimer(config.getMaxFPS(), 1),
		tickTimer(1.f, maxTimes),
		updateTimer(config.getWorldUpdateFPS(), maxTimes),
		updateCameraTimer(GameConstants::cameraFps, maxTimes) {

	programState = NULL;

    //set video mode
	setDisplaySettings();

	//window
	setText("Glest");
	setStyle(config.getWindowed()? wsWindowedFixed: wsFullscreen);
	setPos(0, 0);
	setSize(config.getScreenWidth(), config.getScreenHeight());
	create();

    //log start
	Logger &logger= Logger::getInstance();
	logger.setFile("glest.log");
	logger.clear();

	//lang
	Lang &lang= Lang::getInstance();
	lang.load("data/lang/"+ config.getLang());

	//render
	Renderer &renderer= Renderer::getInstance();

	initGl(config.getColorBits(), config.getDepthBits(), config.getStencilBits());
	makeCurrentGl();

	//coreData, needs renderer, but must load before renderer init
	CoreData &coreData= CoreData::getInstance();
    coreData.load();

	//init renderer (load global textures)
	renderer.init();

	//sound
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.init(this);

	// startup and immediately host a game
	if(argc == 2 && string(argv[1]) == "-server") {
		MainMenu* mainMenu = new MainMenu(this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateNewGame(this, mainMenu, true));

	// startup and immediately connect to server
	} else if(argc == 3 && string(argv[1]) == "-client") {
		MainMenu* mainMenu = new MainMenu(this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateJoinGame(this, mainMenu, true, Ip(argv[2])));

	// normal startup
	} else {
		setState(new Intro(this));
	}
}

Program::~Program(){
	if(programState) {
		delete programState;
	}

	Renderer::getInstance().end();

	//restore video mode
	restoreDisplaySettings();
}

void Program::loop(){
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

void Program::exit(){
	destroy();
}

// ==================== PRIVATE ====================

void Program::setDisplaySettings(){

	Config &config= Config::getInstance();

	if(!config.getWindowed()){

		int freq= config.getRefreshFrequency();
		int colorBits= config.getColorBits();
		int screenWidth= config.getScreenWidth();
		int screenHeight= config.getScreenHeight();

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

	if(!config.getWindowed()){
		restoreVideoMode();
	}
}

}}//end namespace
