// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//					   2009 Daniel Santos <daniel.santos@pobox.com>
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
#include "gui_program.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;


namespace Glest { namespace Game {

// =====================================================
// 	class GuiProgramState
// =====================================================

GuiProgramState::~GuiProgramState() {
}

void GuiProgramState::init() {}
void GuiProgramState::load() {}
void GuiProgramState::end() {}
void GuiProgramState::mouseDownLeft(int x, int y) {}
void GuiProgramState::mouseDownRight(int x, int y) {}
void GuiProgramState::mouseDownCenter(int x, int y) {}
void GuiProgramState::mouseUpLeft(int x, int y) {}
void GuiProgramState::mouseUpRight(int x, int y) {}
void GuiProgramState::mouseUpCenter(int x, int y) {}
void GuiProgramState::mouseDoubleClickLeft(int x, int y) {}
void GuiProgramState::mouseDoubleClickRight(int x, int y) {}
void GuiProgramState::mouseDoubleClickCenter(int x, int y) {}
void GuiProgramState::eventMouseWheel(int x, int y, int zDelta) {}
void GuiProgramState::mouseMove(int x, int y, const MouseState &mouseState) {}
void GuiProgramState::keyDown(const Key &key) {}
void GuiProgramState::keyUp(const Key &key) {}
void GuiProgramState::keyPress(char c) {}

// ===================== PUBLIC ========================

GuiProgram::GuiProgram(Program::LaunchType launchType, const string &ipAddress)
		: Program(launchType)
		, WindowGl(
			getConfig().getDisplayWindowed() ? wsWindowedFixed : wsFullscreen,
			0,
			0,
			getConfig().getDisplayWidth(),
			getConfig().getDisplayHeight(),
			getConfig().getDisplayRefreshFrequency(),
			getConfig().getRenderColorBits(),
			getConfig().getRenderDepthBits(),
			getConfig().getRenderStencilBits(),
			"Glest Advanced Engine")
		, FactoryRepository(
			getContext(),
			getConfig().getRenderGraphicsFactory(),
			getConfig().getSoundFactory())
		, programState(NULL)
		, preCrashState(NULL)
		, keymap(getInput(), "keymap.ini")
		, metrics()
		, renderer()
		, soundRenderer(getConfig(), this)
		, coreData(getConfig(), renderer)
		, renderTimer(getConfig().getRenderFpsMax(), 1)
		, tickTimer(1.f, maxTimes, -1)
		, updateTimer(static_cast<float>(getConfig().getGsWorldUpdateFps()), maxTimes, 2)
		, updateCameraTimer(static_cast<float>(GameConstants::cameraFps), maxTimes, 10) {

	//set video mode
	//setDisplaySettings(config.getDisplayWindowed() ? wsWindowedFixed: wsFullscreen);

	//window
	//setText("Glest Advanced Engine");
	//setStyle(config.getDisplayWindowed() ? wsWindowedFixed: wsFullscreen);
	//setPos(0, 0);
	//setSize(config.getDisplayWidth(), config.getDisplayHeight());
	//create();

	//initGl(config.getRenderColorBits(), config.getRenderDepthBits(), config.getRenderStencilBits());
	//makeCurrentGl();

	//init renderer (load global textures)
	renderer.init();

	keymap.save("keymap.ini");

	// startup and immediately host a game
	if(launchType == Program::START_OPTION_SERVER) {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateNewGame(*this, mainMenu, true));

	// startup and immediately connect to server
	} else if(launchType == Program::START_OPTION_CLIENT) {
		MainMenu* mainMenu = new MainMenu(*this);
		setState(mainMenu);
		mainMenu->setState(new MenuStateJoinGame(*this, mainMenu, true, IpAddress(ipAddress)));

	// normal startup
	} else {
		setState(new Intro(*this));
	}
}

GuiProgram::~GuiProgram() {
	if(programState) {
		delete programState;
	}

	if(preCrashState) {
		delete preCrashState;
	}

	theRenderer.end();

	//restore video mode
	restoreDisplaySettings();
}

/**
 * Main program loop.  This function will loop until Window::handleEvent() returns false.  In the
 * SDL implementation, this is when the SDL_QUIT event is sent to the application, for windows, it's
 * the WM_QUIT message.  This function effectively "runs" multiple sub-processes (in the logical
 * sense, they aren't operating system processes) by calling various member functions of the
 * programState object when their timers are due.
 *
 * Each iteration the status of each timer is checked to determine the soonest that any are due.  If
 * none are currently due, then it will enter a sleep state until one is due.  Then, each timer is
 * re-checked, and if due, they are run.
 */
int GuiProgram::main() {
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
			theSoundRenderer.update();
		}

		//tick timer
		while(tickTimer.isTime()) {
			programState->tick();
		}
	}
	return 0;
}


// ===================== Functions of Shared::Platform::Window ========================


void GuiProgram::eventMouseDown(int x, int y, MouseButton mouseButton) {
	const Metrics &metrics = getMetrics();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

	switch(mouseButton) {
	case mbLeft:
		programState->mouseDownLeft(vx, vy);
		break;
	case mbRight:
		programState->mouseDownRight(vx, vy);
		break;
	case mbCenter:
		programState->mouseDownCenter(vx, vy);
		break;
	default:
		break;
	}
}

