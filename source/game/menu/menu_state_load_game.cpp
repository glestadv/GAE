// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Jaagup Repän <jrepan@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_load_game.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "game.h"
#include "network_manager.h"
#include "xml_parser.h"

#include "leak_dumper.h"


namespace Game {

using namespace Shared::Util;
// =====================================================
// 	class MenuStateLoadGame
// =====================================================
SavedGamePreviewLoader::SavedGamePreviewLoader(MenuStateLoadGame &menu)
		: Thread()
		, menu(menu)
		, fileName()
		, seeJaneRun(true)
		, mutex() {
	start();
}


void SavedGamePreviewLoader::goAway() {
	MutexLock lock(mutex);
	seeJaneRun = false;
}

void SavedGamePreviewLoader::execute() {
	while(seeJaneRun) {
		MutexLock lock(mutex);
		if(fileName.get()) {
			// Store filename in local variable and clear this->fileName
			shared_ptr<string> localFN;
			localFN.swap(fileName);
			mutex.v();
			_loadInternal(localFN);
			mutex.p();
		} else {
			mutex.v();
			sleep(25);
			mutex.p();
		}
	}
}

void SavedGamePreviewLoader::_loadInternal(const shared_ptr<string> &fileName) {
	string err;
	shared_ptr<const XmlNode> root;

	try {
		root = shared_ptr<const XmlNode> (XmlIo::getInstance().load(*fileName));
	} catch (exception &e) {
		err = "Can't open game " + *fileName + ": " + e.what();
	}

	menu.setGameInfo(*fileName, root, err);
}

void SavedGamePreviewLoader::load(const string &fileName) {
	MutexLock lock(mutex);
	this->fileName = shared_ptr<string>(new string(fileName));
}


// =====================================================
// 	class MenuStateLoadGame
// =====================================================

MenuStateLoadGame::MenuStateLoadGame(Program &program, MainMenu *mainMenu) :
		MenuStateStartGameBase(program, mainMenu, "loadgame"), loaderThread(*this) {
	confirmMessageBox = NULL;
	savedGame.reset();

	Shared::Platform::mkdir("savegames", true);

	Lang &lang= Lang::getInstance();

	//create
	buttonReturn.init(350, 200, 100);
	buttonDelete.init(462, 200, 100);
	buttonPlayNow.init(575, 200, 100);

	//savegames listBoxGames
	listBoxGames.init(400, 300, 225);
	if(!loadGameList()) {
		msgBox = new GraphicMessageBox();
		msgBox->init(Lang::getInstance().get("NoSavedGames"), Lang::getInstance().get("Ok"));
		criticalError = true;
		return;
	}

	//texts
	buttonReturn.setText(lang.get("Return"));
	buttonDelete.setText(lang.get("Delete"));
	buttonPlayNow.setText(lang.get("Load"));

	//game info lables
	labelInfoHeader.init(350, 500, 440, 225, false);

    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(350, 450-i*30);
        labelControls[i].init(425, 450-i*30);
        labelFactions[i].init(500, 450-i*30);
		labelTeams[i].init(575, 450-i*30, 60);
		labelNetStatus[i].init(600, 450-i*30, 60);
	}
	//initialize network interface
	NetworkManager &networkManager= NetworkManager::getInstance();
	networkManager.init(NR_SERVER);
	ServerInterface *serverInterface = networkManager.getServerInterface();
	initGameSettings(*serverInterface);

//	serverInterface->setResumingSaved(true);
	labelNetwork.init(50, 50);
	try {
		labelNetwork.setText(lang.get("Address") + ": " + serverInterface->getIpAddress().toString()
				+ ":" + Conversion::toStr(serverInterface->getPort()));
	} catch(const exception &e) {
		labelNetwork.setText(lang.get("Address") + ": ? " + e.what());
	}
	updateNetworkSlots();
	selectionChanged();
}

MenuStateLoadGame::~MenuStateLoadGame() {
	loaderThread.goAway();
	if(confirmMessageBox) {
		delete confirmMessageBox;
	}
	if(msgBox) {
		delete msgBox;
	}
	loaderThread.join();
}

void MenuStateLoadGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	Lang &lang= Lang::getInstance();

	if (confirmMessageBox) {
		int button = 1;
		if(confirmMessageBox->mouseClick(x,y, button)) {
			if (button == 1){
				remove(getFileName().c_str());
				if(!loadGameList()) {
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
					return;
				}
				selectionChanged();
			}
			delete confirmMessageBox;
			confirmMessageBox = NULL;
		}
		return;
	}

	if((msgBox && criticalError && msgBox->mouseClick(x,y)) || buttonReturn.mouseClick(x,y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
		return;
	}

	if(msgBox) {
		if(msgBox->mouseClick(x,y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
	} else if(buttonDelete.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		confirmMessageBox = new GraphicMessageBox();
		confirmMessageBox->init(lang.get("Delete") + " " + listBoxGames.getSelectedItem() + "?",
				lang.get("Yes"), lang.get("No"));
	} else if(buttonPlayNow.mouseClick(x,y) && gs){
		soundRenderer.playFx(coreData.getClickSoundC());
		if(!loadGame()) {
			buttonPlayNow.mouseMove(1, 1);
			msgBox = new GraphicMessageBox();
			msgBox->init(Lang::getInstance().get("WaitingForConnections"), Lang::getInstance().get("Ok"));
			criticalError = false;
		}
	} else if(listBoxGames.mouseClick(x, y)){
		selectionChanged();
	}
}

void MenuStateLoadGame::mouseMove(int x, int y, const MouseState &ms){

	if (confirmMessageBox != NULL){
		confirmMessageBox->mouseMove(x,y);
		return;
	}

	if (msgBox != NULL){
		msgBox->mouseMove(x,y);
		return;
	}

	listBoxGames.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonDelete.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateLoadGame::render() {
	Renderer &renderer= Renderer::getInstance();
	if(msgBox && criticalError) {
		renderer.renderMessageBox(msgBox);
		return;
	}

	if(savedGame.get()) {
		initGameInfo();
	}

	renderer.renderLabel(&labelInfoHeader);
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		renderer.renderLabel(&labelPlayers[i]);
		renderer.renderLabel(&labelControls[i]);
		renderer.renderLabel(&labelFactions[i]);
		renderer.renderLabel(&labelTeams[i]);
		renderer.renderLabel(&labelNetStatus[i]);
	}

	renderer.renderLabel(&labelNetwork);
	renderer.renderListBox(&listBoxGames);
	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonDelete);
	renderer.renderButton(&buttonPlayNow);

	if(confirmMessageBox) {
		renderer.renderMessageBox(confirmMessageBox);
	}

	if(msgBox) {
		renderer.renderMessageBox(msgBox);
	}

}

void MenuStateLoadGame::update() {
	if (!gs) {
		return;
	}

	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();
	Lang& lang = Lang::getInstance();

	foreach(const shared_ptr<GameSettings::Faction> &f, gs->getFactions()) {
		int slot = f->getMapSlot();
		if (f->getControlType() == CT_NETWORK) {
			RemoteClientInterface* client = serverInterface->findClientForMapSlot(slot);

			if (client && client->getSocket()) {
				labelNetStatus[slot].setText(client->getDescription());
			} else {
				labelNetStatus[slot].setText(lang.get("NotConnected"));
			}
		} else {
			labelNetStatus[slot].setText("");
		}
	}
}

// ============ misc ===========================

void MenuStateLoadGame::setGameInfo(const string &fileName, shared_ptr<const XmlNode> &root, const string &err) {
	MutexLock lock(mutex);
	if(this->fileName != fileName) {
		savedGame.reset();
		return;
	}

	savedGame = root;

	if(!root.get()) {
		labelInfoHeader.setText(err);
		for(int i=0; i<GameConstants::maxPlayers; ++i){
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}
	}
}

// ============ PRIVATE ===========================

bool MenuStateLoadGame::loadGameList() {
	try {
		findAll("savegames/*.sav", fileNames, true);
	} catch (exception e){
		fileNames.clear();
		prettyNames.clear();
		return false;
	}

	prettyNames.resize(fileNames.size());

	for(int i = 0; i < fileNames.size(); ++i){
		prettyNames[i] = formatString(fileNames[i]);
	}
	listBoxGames.setItems(prettyNames);
	return true;
}

bool MenuStateLoadGame::loadGame() {
	XmlNode *root;
	ServerInterface* serverInterface = NULL;

	if(!gs) {
		return false;
	}

	foreach(const shared_ptr<GameSettings::Faction> &f, gs->getFactions()) {
		if (f->getControlType() == CT_NETWORK) {
			if(!serverInterface) {
				serverInterface = NetworkManager::getInstance().getServerInterface();
				serverInterface->setGameSettings(gs);
			}

			RemoteClientInterface* client = serverInterface->findClientForMapSlot(f->getMapSlot());
			if(!(client && client->isConnected())) {
				return false;
			}
		}
	}

	root = XmlIo::getInstance().load(getFileName());

	if(serverInterface) {
//		serverInterface->launchGame(*gs, getFileName());
		serverInterface->setResumeSavedGame(getFileName());
		throw runtime_error("restoring network games is broken.  Fix me!");
	}

	program.setState(new Game(program, gs, root));
	return true;
}

string MenuStateLoadGame::getFileName() {
	return "savegames/" + fileNames[listBoxGames.getSelectedItemIndex()] + ".sav";
}

void MenuStateLoadGame::selectionChanged() {
	{
		MutexLock lock(mutex);
		fileName = getFileName();
		labelInfoHeader.setText("Loading...");
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}
	}
	loaderThread.load(fileName);
}

