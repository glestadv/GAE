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
#include "user_interface.h"
#include "resource_bar.h"

#include "leak_dumper.h"

namespace Glest { namespace Gui {

using Global::CoreData;

typedef void (GameMenu::*SlotFunc)(Widget*);

Button* buildButton(GameMenu *menu, CellStrip *strip, int cell, const string &txt, SlotFunc func) {
	int dh = g_widgetConfig.getDefaultItemHeight();
	Vec2i btnSize(dh * 7, dh);
	Button *btn = new Button(strip, Vec2i(0), btnSize);
	btn->setCell(cell);
	btn->setAnchors(Anchors::getCentreAnchors());
	btn->setText(txt);
	btn->Clicked.connect(menu, func);
	return btn;
}

GameMenu::GameMenu()
		: Frame((Container*)(&g_widgetWindow), ButtonFlags::CLOSE | ButtonFlags::ROLL_UPDOWN)
		, m_btnStrip(0) {
	setTitleText(_("Game Menu"));

	m_btnStrip = new CellStrip(this, Orientation::VERTICAL, 9); // 8 buttons
	m_btnStrip->setCell(1);
	m_btnStrip->setAnchors(Anchors::getFillAnchors());
	Close.connect(this, &GameMenu::onReturnToGame);
	init();
}

void GameMenu::init() {
	m_btnStrip->clear();
	m_btnStrip->addCells(9);

	int dh = g_widgetConfig.getDefaultItemHeight();
	Vec2i size = Vec2i(dh * 8, dh * 12);
	Vec2i pos = Vec2i(g_metrics.getScreenDims() - size) / 2;
	setPos(pos);
	setSize(size);

	buildButton(this, m_btnStrip, 8, _("Exit Program"), &GameMenu::onExit);
	buildButton(this, m_btnStrip, 7, _("Quit Game"), &GameMenu::onQuit);
	buildButton(this, m_btnStrip, 6, _("Save Game"), &GameMenu::onSaveGame);
	m_pinWidgetsBtn = buildButton(this, m_btnStrip, 5,
		g_config.getUiPinWidgets() ? _("Unpin Widgets") : _("Pin Widgets"),
		&GameMenu::onPinWidgets);
	buildButton(this, m_btnStrip, 4, _("Save Widgets"), &GameMenu::onSaveWidgets);
	buildButton(this, m_btnStrip, 3, _("Reset Widgets"), &GameMenu::onResetWidgets);
	buildButton(this, m_btnStrip, 2, _("Toggle Photo Mode"), &GameMenu::onTogglePhotoMode);
	//buildButton(this, m_btnStrip, 2, _("Toggle Debug Info"), &GameMenu::onDebugToggle);
	buildButton(this, m_btnStrip, 1, _("Options"), &GameMenu::onOptions);
	buildButton(this, m_btnStrip, 0, _("Return to Game"), &GameMenu::onReturnToGame);
}

//GameMenu* GameMenu::showDialog(Vec2i pos, Vec2i size) {
//	GameMenu* menu = new GameMenu(pos, size);
//	g_widgetWindow.setFloatingWidget(menu, true);
//	return menu;
//}

void GameMenu::onSaveWidgets(Widget*) {
	g_userInterface.getResourceBar()->persist();
	g_userInterface.getMinimap()->persist();
	g_userInterface.getDisplay()->persist();
	g_config.save();
}

void GameMenu::onResetWidgets(Widget*) {
	g_userInterface.getResourceBar()->reset();
	g_userInterface.getMinimap()->reset();
	g_userInterface.getDisplay()->reset();
	g_config.save();
}

void GameMenu::onPinWidgets(Widget*) {
	g_gameState.togglePinWidgets(0);
	m_pinWidgetsBtn->setText(g_config.getUiPinWidgets() ? _("Unpin Widgets") : _("Pin Widgets"));
}

void GameMenu::onReturnToGame(Widget*) {
	g_gameState.toggleGameMenu();
}

void GameMenu::onOptions(Widget*) {
	g_gameState.toggleOptions();
}

//void GameMenu::onDebugToggle(Widget*) {
//	g_gameState.toggleDebug();
//}

void GameMenu::onTogglePhotoMode(Widget*) {
	g_config.setUiPhotoMode(!g_config.getUiPhotoMode());
}

void GameMenu::onSaveGame(Widget*) {
	g_gameState.toggleGameMenu();
	g_gameState.doSaveBox();
}

void GameMenu::onQuit(Widget*) {
	g_gameState.toggleGameMenu();
	g_gameState.confirmQuitGame();
}

void GameMenu::onExit(Widget*) {
	g_gameState.toggleGameMenu();
	g_gameState.confirmExitProgram();
}

}}

