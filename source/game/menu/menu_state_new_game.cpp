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
#include "menu_state_new_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "socket.h"
#include "game.h"
#include "random.h"
#include "game_settings.h"

#include "leak_dumper.h"

namespace Game {

using namespace Shared::Util;

// =====================================================
//  class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots)
		: MenuStateStartGameBase(program, mainMenu, "new-game")
		, dirty(true) {

	Lang &lang = Lang::getInstance();
	Config &config = Config::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();
	vector<string> results;
	vector<string> teamItems;
	vector<string> controlItems;
	int match = 0;

	//create
	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

	//map listBox
	findAll("maps/*.gbm", results, true);
	if (results.size() == 0) {
		throw runtime_error("There are no maps");
	}
	mapFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastMap()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	listBoxMap.init(200, 320, 150);
	listBoxMap.setItems(results);
	listBoxMap.setSelectedItemIndex(match);
	labelMap.init(200, 350);
	labelMapInfo.init(200, 290, 200, 40);

	//tileset listBox
	match = 0;
	findAll("tilesets/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tile sets");
	}
	tilesetFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastTileset()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	listBoxTileset.init(400, 320, 150);
	listBoxTileset.setItems(results);
	listBoxTileset.setSelectedItemIndex(match);
	labelTileset.init(400, 350);

	//tech Tree listBox
	match = 0;
	findAll("techs/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no tech trees");
	}
	techTreeFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		if(results[i] == config.getUiLastTechTree()) {
			match = i;
		}
		results[i] = formatString(results[i]);
	}
	listBoxTechTree.init(600, 320, 150);
	listBoxTechTree.setItems(results);
	listBoxTechTree.setSelectedItemIndex(match);
	labelTechTree.init(600, 350);

	//list boxes
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		labelPlayers[i].init(200, 500 - i*30);
		listBoxControls[i].init(300, 500 - i*30);
		listBoxFactions[i].init(500, 500 - i*30);
		listBoxTeams[i].init(700, 500 - i*30, 60);
		labelNetStatus[i].init(800, 500 - i*30, 60);
	}

	labelControl.init(300, 550, GraphicListBox::defW, GraphicListBox::defH, true);
	labelFaction.init(500, 550, GraphicListBox::defW, GraphicListBox::defH, true);
	labelTeam.init(700, 550, 60, GraphicListBox::defH, true);

	//texts
	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

	for (int i = 0; i < CT_COUNT; ++i) {
		controlItems.push_back(lang.get(enumControlTypeDesc[i]));
	}

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		teamItems.push_back(Conversion::toStr(i + 1));
	}

	reloadFactions();

	findAll("techs/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results);
	if (results.size() == 0) {
		throw runtime_error("There are no factions for this tech tree");
	}

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		labelPlayers[i].setText(lang.get("Player") + " " + Conversion::toStr(i + 1));
		listBoxTeams[i].setItems(teamItems);
		listBoxTeams[i].setSelectedItemIndex(i);
		listBoxControls[i].setItems(controlItems);
		labelNetStatus[i].setText("");
	}

	labelMap.setText(lang.get("Map"));
	labelTileset.setText(lang.get("Tileset"));
	labelTechTree.setText(lang.get("TechTree"));
	labelControl.setText(lang.get("Control"));
	labelFaction.setText(lang.get("Faction"));
	labelTeam.setText(lang.get("Team"));

	loadMapInfo("maps/" + mapFiles[listBoxMap.getSelectedItemIndex()] + ".gbm", &mapInfo);

	labelMapInfo.setText(mapInfo.desc);

	//initialize network interface
	networkManager.init(NR_SERVER);
	initGameSettings(*networkManager.getNetworkMessenger());
	labelNetwork.init(50, 50);
	try {
		labelNetwork.setText(lang.get("Address") + ": " + networkManager.getServerInterface()->getIpAddress().toString()
				+ ":" + Conversion::toStr(networkManager.getServerInterface()->getPort()));
	} catch (const exception &e) {
		labelNetwork.setText(lang.get("Address") + ": ? " + e.what());
	}

	//init controllers
	listBoxControls[0].setSelectedItemIndex(CT_HUMAN);
	if (openNetworkSlots) {
		for (int i = 1; i < mapInfo.players; ++i) {
			listBoxControls[i].setSelectedItemIndex(CT_NETWORK);
		}
	} else {
		listBoxControls[1].setSelectedItemIndex(CT_CPU);
	}
	updateControlers();

	labelRandomize.init(200, 500 - GameConstants::maxPlayers * 30);
	labelRandomize.setText(lang.get("RandomizeLocations"));
	listBoxRandomize.init(332, 500 - GameConstants::maxPlayers * 30, 75);
	listBoxRandomize.pushBackItem(lang.get("No"));
	listBoxRandomize.pushBackItem(lang.get("Yes"));
	listBoxRandomize.setSelectedItemIndex(Config::getInstance().getGsRandStartLocs() ? 1 : 0);

	updateNetworkSlots();
}


