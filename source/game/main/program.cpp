// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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


using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;


namespace Game {

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
				" the file gae-crash.txt to " + getGaeMailString()
				+ " or reporting your bug at http://bugs.codemonger.org.");
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
		updateTimer(static_cast<float>(config.getGsWorldUpdateFps()), maxTimes, 2),
		updateCameraTimer(static_cast<float>(GameConstants::cameraFps), maxTimes, 10),
		programState(NULL),
		preCrashState(NULL),
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

	//lang
	Lang &lang= Lang::getInstance();
	lang.load("data/lang/" + config.getUiLang());

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

	// startup and immediately host a game
	if(argc == 2 && string(argv[1]) == "-server") {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateNewGame(*this, mainMenu, true));

	// startup and immediately connect to server
	} else if(argc == 3 && string(argv[1]) == "-client") {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateJoinGame(*this, mainMenu, true, IpAddress(argv[2])));

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

	if(preCrashState) {
		delete preCrashState;
	}

	Renderer::getInstance().end();

	//restore video mode
	restoreDisplaySettings();
}

void Program::loop() {
	while(handleEvent()) {
		// determine the soonest time that any of the timers are due.
		int64 nextExec = renderTimer.getNextExecution();
		if(nextExec > updateCameraTimer.getNextExecution()) {
			nextExec = updateCameraTimer.getNextExecution();
		}
		if(nextExec > updateTimer.getNextExecution()) {
			nextExec = updateTimer.getNextExecution();
		}
		if(nextExec > tickTimer.getNextExecution()) {
			nextExec = tickTimer.getNextExecution();
		}

		// If the difference is positive, sleep for that amount of time.
		int sleepTime = static_cast<int>((nextExec - Chrono::getCurMicros()) / 1000);
		if(sleepTime > 0) {
			Shared::Platform::sleep(sleepTime / 1000);
		}

		//render
		while(renderTimer.isTime()) {
			programState->render();
		}
	
		//update camera
		while(updateCameraTimer.isTime()) {
			programState->updateCamera();
		}

		//update world
		while(updateTimer.isTime()) {
			GraphicComponent::update();
//			NetworkManager::getInstance().beginUpdate();
			programState->update();
			SoundRenderer::getInstance().update();
		}

		//tick timer
		while(tickTimer.isTime()) {
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
				Conversion::toStr(screenWidth) + "x" + Conversion::toStr(screenHeight) + "x" + Conversion::toStr(colorBits));
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
	if(!preCrashState) {
		preCrashState = programState;
		programState = new CrashProgramState(*this, e);

		loop();
	} else {
		exit();
	}
}

} // end namespace