void GuiProgram::eventMouseUp(int x, int y, MouseButton mouseButton) {
	const Metrics &metrics = getMetrics();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

	switch(mouseButton) {
	case mbLeft:
		programState->mouseUpLeft(vx, vy);
		break;
	case mbRight:
		programState->mouseUpRight(vx, vy);
		break;
	case mbCenter:
		programState->mouseUpCenter(vx, vy);
		break;
	default:
		break;
	}
}

void GuiProgram::eventMouseMove(int x, int y, const MouseState &ms) {
	const Metrics &metrics= getMetrics();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

	programState->mouseMove(vx, vy, ms);
}

void GuiProgram::eventMouseDoubleClick(int x, int y, MouseButton mouseButton) {
	const Metrics &metrics= getMetrics();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

	switch(mouseButton){
	case mbLeft:
		programState->mouseDoubleClickLeft(vx, vy);
		break;
	case mbRight:
		programState->mouseDoubleClickRight(vx, vy);
		break;
	case mbCenter:
		programState->mouseDoubleClickCenter(vx, vy);
		break;
	default:
		break;
	}
}

void GuiProgram::eventMouseWheel(int x, int y, int zDelta) {
	const Metrics &metrics= getMetrics();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

	programState->eventMouseWheel(vx, vy, zDelta);
}

// FIXME: using both left & right alt/control/shift at the same time will cause these to be
// incorrect on some platforms (not sure that anybody cares though).
void GuiProgram::eventKeyDown(const Key &key) {
	programState->keyDown(key);
}

void GuiProgram::eventKeyUp(const Key &key) {
	programState->keyUp(key);
}

void GuiProgram::eventKeyPress(char c) {
	programState->keyPress(c);
}

void GuiProgram::eventCreate() {}
void GuiProgram::eventDestroy() {}
void GuiProgram::eventResize() {}

void GuiProgram::eventResize(SizeState sizeState) {

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

void GuiProgram::eventActivate(bool activated) {
	if (!activated) {
		//minimize();
	}
}

void GuiProgram::eventMenu(int menuId) {}
void GuiProgram::eventClose() {}
void GuiProgram::eventPaint() {}
void GuiProgram::eventTimer(int timerId) {}

// ==================== misc ====================

void GuiProgram::setState(GuiProgramState *programState){

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

void GuiProgram::exit() {
	destroy();
}

void GuiProgram::resetTimers() {
	renderTimer.reset();
	tickTimer.reset();
	updateTimer.reset();
	updateCameraTimer.reset();
}

// ==================== PRIVATE ====================

void GuiProgram::setDisplaySettings() {

	Config &config = theConfig;
	// bool multisamplingSupported = isGlExtensionSupported("WGL_ARB_multisample");

	if (!config.getDisplayWindowed()) {

		int freq = config.getDisplayRefreshFrequency();
		int colorBits = config.getRenderColorBits();
		int screenWidth = config.getDisplayWidth();
		int screenHeight = config.getDisplayHeight();

		if (!(changeVideoMode(screenWidth, screenHeight, colorBits, freq)
				|| changeVideoMode(screenWidth, screenHeight, colorBits, 0))) {
			throw runtime_error("Error setting video mode: " + Conversion::toStr(screenWidth)
					+ "x" + Conversion::toStr(screenHeight) + "x" + Conversion::toStr(colorBits));
		}
	}
}

void GuiProgram::restoreDisplaySettings(){
	Config &config= theConfig;

	if(!config.getDisplayWindowed()){
		restoreVideoMode();
	}
}

void GuiProgram::crash(const exception *e) {
	// if we've already crashed then we just try to exit
	if(!preCrashState) {
		preCrashState = programState;
		programState = new CrashProgramState(*this, e);

		main();
	} else {
		exit();
	}
}

// =====================================================
// 	class CrashProgramState
// =====================================================

CrashProgramState::CrashProgramState(GuiProgram &program, const exception *e)
		: GuiProgramState(program)
		, msgBox()
		, mouseX(0)
		, mouseY(0)
		, mouse2dAnim(0)
		, e(e) {
	msgBox.init("", "Exit");
	if(e) {
		fprintf(stderr, "%s\n", e->what());
		msgBox.setText(string("Exception: ") + e->what());
	} else {
		msgBox.setText("Glest Advanced Engine has crashed.  Please help us improve GAE by emailing "
				" the file gae-crash.txt to " + getGaeMailString()
				+ " or reporting your bug at http://bugs.codemonger.org.");
	}
}

void CrashProgramState::render() {
	Renderer &renderer= theRenderer;
	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderMessageBox(&msgBox);
	renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);
	renderer.swapBuffers();
}

void CrashProgramState::mouseDownLeft(int x, int y) {
	if(msgBox.mouseClick(x,y)) {
		program.exit();
	}
}

void CrashProgramState::mouseMove(int x, int y, const MouseState &mouseState) {
	mouseX = x;
	mouseY = y;
	msgBox.mouseMove(x, y);
}

void CrashProgramState::update() {
	mouse2dAnim = (mouse2dAnim + 1) % Renderer::maxMouse2dAnim;
}

}} // end namespace
