// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "game_menu.h"
#include "core_data.h"
#include "game.h"
#include "program.h"

#include "leak_dumper.h"

namespace Glest { namespace Gui {

using Global::CoreData;

typedef void (GameMenu::*SlotFunc)(Widget*);

void buildButton(GameMenu *menu, CellStrip *strip, int cell, const string &txt, SlotFunc func) {
	int dh = g_widgetConfig.getDefaultItemHeight();
	Vec2i btnSize(dh * 7, dh);
	Button *btn = new Button(strip, Vec2i(0), btnSize);
	btn->setCell(cell);
	btn->setAnchors(Anchors::getCentreAnchors());
	btn->setText(txt);
	btn->Clicked.connect(menu, func);
}

GameMenu::GameMenu(Vec2i pos, Vec2i size)
		: Frame(&g_widgetWindow, ButtonFlags::CLOSE | ButtonFlags::ROLL_UPDOWN) {
	setPos(pos);
	setSize(size);
	setTitleText(g_lang.get("GameMenu"));

	int dh = g_widgetConfig.getDefaultItemHeight();
	CellStrip *strip = new CellStrip(this, Orientation::VERTICAL, 6);
	strip->setCell(1);
	strip->setAnchors(Anchors::getFillAnchors());

	Vec2i btnSize(dh * 5, dh);

	buildButton(this, strip, 5, g_lang.get("ExitProgram"), &GameMenu::onExit);
	buildButton(this, strip, 4, g_lang.get("QuitGame"), &GameMenu::onQuit);
	buildButton(this, strip, 3, g_lang.get("SaveGame"), &GameMenu::onSaveGame);
	buildButton(this, strip, 2, g_lang.get("TogglePhotoMode"), &GameMenu::onTogglePhotoMode);
	buildButton(this, strip, 1, g_lang.get("ToggleDebug"), &GameMenu::onDebugToggle);
	buildButton(this, strip, 0, g_lang.get("ReturnToGame"), &GameMenu::onReturnToGame);
	Close.connect(this, &GameMenu::onReturnToGame);
}

GameMenu* GameMenu::showDialog(Vec2i pos, Vec2i size) {
	GameMenu* menu = new GameMenu(pos, size);
	g_widgetWindow.setFloatingWidget(menu, true);
	return menu;
}

void GameMenu::onReturnToGame(Widget*) {
	g_gameState.destroyDialog();
}

void GameMenu::onDebugToggle(Widget*) {
	//g_config.setMiscDebugMode(!g_config.getMiscDebugMode());
	g_gameState.toggleDebug();
}

void GameMenu::onTogglePhotoMode(Widget*) {
	g_config.setUiPhotoMode(!g_config.getUiPhotoMode());
}

void GameMenu::onSaveGame(Widget*) {
	g_gameState.destroyDialog();
	g_gameState.doSaveBox();
}

void GameMenu::onQuit(Widget*) {
	g_gameState.destroyDialog();
	g_gameState.confirmQuitGame();
}

void GameMenu::onExit(Widget*) {
	g_gameState.destroyDialog();
	g_gameState.confirmExitProgram();
}

}}

