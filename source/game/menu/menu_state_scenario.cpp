// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
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
	Config &config = Config::getInstance();
	Lang &lang = Lang::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();
	vector<string> results;
	int match = 0;

	labelInfo.init(350, 350);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

	listBoxCategory.init(350, 500, 190);
	labelCategory.init(350, 530);

	listBoxScenario.init(350, 400, 190);
	labelScenario.init(350, 430);

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

	labelCategory.setText(lang.get("Category"));
	labelScenario.setText(lang.get("Scenario"));

	//categories listBox
	findAll("gae/scenarios/*.", results);
	categories = results;

	if (results.size() == 0) {
		throw runtime_error("There are no categories");
	}
	for(int i = 0; i < results.size(); ++i) {
		if (results[i] == config.getUiLastScenarioCatagory()) {
			match = i;
		}
	}

	listBoxCategory.setItems(results);
	listBoxCategory.setSelectedItemIndex(match);
	updateScenarioList(categories[listBoxCategory.getSelectedItemIndex()], true);

	networkManager.init(NR_SERVER);
	msgBox = NULL;
}

MenuStateScenario::~MenuStateScenario() {
	if (msgBox) {
		delete msgBox;
	}
}

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	Config &config = Config::getInstance();
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
		config.save();
		mainMenu->setState(new MenuStateRoot(program, mainMenu)); //TO CHANGE
	} else if (buttonPlayNow.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundC());
		config.save();
		launchGame();
	} else if (listBoxScenario.mouseClick(x, y)) {
		const string &catagory = categories[listBoxCategory.getSelectedItemIndex()];
		const string &scenario = scenarioFiles[listBoxScenario.getSelectedItemIndex()];

		loadScenarioInfo(scenario, scenarioInfo);
		labelInfo.setText(scenarioInfo.desc);
		config.setUiLastScenario(catagory + "/" + scenario);
	} else if (listBoxCategory.mouseClick(x, y)) {
		const string &catagory = categories[listBoxCategory.getSelectedItemIndex()];

		updateScenarioList(catagory);
		config.setUiLastScenarioCatagory(catagory);
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState &ms) {

	listBoxScenario.mouseMove(x, y);
	listBoxCategory.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateScenario::render() {

	Renderer &renderer = Renderer::getInstance();

	renderer.renderLabel(&labelInfo);

	renderer.renderLabel(&labelCategory);
	renderer.renderListBox(&listBoxCategory);

	renderer.renderLabel(&labelScenario);
	renderer.renderListBox(&listBoxScenario);

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);
}

void MenuStateScenario::update() {
	//TOOD: add AutoTest to config
	/*
	if(Config::getInstance().getBool("AutoTest")){
	 AutoTest::getInstance().updateScenario(this);
	}
	*/
}

void MenuStateScenario::launchGame() {
	/*
	msgBox = new GraphicMessageBox();
	msgBox->init(Lang::getInstance().get("Broken"), Lang::getInstance().get("Ok"));
	*/
	shared_ptr<GameSettings> gameSettings = shared_ptr<GameSettings>(new GameSettings());
	loadGameSettings(scenarioInfo, *gameSettings);
	program.setState(new Game(program, gameSettings));
}

void MenuStateScenario::setScenario(int i) {
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()], scenarioInfo);
}

