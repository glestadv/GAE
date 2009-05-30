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
#include "menu_state_scenario.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "network_manager.h"
#include "game.h"

#include "leak_dumper.h"

namespace Game {

using namespace Shared::Xml;

// =====================================================
//  class MenuStateScenario
// =====================================================

MenuStateScenario::MenuStateScenario(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu, "scenario") {
	Lang &lang = Lang::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();
	vector<string> results;

	labelInfo.init(350, 350);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

	listBoxScenario.init(350, 400, 190);
	labelScenario.init(350, 430);

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

	labelScenario.setText(lang.get("Scenario"));

	//tileset listBox
	findAll("scenarios/*.xml", results, true);
	scenarioFiles = results;
	if (!results.size()) {
		throw runtime_error("There is no scenarios");
	}
	for (int i = 0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}
	listBoxScenario.setItems(results);

	loadScenarioInfo("scenarios/" + scenarioFiles[listBoxScenario.getSelectedItemIndex()] + ".xml", scenarioInfo);
	labelInfo.setText(scenarioInfo.desc);

	networkManager.init(NR_SERVER);
	msgBox = NULL;
}

MenuStateScenario::~MenuStateScenario() {
	if (msgBox) {
		delete msgBox;
	}
}

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {

	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	if (msgBox) {
		if (msgBox->mouseClick(x, y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
	} else if (buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	} else if (buttonPlayNow.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundC());
		msgBox = new GraphicMessageBox();
		msgBox->init(Lang::getInstance().get("Broken"), Lang::getInstance().get("Ok"));
		/* FIXME: broken
		GameSettings *gs = new GameSettings();
		loadGameSettings(scenarioInfo, *gs);
		program.setState(new Game(program, gs));
		*/
	} else if (listBoxScenario.mouseClick(x, y)) {
		loadScenarioInfo("scenarios/" + scenarioFiles[listBoxScenario.getSelectedItemIndex()] + ".xml", scenarioInfo);
		labelInfo.setText(scenarioInfo.desc);
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState &ms) {
	listBoxScenario.mouseMove(x, y);
	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateScenario::render() {
	Renderer &renderer = Renderer::getInstance();

	renderer.renderLabel(&labelInfo);
	renderer.renderLabel(&labelScenario);
	renderer.renderListBox(&listBoxScenario);
	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);

	if(msgBox) {
		renderer.renderMessageBox(msgBox);
	}
}

void MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo &si) {
	Lang &lang = Lang::getInstance();
	XmlTree xmlTree;

	xmlTree.load(file);

	const XmlNode *scenarioNode = xmlTree.getRootNode();
	const XmlNode *difficultyNode = scenarioNode->getChild("difficulty");
	si.difficulty = difficultyNode->getAttribute("value")->getIntValue();
	if (si.difficulty < dVeryEasy || si.difficulty > dInsane) {
		throw std::runtime_error("Invalid difficulty");
	}

	const XmlNode *playersNode = scenarioNode->getChild("players");
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		const XmlNode* playerNode = playersNode->getChild("player", i);
		ControlType factionControl = strToControllerType(playerNode->getAttribute("control")->getValue());
		string factionTypeName;

		si.factionControls[i] = factionControl;

		if (factionControl != CT_CLOSED) {
			int teamIndex = playerNode->getAttribute("team")->getIntValue();

			if (teamIndex < 1 || teamIndex > GameConstants::maxPlayers) {
				throw runtime_error("Team out of range: " + Conversion::toStr(teamIndex));
			}

			si.teams[i] = playerNode->getAttribute("team")->getIntValue();
			si.factionTypeNames[i] = playerNode->getAttribute("faction")->getValue();
		}

		si.mapName = scenarioNode->getChild("map")->getAttribute("value")->getValue();
		si.tilesetName = scenarioNode->getChild("tileset")->getAttribute("value")->getValue();
		si.techTreeName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();
	}

	//add player info
	si.desc = lang.get("Player") + ": ";
	for (int i = 0; i < GameConstants::maxFactions; ++i) {
		if (si.factionControls[i] == CT_HUMAN) {
			si.desc += formatString(si.factionTypeNames[i]);
			break;
		}
	}

	//add misc info
	string difficultyString = "Difficulty" + intToStr(si.difficulty);
	stringstream str;
	str << endl
		<< lang.get("Difficulty") << ": " << lang.get(difficultyString) << endl
		<< lang.get("Map") << ": " << formatString(si.mapName) << endl
		<< lang.get("Tileset") << ": " << formatString(si.tilesetName) << endl
		<< lang.get("TechTree") << ": " << formatString(si.techTreeName) << endl;
	si.desc = str.str();
}

void MenuStateScenario::loadGameSettings(const ScenarioInfo &si, GameSettings &gs) {
	int factionCount = 0;

	gs.setDescription(formatString(scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
	gs.setMapPath("maps/" + si.mapName + ".gbm");
	gs.setTilesetPath("tilesets/" + si.tilesetName);
	gs.setTechPath("techs/" + si.techTreeName);

	// since this can get out of whack when a player disconnects unexpectedly, we're going to
	// sanitize it.
	if (gs.getCommandDelay() < 250 || gs.getCommandDelay() > 1000) {
		gs.setCommandDelay(250);
	}

	for (int i = 0; i < GameConstants::maxFactions; ++i) {
		gs.addTeam("");
	}

	for (int i = 0; i < GameConstants::maxFactions; ++i) {
		ControlType ct = static_cast<ControlType>(si.factionControls[i]);
		if (ct == CT_CLOSED) {
			continue;
		}
		/* FIXME: broken.  Shouldn't we consolidate this functionality into MenuStateStartGame?
		gs.addFaction(
				gs.getTeam(si.teams[i] - 1),
				factionCount,
				si.factionTypeNames[i],
				ct == CT_HUMAN ? Config::getInstance().getNetPlayerName() : string(""),
				"",
				i,
				ct);
		*/
		factionCount++;
	}
}

ControlType MenuStateScenario::strToControllerType(const string &str) {
	if (str == "closed") {
		return CT_CLOSED;
	} else if (str == "cpu") {
		return CT_CPU;
	} else if (str == "cpu-ultra") {
		return CT_CPU_ULTRA;
	} else if (str == "human") {
		return CT_HUMAN;
	}

	throw std::runtime_error("Unknown controller type: " + str);
}

} // end namespace
