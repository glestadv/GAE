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
#include "main_menu.h"

#include "renderer.h"
#include "sound.h"
#include "config.h"
#include "program.h"
#include "game_util.h"
#include "game.h"
#include "platform_util.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "faction.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "menu_state_root.h"

#include "leak_dumper.h"


using namespace Shared::Sound;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Xml;

//NEWGUI
using namespace Gooey;

namespace Glest { namespace Game {

// =====================================================
//  class MainMenu
// =====================================================

// ===================== PUBLIC ========================

MainMenu::MainMenu(Program &program) : ProgramState(program) {
	mouseX = 100;
	mouseY = 100;

	state = NULL;
	//NEWGUI
	oldState = NULL;
	//this->program = program;

	fps = 0;
	lastFps = 0;
}

MainMenu::~MainMenu() {
	delete state;
	Renderer::getInstance().endMenu();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	soundRenderer.stopAllSounds();

	//NEWGUI
	WindowManager::instance().terminate();
}
//NEWGUI
bool isShiftPressed()
{
    return false;
}

bool isAltPressed()
{
    return false;
}

bool isCtrlPressed()
{
    return false;
}
//END NEWGUI

void MainMenu::init() {
	Renderer::getInstance().initMenu(this);
	//NEWGUI
	Gooey::WindowManager::instance().initialize("data/gui/fonts/DejaVuSans.ttf", isShiftPressed, isAltPressed, isCtrlPressed);

	Metrics m = Metrics::getInstance();
	WindowManager::instance().applicationResized(m.getScreenW(), m.getScreenH()); //needed otherwise doesn't display

	setState(new MenuStateRoot(program, this)); //moved from constructor so that the glgooey is setup when the menu state is constructed
	//END NEWGUI
}

//asynchronus render update
void MainMenu::render() {

	Config &config = Config::getInstance();
	Renderer &renderer = Renderer::getInstance();
	CoreData &coreData = CoreData::getInstance();

	fps++;

	renderer.clearBuffers();

	//3d
	renderer.reset3dMenu();
	renderer.clearZBuffer();
	renderer.loadCameraMatrix(menuBackground.getCamera());
	renderer.renderMenuBackground(&menuBackground);
	renderer.renderParticleManager(rsMenu);

	//2d
	renderer.reset2d();
	state->render();

	//NEWGUI : must be here not in update, not sure why
	// - think of WindowManager::update() as 'render()' :)
	//  our update() mathods update 'world state', in the case of the menus
	//  the camera position, rain drops, anim fram for mouse, etc
	//  we do all our drawing in render() methods (like this one)
	//  glgooey uses render() methods for all its classes, but they are 'triggered'
	//  by WindowManager::update().
	Gooey::WindowManager::instance().update();

	renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);

	if (config.getMiscDebugMode()) {
		renderer.renderText(
				"FPS: " + intToStr(lastFps),
				coreData.getMenuFontNormal(), Vec3f(1.f), 10, 10, false);
	}

	renderer.swapBuffers();
}

//syncronus update
void MainMenu::update() {
	Renderer::getInstance().updateParticleManager(rsMenu);
	mouse2dAnim = (mouse2dAnim + 1) % Renderer::maxMouse2dAnim;
	menuBackground.update();
	state->update();

	// deferred delete of state so that no longer in a GLGooey slot
	if (oldState != NULL) {
		delete oldState;
		oldState = NULL;
	}
}

void MainMenu::tick() {
	lastFps = fps;
	fps = 0;
}

//event magangement: mouse click
void MainMenu::mouseMove(int x, int y, const MouseState &ms) {
	mouseX = x;
	mouseY = y;
	state->mouseMove(x, y, ms);

	WindowManager::instance().onMouseMove(x, Metrics::getInstance().invertHeight(y));
}

//returns if exiting
void MainMenu::mouseDownLeft(int x, int y) {
	state->mouseClick(x, y, mbLeft);

	WindowManager::instance().onLeftButtonDown(x, Metrics::getInstance().invertHeight(y));
}

void MainMenu::mouseDownRight(int x, int y) {
	state->mouseClick(x, y, mbRight);

	WindowManager::instance().onRightButtonDown(x, Metrics::getInstance().invertHeight(y));
}

//NEWGUI
void MainMenu::mouseUpLeft(int x, int y) {
	WindowManager::instance().onLeftButtonUp(x, Metrics::getInstance().invertHeight(y));
}

void MainMenu::mouseUpRight(int x, int y) {
	WindowManager::instance().onRightButtonUp(x, Metrics::getInstance().invertHeight(y));
}
//END NEWGUI

void MainMenu::keyDown(const Key &key) {
	state->keyDown(key);
}

void MainMenu::keyPress(char c) {
	state->keyPress(c);
}

void MainMenu::setState(MenuState *state) {
	// store the old state to be deleted later and apply 
	// the new state
	oldState = this->state;
	this->state = state;

	GraphicComponent::resetFade();
	menuBackground.setTargetCamera(state->getCamera());
}


// =====================================================
//  class MenuState
// =====================================================

MenuState::MenuState(Program &program, MainMenu *mainMenu, const string &stateName)
		: program(program), mainMenu(mainMenu), camera() {

	//camera
	XmlTree xmlTree;
	xmlTree.load("data/core/menu/menu.xml");
	const XmlNode *menuNode = xmlTree.getRootNode();
	const XmlNode *cameraNode = menuNode->getChild("camera");

	//position
	const XmlNode *positionNode = cameraNode->getChild(stateName + "-position");
	Vec3f startPosition;
	startPosition.x = positionNode->getAttribute("x")->getFloatValue();
	startPosition.y = positionNode->getAttribute("y")->getFloatValue();
	startPosition.z = positionNode->getAttribute("z")->getFloatValue();
	camera.setPosition(startPosition);

	//rotation
	const XmlNode *rotationNode = cameraNode->getChild(stateName + "-rotation");
	Vec3f startRotation;
	startRotation.x = rotationNode->getAttribute("x")->getFloatValue();
	startRotation.y = rotationNode->getAttribute("y")->getFloatValue();
	startRotation.z = rotationNode->getAttribute("z")->getFloatValue();
	camera.setOrientation(Quaternion(EulerAngles(
			degToRad(startRotation.x),
			degToRad(startRotation.y),
			degToRad(startRotation.z))));
}

MenuState::~MenuState() {
	/*if (WindowManager::instance().captureWindow()) {
		WindowManager::instance().releaseMouseCapture();
	}*/
}

}}//end namespace