void MenuStateNewGame::mouseClick(int x, int y, MouseButton mouseButton) {
	Config &config = Config::getInstance();
	Lang &lang = Lang::getInstance();
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();

	if (msgBox) {
		if (msgBox->mouseClick(x, y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
	} else if (buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		config.save();
		serverInterface->quit();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
		// this becomes invalid after the above function call
		return;
	} else if (buttonPlayNow.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundC());
		if (isUnconnectedSlots()) {
			buttonPlayNow.mouseMove(1, 1);
			msgBox = new GraphicMessageBox();
			msgBox->init(lang.get("WaitingForConnections"), lang.get("Ok"));
		} else {
			config.save();
			shared_ptr<GameSettings> gs;
			{
				shared_ptr<MutexLock> localLock = serverInterface->getLock();
				updateGameSettings();
				gs = serverInterface->getGameSettings();
				gs->doRandomization(factionFiles);
			}
			serverInterface->launchGame();
			program.setState(new Game(program, gs));
			// this becomes invalid after the above function call
			return;
		}

	} else if (listBoxMap.mouseClick(x, y)) {
		string mapBaseName = mapFiles[listBoxMap.getSelectedItemIndex()];
		string mapFile = "maps/" + mapBaseName + ".gbm";
		loadMapInfo(mapFile, &mapInfo);

		labelMapInfo.setText(mapInfo.desc);
		updateControlers();
		dirty = true;
		config.setUiLastMap(mapBaseName);
	} else if (listBoxTileset.mouseClick(x, y)) {
		dirty = true;
		config.setUiLastTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
	} else if (listBoxTechTree.mouseClick(x, y)) {
		reloadFactions();
		dirty = true;
		config.setUiLastTechTree(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	} else if (listBoxRandomize.mouseClick(x, y)) {
		config.setUiLastRandStartLocs(listBoxRandomize.getSelectedItemIndex());
		dirty = true;
	} else {
		for (int i = 0; i < mapInfo.players; ++i) {
			//ensure thet only 1 human player is present
			if (listBoxControls[i].mouseClick(x, y)) {

				//look for human players
				int humanIndex1 = -1;
				int humanIndex2 = -1;
				for (int j = 0; j < GameConstants::maxPlayers; ++j) {
					ControlType ct = static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
					if (ct == CT_HUMAN) {
						if (humanIndex1 == -1) {
							humanIndex1 = j;
						} else {
							humanIndex2 = j;
						}
					}
				}

				//no human
				if (humanIndex1 == -1 && humanIndex2 == -1) {
					listBoxControls[i].setSelectedItemIndex(CT_HUMAN);
				}

				//2 humans
				if (humanIndex1 != -1 && humanIndex2 != -1) {
					listBoxControls[humanIndex1==i? humanIndex2: humanIndex1].setSelectedItemIndex(CT_CLOSED);
				}
				updateNetworkSlots();
				dirty = true;
			} else if (listBoxFactions[i].mouseClick(x, y)) {
				dirty = true;
			} else if (listBoxTeams[i].mouseClick(x, y)) {
				dirty = true;
			}
		}
	}
	if(dirty) {
		updateGameSettings();
		dirty = false;
	}
}

void MenuStateNewGame::mouseMove(int x, int y, const MouseState &ms) {

	if (msgBox != NULL) {
		msgBox->mouseMove(x, y);
		return;
	}

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		listBoxControls[i].mouseMove(x, y);
		listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
	}
	listBoxMap.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxRandomize.mouseMove(x, y);
}

void MenuStateNewGame::render() {
	Renderer &renderer = Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		renderer.renderLabel(&labelPlayers[i]);
		renderer.renderListBox(&listBoxControls[i]);
		if (listBoxControls[i].getSelectedItemIndex() != CT_CLOSED) {
			renderer.renderListBox(&listBoxFactions[i]);
			renderer.renderListBox(&listBoxTeams[i]);
			renderer.renderLabel(&labelNetStatus[i]);
		}
	}
	renderer.renderLabel(&labelNetwork);
	renderer.renderLabel(&labelMap);
	renderer.renderLabel(&labelTileset);
	renderer.renderLabel(&labelTechTree);
	renderer.renderLabel(&labelControl);
	renderer.renderLabel(&labelFaction);
	renderer.renderLabel(&labelTeam);
	renderer.renderLabel(&labelMapInfo);
	renderer.renderLabel(&labelRandomize);

	renderer.renderListBox(&listBoxMap);
	renderer.renderListBox(&listBoxTileset);
	renderer.renderListBox(&listBoxTechTree);
	renderer.renderListBox(&listBoxRandomize);

	if (msgBox != NULL) {
		renderer.renderMessageBox(msgBox);
	}
}

