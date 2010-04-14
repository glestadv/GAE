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
#include "auto_test.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

using namespace Shared::Xml;

// =====================================================
//  class MenuStateScenario
// =====================================================

MenuStateScenario::MenuStateScenario(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu, "scenario")
		, msgBox(0), failAction(FailAction::INVALID) {
	Config &config = Config::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();
	vector<string> results;
	int match = -1;
	
	labelInfo.init(350, 350);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

	listBoxCategory.init(350, 500, 190);
	labelCategory.init(350, 530);

	listBoxScenario.init(350, 400, 190);
	labelScenario.init(350, 430);

	buttonReturn.setText(theLang.get("Return"));
	buttonPlayNow.setText(theLang.get("PlayNow"));

	labelCategory.setText(theLang.get("Category"));
	labelScenario.setText(theLang.get("Scenario"));

	//categories listBox
	findAll("gae/scenarios/*.", results);
	
	// remove empty directories...
	for (vector<string>::iterator cat = results.begin(); cat != results.end(); ) {
		vector<string> scenarios;
		findAll("gae/scenarios/" + *cat + "/*.", scenarios);
		if (scenarios.empty()) {
			cat = results.erase(cat);
		} else {
			++cat;
		}
	}
	// fail gracefully
	if (results.empty()) {
		msgBox = new GraphicMessageBox();
		msgBox->init(theLang.get("NoCategoryDirectories"), theLang.get("Ok"));
		failAction = FailAction::MAIN_MENU;
		return;
	}
	for(int i = 0; i < results.size(); ++i) {
		if (results[i] == config.getUiLastScenarioCatagory()) {
			match = i;
		}
	}

	categories = results;
	listBoxCategory.setItems(results);
	if (match != -1) {
		listBoxCategory.setSelectedItemIndex(match);
	}
	updateScenarioList(categories[listBoxCategory.getSelectedItemIndex()], true);

	networkManager.init(nrServer);
}


void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	Config &config = Config::getInstance();
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	if (msgBox) {
		if (msgBox->mouseClick(x,y)) {
			delete msgBox;
			msgBox = 0;
			switch (failAction) {
				case FailAction::MAIN_MENU:
					soundRenderer.playFx(coreData.getClickSoundA());
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
					return;
				case FailAction::SCENARIO_MENU:
					soundRenderer.playFx(coreData.getClickSoundA());
					break;
			}
		}
		return;
	}
	if (buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		config.save();
		mainMenu->setState(new MenuStateRoot(program, mainMenu)); //TO CHANGE
	} else if (buttonPlayNow.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundC());
		config.save();
		launchGame();
	} else if (listBoxScenario.mouseClick(x, y)) {
		const string &catagory = categories[listBoxCategory.getSelectedItemIndex()];
		const string &scenario = scenarioFiles[listBoxScenario.getSelectedItemIndex()];

		if (loadScenarioInfo(scenario, &scenarioInfo)) {
			labelInfo.setText(scenarioInfo.desc);
			config.setUiLastScenario(catagory + "/" + scenario);
			buttonPlayNow.setEnabled(true);
		} else {
			labelInfo.setText(theLang.get("Unavailable"));
			buttonPlayNow.setEnabled(false);
		}
	} else if (listBoxCategory.mouseClick(x, y)) {
		const string &catagory = categories[listBoxCategory.getSelectedItemIndex()];

		updateScenarioList(catagory);
		config.setUiLastScenarioCatagory(catagory);
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState &ms) {
	if (!msgBox) {
		listBoxScenario.mouseMove(x, y);
		listBoxCategory.mouseMove(x, y);

		buttonReturn.mouseMove(x, y);
		buttonPlayNow.mouseMove(x, y);
	} else {
		msgBox->mouseMove(x, y);
	}
}

void MenuStateScenario::render() {
	Renderer &renderer = Renderer::getInstance();

	if(msgBox) {
		renderer.renderMessageBox(msgBox);
		return;
	}
	renderer.renderLabel(&labelInfo);

	renderer.renderLabel(&labelCategory);
	renderer.renderListBox(&listBoxCategory);

	renderer.renderLabel(&labelScenario);
	renderer.renderListBox(&listBoxScenario);

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);
}

void MenuStateScenario::update() {
	if (Config::getInstance().getMiscAutoTest()) {
		AutoTest::getInstance().updateScenario(this);
	}
}

void MenuStateScenario::launchGame() {
	GameSettings gameSettings;
	if (loadGameSettings(&scenarioInfo, &gameSettings)) {
		program.setState(new Game(program, gameSettings));
	}
}

void MenuStateScenario::setScenario(int i) {
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()], &scenarioInfo);
}

