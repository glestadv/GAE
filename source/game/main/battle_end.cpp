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
#include "battle_end.h"

#include "main_menu.h"
#include "program.h"
#include "core_data.h"
#include "lang.h"
#include "util.h"
#include "renderer.h"
#include "main_menu.h"
#include "sound_renderer.h"
#include "components.h"
#include "metrics.h"
#include "stats.h"
#include "auto_test.h"
#include "text_renderer.h"

#include "leak_dumper.h"


using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
//  class BattleEnd
// =====================================================

BattleEnd::BattleEnd(GuiProgram &program)
		: GuiProgramState(program) {
}

BattleEnd::~BattleEnd() {
	getSoundRenderer().playMusic(theCoreData.getMenuMusic());
}

void BattleEnd::update(){
	//TOOD: add AutoTest to config
	/*
	if(theConfig.getMiscAutoTest()){
		AutoTest::getInstance().updateBattleEnd(program);
	}*/
}

void BattleEnd::render() {
	Renderer &renderer = getRenderer();
	TextRenderer2D *textRenderer = renderer.getTextRenderer();
	const Lang &lang = getLang();

	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderBackground(getCoreData().getBackgroundTexture());

	textRenderer->begin(getCoreData().getMenuFontBig());

	int lm = 80;
	int bm = 100;

	foreach(const shared_ptr<GameSettings::Faction> &f, gs->getFactions()) {
		int id = f->getId();

		int textX = lm + 300 + id * 120;
		int team = f->getTeam().getId() + 1;
		int kills = stats.getKills(id);
		int deaths = stats.getDeaths(id);
		int unitsProduced = stats.getUnitsProduced(id);
		int resourcesHarvested = stats.getResourcesHarvested(id);

		int score = kills * 100 + unitsProduced * 50 + resourcesHarvested / 10;
		string controlString;

		switch (gs.getFactionControl(i)) {
		case CT_CPU:
			controlString = lang.get("Cpu");
			break;
		case CT_CPU_ULTRA:
			controlString = lang.get("CpuUltra");
			break;
		case CT_NETWORK:
			controlString = lang.get("Network");
			break;
		case CT_HUMAN:
			controlString = lang.get("Human");
			break;
		default:
			assert(false);
		};

		string playerName = gs.getPlayerName(i).empty()
				? (lang.get("Player") + " " + intToStr(i + 1))
				: gs.getPlayerName(i);
		if(gs.getThisFactionIndex() == i) {
			playerName = Config::getInstance().getNetPlayerName();
		}
//		textRenderer->render((lang.get("Player") + " " + intToStr(i + 1)).c_str(), textX, bm + 400);
//		textRenderer->render(gs.getPlayerName(i), textX, bm + 400);
		textRenderer->render(playerName, textX, bm + 400);
		textRenderer->render(stats.getVictory(i) ? lang.get("Victory").c_str() : lang.get("Defeat").c_str(), textX, bm + 360);
		textRenderer->render(controlString, textX, bm + 320);
		textRenderer->render(gs.getFactionTypeName(i), textX, bm + 280);
		textRenderer->render(intToStr(team).c_str(), textX, bm + 240);
		textRenderer->render(intToStr(kills).c_str(), textX, bm + 200);
		textRenderer->render(intToStr(deaths).c_str(), textX, bm + 160);
		textRenderer->render(intToStr(unitsProduced).c_str(), textX, bm + 120);
		textRenderer->render(intToStr(resourcesHarvested).c_str(), textX, bm + 80);
		textRenderer->render(intToStr(score).c_str(), textX, bm + 20);
	}

	textRenderer->render(lang.get("Result"), lm + 50, bm + 360);
	textRenderer->render(lang.get("Control"), lm + 50, bm + 320);
	textRenderer->render(lang.get("Faction"), lm + 50, bm + 280);
	textRenderer->render(lang.get("Team"), lm + 50, bm + 240);
	textRenderer->render(lang.get("Kills"), lm + 50, bm + 200);
	textRenderer->render(lang.get("Deaths"), lm + 50, bm + 160);
	textRenderer->render(lang.get("UnitsProduced"), lm + 50, bm + 120);
	textRenderer->render(lang.get("ResourcesHarvested"), lm + 50, bm + 80);
	textRenderer->render(lang.get("Score"), lm + 50, bm + 20);

	textRenderer->end();

	textRenderer->begin(getCoreData().getMenuFontVeryBig());

	string header = gs->getDescription() + " - ";

	if (stats.getVictory(gs->getThisFactionId())) {
		header += lang.get("Victory");
	} else {
		header += lang.get("Defeat");
	}

	textRenderer->render(header, lm + 250, bm + 550);

	textRenderer->end();
	renderer.swapBuffers();
}

void BattleEnd::keyDown(const Key &key) {
	if(!key.isModifier()) {
		getProgram().setState(new MainMenu(getProgram()));
	}
}

void BattleEnd::mouseDownLeft(int x, int y) {
	getProgram().setState(new MainMenu(getProgram()));
}

}}//end namespace