void MenuStateScenario::updateScenarioList(const string &category, bool selectDefault) {
	const Config &config = Config::getInstance();
	vector<string> results;
	int match = 0;

	findAll("gae/scenarios/" + category + "/*.", results);

	//update scenarioFiles
	scenarioFiles = results;
	if (results.size() == 0) {
		throw runtime_error("There are no scenarios for category, " + category + ".");
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
	if(selectDefault) {
		listBoxScenario.setSelectedItemIndex(match);
	}

	//update scenario info
	loadScenarioInfo(scenarioFiles[listBoxScenario.getSelectedItemIndex()], scenarioInfo);
	labelInfo.setText(scenarioInfo.desc);
}


void MenuStateScenario::loadScenarioInfo(const string &file, ScenarioInfo &si) {
	Lang &lang = Lang::getInstance();
	XmlTree xmlTree;

	//gae/scenarios/[category]/[scenario]/[scenario].xml
	xmlTree.load("gae/scenarios/" + categories[listBoxCategory.getSelectedItemIndex()] + "/" + file + "/" + file + ".xml");

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
			XmlAttribute *nameAttrib = playerNode->getAttribute("name", false);
			XmlAttribute *resMultAttrib = playerNode->getAttribute("resource-multiplier", false);
			if (nameAttrib) {
				si.playerNames[i] = nameAttrib->getValue();
				} else if (factionControl == CT_HUMAN) {
				si.playerNames[i] = Config::getInstance().getNetPlayerName();
			} else {
				si.playerNames[i] = "CPU Player";
			}
			if (resMultAttrib) {
				si.resourceMultipliers[i] = resMultAttrib->getFloatValue();
			} else {
				if (factionControl == CT_CPU_ULTRA) {
					si.resourceMultipliers[i] = 3.f;
				} else {
					si.resourceMultipliers[i] = 1.f;
				}
			}
			if (teamIndex < 1 || teamIndex > GameConstants::maxPlayers) {
				throw runtime_error("Team out of range: " + intToStr(teamIndex));
			}

			si.teams[i] = playerNode->getAttribute("team")->getIntValue();
			si.factionTypeNames[i] = playerNode->getAttribute("faction")->getValue();
		}

		si.mapName = scenarioNode->getChild("map")->getAttribute("value")->getValue();
		si.tilesetName = scenarioNode->getChild("tileset")->getAttribute("value")->getValue();
		si.techTreeName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();
		si.defaultUnits = scenarioNode->getChild("default-units")->getAttribute("value")->getBoolValue();
		si.defaultResources = scenarioNode->getChild("default-resources")->getAttribute("value")->getBoolValue();
		si.defaultVictoryConditions = scenarioNode->getChild("default-victory-conditions")->getAttribute("value")->getBoolValue();
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
	NetworkManager &netman = NetworkManager::getInstance();
	GameInterface &gameInterface = *netman.getGameInterface();
	const Config &config = Config::getInstance();
	bool autoRepair = config.getGsAutoRepairEnabled();
	bool autoReturn = config.getGsAutoReturnEnabled();
	int factionCount = 0;

	gs.setDescription(formatString(scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
	gs.setMapPath("maps/" + si.mapName + ".gbm");
	gs.setTilesetPath("tilesets/" + si.tilesetName);
	gs.setTechPath("techs/" + si.techTreeName);
	gs.setScenarioPath("gae/scenarios/" + categories[listBoxCategory.getSelectedItemIndex()]
						+ "/" + scenarioFiles[listBoxScenario.getSelectedItemIndex()]);
	gs.setDefaultUnits(si.defaultUnits);
	gs.setDefaultResources(si.defaultResources);
	gs.setDefaultVictoryConditions(si.defaultVictoryConditions);

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
		/* FIXME: broken.  Shouldn't we consolidate this functionality into MenuStateStartGame? */
		const GameSettings::Faction &newFaction = gs.addFaction(
				/* ct == CT_HUMAN ? Config::getInstance().getNetPlayerName() : */ si.playerNames[i],
				*gs.getTeam(si.teams[i] - 1),
				si.factionTypeNames[i],
				false,
				factionCount,
				si.resourceMultipliers[i]);
		if(ct == CT_HUMAN) {
			gs.setThisFactionId(factionCount);
			gameInterface.setId(factionCount);
			gs.addPlayerToFaction(newFaction, gameInterface.getPlayer());
		} else {
			AiPlayer ai = AiPlayer(
					factionCount,
					si.factionTypeNames[i],
					autoRepair,
					autoReturn,
					ct == CT_CPU_ULTRA);
			gs.addPlayerToFaction(newFaction, ai);
		}
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

	throw std::range_error("Unknown controller type: " + str);
}

} // end namespace