void MenuStateScenario::updateScenarioList(const string &category, bool selectDefault) {
	const Config &config = Config::getInstance();
	vector<string> results;
	int match = -1;

	findAll("gae/scenarios/" + category + "/*.", results);

	//update scenarioFiles
	scenarioFiles = results;
	if (results.empty()) {
		// this shouldn't happen, empty directories have been weeded out earlier
		throw runtime_error("No scenario directories found for category: " + category);
	}
	for (int i = 0; i < results.size(); ++i) {
		string path = category + "/" + results[i];
		cout << path << " / " << config.getUiLastScenario() << endl;
		if (path == config.getUiLastScenario()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	listBoxScenario.setItems(results);
	if (selectDefault && match != -1) {
		listBoxScenario.setSelectedItemIndex(match);
	}

	// update scenario info
	if (!scenarioFiles.empty()) {
		if (loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()], &scenarioInfo)) {
			buttonPlayNow.setEnabled(true);
			return;
		}// else problem with scenario... fall through
	} // else no scenarios in category
	labelInfo.setText(theLang.get("Unavailable"));
	buttonPlayNow.setEnabled(false);
}

bool MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
	XmlTree xmlTree;
	try {
		//gae/scenarios/[category]/[scenario]/[scenario].xml
		xmlTree.load("gae/scenarios/" + categories[listBoxCategory.getSelectedItemIndex()] + "/" + file + "/" + file + ".xml");

		const XmlNode *scenarioNode = xmlTree.getRootNode();
		const XmlNode *difficultyNode = scenarioNode->getChild("difficulty");
		scenarioInfo->difficulty = difficultyNode->getAttribute("value")->getIntValue();
		if (scenarioInfo->difficulty < dVeryEasy || scenarioInfo->difficulty > dInsane) {
			throw std::runtime_error("Invalid difficulty");
		}

		const XmlNode *tmp = scenarioNode->getOptionalChild("fog-of-war");
		if ( tmp ) {
			scenarioInfo->fogOfWar = tmp->getAttribute("value")->getBoolValue();
		} else {
			scenarioInfo->fogOfWar = true;
		}
		tmp = scenarioNode->getOptionalChild("shroud-of-darkness");
		if ( tmp ) {
			scenarioInfo->shroudOfDarkness = tmp->getAttribute("value")->getBoolValue();
		} else {
			scenarioInfo->shroudOfDarkness = true;
		}

		const XmlNode *playersNode = scenarioNode->getChild("players");
		for (int i = 0; i < GameConstants::maxPlayers; ++i) {
			const XmlNode* playerNode = playersNode->getChild("player", i);
			ControlType factionControl = strToControllerType(playerNode->getAttribute("control")->getValue());
			string factionTypeName;

			scenarioInfo->factionControls[i] = factionControl;

			if (factionControl != ControlType::CLOSED) {
				int teamIndex = playerNode->getAttribute("team")->getIntValue();
				XmlAttribute *nameAttrib = playerNode->getAttribute("name", false);
				XmlAttribute *resMultAttrib = playerNode->getAttribute("resource-multiplier", false);
				if (nameAttrib) {
					scenarioInfo->playerNames[i] = nameAttrib->getValue();
				} else if (factionControl == ControlType::HUMAN) {
					scenarioInfo->playerNames[i] = Config::getInstance().getNetPlayerName();
				} else {
					scenarioInfo->playerNames[i] = "CPU Player";
				}
				if (resMultAttrib) {
					scenarioInfo->resourceMultipliers[i] = resMultAttrib->getFloatValue();
				} else {
					if (factionControl == ControlType::CPU_MEGA) {
						scenarioInfo->resourceMultipliers[i] = 4.f;
					}
					else if (factionControl == ControlType::CPU_ULTRA) {
						scenarioInfo->resourceMultipliers[i] = 3.f;
					} 
					else {
						scenarioInfo->resourceMultipliers[i] = 1.f;
					}
				}
				if (teamIndex < 1 || teamIndex > GameConstants::maxPlayers) {
					throw runtime_error("Team out of range: " + intToStr(teamIndex));
				}

				scenarioInfo->teams[i] = playerNode->getAttribute("team")->getIntValue();
				scenarioInfo->factionTypeNames[i] = playerNode->getAttribute("faction")->getValue();
			}

			scenarioInfo->mapName = scenarioNode->getChild("map")->getAttribute("value")->getValue();
			scenarioInfo->tilesetName = scenarioNode->getChild("tileset")->getAttribute("value")->getValue();
			scenarioInfo->techTreeName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();
			scenarioInfo->defaultUnits = scenarioNode->getChild("default-units")->getAttribute("value")->getBoolValue();
			scenarioInfo->defaultResources = scenarioNode->getChild("default-resources")->getAttribute("value")->getBoolValue();
			scenarioInfo->defaultVictoryConditions = scenarioNode->getChild("default-victory-conditions")->getAttribute("value")->getBoolValue();
		}
	} catch (exception &e) {
		theLogger.getErrorLog().addXmlError(file + ".xml", e.what());
		msgBox = new GraphicMessageBox();
		msgBox->init(theLang.get("ScenarioXmlError"), theLang.get("Ok"));
		failAction = FailAction::SCENARIO_MENU;
		return false;
	}

	//add player info
	scenarioInfo->desc = theLang.get("Player") + ": ";
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (scenarioInfo->factionControls[i] == ControlType::HUMAN) {
			scenarioInfo->desc += formatString(scenarioInfo->factionTypeNames[i]);
			break;
		}
	}

	//add misc info
	string difficultyString = "Difficulty" + intToStr(scenarioInfo->difficulty);

	scenarioInfo->desc += "\n";
	scenarioInfo->desc += theLang.get("Difficulty") + ": " + theLang.get(difficultyString) + "\n";
	scenarioInfo->desc += theLang.get("Map") + ": " + formatString(scenarioInfo->mapName) + "\n";
	scenarioInfo->desc += theLang.get("Tileset") + ": " + formatString(scenarioInfo->tilesetName) + "\n";
	scenarioInfo->desc += theLang.get("TechTree") + ": " + formatString(scenarioInfo->techTreeName) + "\n";
	return true;
}