/**
 * Updates the New Game UI.  This method is called about 40 times per second and aquires the
 * GameSettings object from the network interface, locks and modifies it rather than creating a new
 * GameSettings object that is assigned to the network interface, as occurs when other changes to
 * the UI occur (i.e., faction tree, map, tileset, etc.).
 */
void MenuStateNewGame::update() {
	ServerInterface &si = *NetworkManager::getInstance().getServerInterface();
	bool connectionsChanged = si.isConnectionsChanged();
	bool gsChangeMade = false;
	GameSettings &gs = *si.getGameSettings();
	shared_ptr<MutexLock> localLock = gs.getLock();
	//TOOD: add AutoTest to config
	/*
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateNewGame(program, mainMenu);
	}
	*/
	Lang& lang = Lang::getInstance();

	for (int i = 0; i < mapInfo.players; ++i) {
		ControlType ct = static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		const GameSettings::Faction *f = gs.findFactionForMapSlot(i);
		RemoteClientInterface *client = si.findClientForMapSlot(i);
		if (ct == CT_NETWORK) {
			// GameSettings should have been updated by onMouseClick()
			assert(f);

			// If connections have changed and this network slot is empty, try to fill it with a
			// peer marked "spectator"
			if (connectionsChanged && !client) {
				client = si.findUnslottedClient();
				if(client) {
					si.addPlayerToFaction(f->getId(), client->getId());
					gsChangeMade = true;
				}
			}

			if (client && client->getSocket()) {
				if(client->getState() < STATE_INITIALIZED) {
					labelNetStatus[i].setText("Connecting...");
				} else {
					labelNetStatus[i].setText(client->getStatusStr());
				}
			} else {
				labelNetStatus[i].setText(lang.get("NotConnected"));
			}
		} else {
			if(ct == CT_CLOSED) {
				// if slot is closed, f should be NULL
				assert(!f);
				/*
				if(client) {
					si.removePlayerFromFaction(f->getId(), client->getId());
					gsChangeMade = true;
				}*/
			}
			labelNetStatus[i].setText("");
		}
	}

	if(connectionsChanged && !gsChangeMade) {
		si.resetConnectionsChanged();
	}
}

/**
 * Initialize a GameSettings object using the values of the current user interface
 * (MenuStateNewGame object) and network interface (ServerInterface object).
 */