void MenuStateLoadGame::initGameInfo() {
	const Lang &lang = Lang::getInstance();
	bool good = true;
	try {
		gs = shared_ptr<GameSettings>(new GameSettings(*savedGame->getChild("settings")));
		// override .ini settings with current.
		gs->readLocalConfig();
		string techPath = gs->getTechPath();
		string tilesetPath = gs->getTilesetPath();
		string mapPath = gs->getMapPath();
		string scenarioPath = gs->getScenarioPath();
		int elapsedSeconds = savedGame->getChild("world")->getChildIntValue("frameCount") / 60;
		int elapsedMinutes = elapsedSeconds / 60;
		int elapsedHours = elapsedMinutes / 60;
		elapsedSeconds = elapsedSeconds % 60;
		elapsedMinutes = elapsedMinutes % 60;
		char elapsedTime[0x100];
		loadMapInfo(mapPath, &mapInfo);
/*
		if(techTree.size() > strlen("techs/")) {
			techTree.erase(0, strlen("techs/"));
		}
		if(tileset.size() > strlen("tilesets/")) {
			tileset.erase(0, strlen("tilesets/"));
		}
		if(map.size() > strlen("maps/")) {
			map.erase(0, strlen("maps/"));
		}
		if(map.size() > strlen(".gbm")) {
			map.resize(map.size() - strlen(".gbm"));
		}*/
		if(elapsedHours) {
			sprintf(elapsedTime, "%d:%02d:%02d", elapsedHours, elapsedMinutes, elapsedSeconds);
		} else {
			sprintf(elapsedTime, "%02d:%02d", elapsedMinutes, elapsedSeconds);
		}

		string mapDescr = " (Max Players: " + Conversion::toStr(mapInfo.players)
				+ ", Size: " + Conversion::toStr(mapInfo.size.x) + " x " + Conversion::toStr(mapInfo.size.y) + ")";

		labelInfoHeader.setText(listBoxGames.getSelectedItem() + ": " + gs->getDescription()
				+ "\nTech Tree: " + formatString(basename(techPath))
				+ "\nTileset: " + formatString(basename(tilesetPath))
				+ "\nMap: " + formatString(basename(cutLastExt(mapPath))) + mapDescr
				+ "\nScenario: " + formatString(basename(scenarioPath))
				+ "\nElapsed Time: " + elapsedTime);

		if(gs->getFactionCount() > GameConstants::maxFactions) {
			stringstream str;
			str << "Invalid faction count (" << gs->getFactionCount() << ") in saved game.";
			throw runtime_error(str.str());
		}

		const GameSettings::Factions &factions = gs->getFactions();
		for(GameSettings::Factions::const_iterator i = factions.begin(); i != factions.end(); ++i) {
			int id = (*i)->getId();
			int ct = (*i)->getControlType();
			//beware the buffer overflow -- it's possible for others to send
			//saved game files that are intended to exploit buffer overruns
			if(ct >= CT_COUNT || ct < 0) {
				throw runtime_error("Invalid control type (" + intToStr(ct)
						+ ") in saved game.");
			}

			labelPlayers[id].setText(string("Player ") + intToStr(id));
			labelControls[id].setText(lang.get(enumControlTypeDesc[ct]));
			labelFactions[id].setText((*i)->getTypeName());
			labelTeams[id].setText(intToStr((*i)->getTeam().getId()));
			labelNetStatus[id].setText("");
		}
		for(int i = factions.size(); i < GameConstants::maxFactions; ++i) {
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}

	} catch (exception &e) {
		labelInfoHeader.setText(string("Bad game file.\n") + e.what());
		for(int i = 0; i < GameConstants::maxFactions; ++i){
			labelPlayers[i].setText("");
			labelControls[i].setText("");
			labelFactions[i].setText("");
			labelTeams[i].setText("");
			labelNetStatus[i].setText("");
		}
		good = false;
	}

	if(good) {
		updateNetworkSlots();
	}

	savedGame.reset();
}

} // end namespace