bool MenuStateScenario::loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gs) {
	string scenarioPath = "gae/scenarios/" + categories[listBoxCategory.getSelectedItemIndex()]
							+ "/" + scenarioFiles[listBoxScenario.getSelectedItemIndex()];
	// map in scenario dir ?
	string	test1 = scenarioPath + "/" + scenarioInfo->mapName + ".gbm",
			test2 = "maps/" + scenarioInfo->mapName + ".gbm";
	if (fileExists(test1)) {
		gs->setMapPath(test1);
	} else if (fileExists(test2)) {
		gs->setMapPath(test2);
	} else {
		msgBox = new GraphicMessageBox();
		msgBox->init(theLang.get("MapNotFound") + ": " + scenarioInfo->mapName, theLang.get("Ok"));
		failAction = FailAction::SCENARIO_MENU;
		return false;
	}
	gs->setDescription(formatString(scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
	
	if (!fileExists("tilesets/" + scenarioInfo->tilesetName + "/" + scenarioInfo->tilesetName + ".xml")) {
		msgBox = new GraphicMessageBox();
		msgBox->init(theLang.get("TilesetNotFound") + ": " + scenarioInfo->tilesetName, theLang.get("Ok"));
		failAction = FailAction::SCENARIO_MENU;
		return false;
	}
	gs->setTilesetPath(string("tilesets/") + scenarioInfo->tilesetName);
	
	if (!fileExists("techs/" + scenarioInfo->techTreeName + "/" + scenarioInfo->techTreeName + ".xml")) {
		msgBox = new GraphicMessageBox();
		msgBox->init(theLang.get("TechNotFound") + ": " + scenarioInfo->tilesetName, theLang.get("Ok"));
		failAction = FailAction::SCENARIO_MENU;
		return false;
	}
	gs->setTechPath(string("techs/") + scenarioInfo->techTreeName);

	gs->setScenarioPath(scenarioPath);
	gs->setDefaultUnits(scenarioInfo->defaultUnits);
	gs->setDefaultResources(scenarioInfo->defaultResources);
	gs->setDefaultVictoryConditions(scenarioInfo->defaultVictoryConditions);

	int factionCount = 0;
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		ControlType ct = static_cast<ControlType>(scenarioInfo->factionControls[i]);
		if (ct != ControlType::CLOSED) {
			if (ct == ControlType::HUMAN) {
				gs->setThisFactionIndex(factionCount);
			}
			gs->setFactionControl(factionCount, ct);
			gs->setPlayerName(factionCount, scenarioInfo->playerNames[i]);
			gs->setTeam(factionCount, scenarioInfo->teams[i] - 1);
			gs->setStartLocationIndex(factionCount, i);
			gs->setFactionTypeName(factionCount, scenarioInfo->factionTypeNames[i]);
			gs->setResourceMultiplier(factionCount, scenarioInfo->resourceMultipliers[i]);
			factionCount++;
		}
	}
	gs->setFogOfWar(scenarioInfo->fogOfWar);
	//Config::getInstance().setGsShroudOfDarknessEnabled(scenarioInfo->shroudOfDarkness);
	gs->setFactionCount(factionCount);
	return true;
}

ControlType MenuStateScenario::strToControllerType(const string &str) {
	if (str == "closed") {
		return ControlType::CLOSED;
	} else if (str == "cpu") {
		return ControlType::CPU;
	} else if (str == "cpu-ultra") {
		return ControlType::CPU_ULTRA;
	} else if (str == "human") {
		return ControlType::HUMAN;
	}

	throw std::runtime_error("Unknown controller type: " + str);
}

}}//end namespace