void MenuStateNewGame::updateGameSettings() {
	ServerInterface &si = *NetworkManager::getInstance().getServerInterface();
	gs = shared_ptr<GameSettings>(new GameSettings());

	//shared_ptr<MutexLock> localLock = gs.getLock();
	const Config &config = Config::getInstance();
	int factionCount = 0;
	bool autoRepairAllowed = config.getNetAutoRepairAllowed();
	bool autoReturnAllowed = config.getNetAutoReturnAllowed();

	//gs.clear();
	gs->readLocalConfig();
	gs->setDescription(formatString(mapFiles[listBoxMap.getSelectedItemIndex()]));
	gs->setMapPath("maps/" + mapFiles[listBoxMap.getSelectedItemIndex()] + ".gbm");
	gs->setMapSlots(mapInfo.players);
	gs->setTilesetPath("tilesets/" + tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
	gs->setTechPath("techs/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	gs->setRandStartLocs(listBoxRandomize.getSelectedItemIndex());
	gs->setScenarioPath("");
	gs->setDefaultVictoryConditions(true);
	gs->setDefaultResources(true);
	gs->setDefaultUnits(true);
	gs->setCommandDelay(10);

	si.unslotAllClients();

	for (int i = 0; i < mapInfo.players; ++i) {
		gs->addTeam("");
	}

	for (int i = 0; i < mapInfo.players; ++i) {
		ControlType ct = static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if (ct == CT_CLOSED) {
			continue;
		}
		const GameSettings::Team &team = gs->getTeamOrThrow(listBoxTeams[i].getSelectedItemIndex());
		int typeIndex = listBoxFactions[i].getSelectedItemIndex();
		bool isTypeRandom = typeIndex >= factionFiles.size();
		string typeName = isTypeRandom ? "" : factionFiles[typeIndex];
		const GameSettings::Faction &f = gs->addFaction(
				"Faction " + Conversion::toStr(i),
				team,
				typeName,
				isTypeRandom,
				i,
				ct == CT_CPU_ULTRA ? 3.0f : 1.0f);
		if (ct == CT_HUMAN) {
			gs->addPlayerToFaction(f, si.getPlayer());
			gs->setThisFactionId(f.getId());
		} else if (ct == CT_NETWORK) {
		} else {
			AiPlayer aiPlayer(10000 + factionCount, "AI Player",
					config.getGsAutoRepairEnabled() && autoRepairAllowed,
					config.getGsAutoReturnEnabled() && autoReturnAllowed,
					ct == CT_CPU_ULTRA);
			gs->addPlayerToFaction(f, aiPlayer);
		}
		factionCount++;
	}

	si.setGameSettings(gs);
}

// ============ PRIVATE ===========================


void MenuStateNewGame::reloadFactions() {

	vector<string> results;

	findAll("techs/" + techTreeFiles[listBoxTechTree.getSelectedItemIndex()] + "/factions/*.", results);

	if (results.size() == 0) {
		throw runtime_error("There is no factions for this tech tree");
	}

	factionFiles.clear();
	factionFiles = results;
	for (int i = 0; i < results.size(); ++i) {
		results[i] = formatString(results[i]);
	}

	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		listBoxFactions[i].setItems(results);
		listBoxFactions[i].pushBackItem(Lang::getInstance().get("Random"));
		listBoxFactions[i].setSelectedItemIndex(i % results.size());
	}
}

void MenuStateNewGame::updateControlers() {
	bool humanPlayer = false;

	for (int i = 0; i < mapInfo.players; ++i) {
		if (listBoxControls[i].getSelectedItemIndex() == CT_HUMAN) {
			humanPlayer = true;
		}
	}

	if (!humanPlayer) {
		listBoxControls[0].setSelectedItemIndex(CT_HUMAN);
	}

	for (int i = mapInfo.players; i < GameConstants::maxPlayers; ++i) {
		listBoxControls[i].setSelectedItemIndex(CT_CLOSED);
	}
}

bool MenuStateNewGame::isUnconnectedSlots() {
	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();
	for (int i = 0; i < mapInfo.players; ++i) {
		if (listBoxControls[i].getSelectedItemIndex() == CT_NETWORK) {
			RemoteClientInterface *client = serverInterface->findClientForMapSlot(i);
			if (!(client && client->isConnected())) {
				return true;
			}
		}
	}
	return false;
}

} // end namespace
