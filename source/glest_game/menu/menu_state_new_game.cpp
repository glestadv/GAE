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

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace Shared::Util;
using namespace Gooey;

// =====================================================
// 	class PlayerSlot
// =====================================================

PlayerSlot::PlayerSlot() 
		: Panel(Core::Rectangle(50, 50, 600, 100), 0, "")
		, txtPlayer(0, "Player")
		, txtNetStatus(0, "net_status")
		, cmbControl(0, Core::Rectangle(0, 0, 0, 0)) 
		, cmbFaction(0, Core::Rectangle(0, 0, 0, 0))
		, spinnerTeam(0,1) {

	Lang &lang = Lang::getInstance();

	this->addChildWindow(&txtPlayer);
	txtPlayer.setSize(Core::Vector2(100,20));

	spinnerTeam.setSize(Core::Vector2(30, 30));

	initComboBox(&cmbControl);
	for(int i = 0; i < ctCount; ++i) {
	    cmbControl.addString(lang.get(controlTypeNames[i]));
	}
	cmbControl.selectStringAt(0);

	initComboBox(&cmbFaction);
	cmbFaction.addString("magic");

	this->addChildWindow(&spinnerTeam);
	this->addChildWindow(&txtNetStatus);

	this->arrangeChildren();
}


void PlayerSlot::reloadFactions(vector<std::string> factions) {
/* TODO: figure out how to do this
	for(int i = 0; i < GameConstants::maxPlayers; ++i){
		listBoxFactions[i].setItems(results);
		listBoxFactions[i].pushBackItem(Lang::getInstance().get("Random"));
		listBoxFactions[i].setSelectedItemIndex(i % results.size());
    }*/
}

void PlayerSlot::setControl(ControlType ct) {
	cmbControl.selectStringAt(ct);
}

ControlType PlayerSlot::getControl() {
	return static_cast<ControlType>( cmbControl.selectedIndex() );
}

int PlayerSlot::getTeam() {
	return spinnerTeam.value();
}

int PlayerSlot::getFactionIndex() {
	return cmbFaction.selectedIndex();
}

// =====================================================
// 	class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots) 
		: MenuStateStartGameBase(program, mainMenu, "new-game")
		, Panel(VRectangle(0, 0, 500, 500), 0, "")
		// combo boxes
		, cmbMap(0, Core::Rectangle(0, 0, 0, 0))
		, cmbTechTree(0, Core::Rectangle(0, 0, 0, 0))
		, cmbTileset(0, Core::Rectangle(0, 0, 0, 0))
		// check boxes
		, chkRandomize(0, "Randomize")
		// headings
		, txtControlHeading(0, "control_heading")
		, txtFactionHeading(0, "faction_heading")
		, txtTeamHeading(0, "team_heading")
		// buttons
		, btnReturn(0, "")
		, btnPlayNow(0, "")
		// texts
		, txtMap(0, "map")
		, txtTechTree(0, "tech_tree") 
		, txtTileset(0, "Tileset")
		, txtNetwork(0, "network_info")
		, txtRandomize(0, "Randomize")
		, txtMapInfo(0, "map_info") {
	
	Lang &lang = Lang::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();

	txtMap.setText(lang.get("Map"));
	txtMap.setSize(Core::Vector2(100,20));
	txtTileset.setText(lang.get("Tileset"));
	txtTileset.setSize(Core::Vector2(100,20));
	txtTechTree.setText(lang.get("TechTree"));
	txtTechTree.setSize(Core::Vector2(100,20));

	// combo boxes and associated label
	this->addChildWindow(&txtMap);
	initFileComboBox(&cmbMap, "maps/*.gbm", mapFiles, "There are no maps", true);
	this->addChildWindow(&txtTechTree);
	initFileComboBox(&cmbTechTree, "tilesets/*.", tilesetFiles, "There are no tile sets");
	this->addChildWindow(&txtTileset);
	initFileComboBox(&cmbTileset, "techs/*.", techTreeFiles, "There are no tech trees");

	//headings
	txtControlHeading.setText(lang.get("Control"));
	txtControlHeading.setSize(Core::Vector2(100,20));
	this->addChildWindow(&txtControlHeading);
    txtFactionHeading.setText(lang.get("Faction"));
	txtFactionHeading.setSize(Core::Vector2(100,20));
	this->addChildWindow(&txtFactionHeading);
    txtTeamHeading.setText(lang.get("Team"));
	txtTeamHeading.setSize(Core::Vector2(100,20));
	this->addChildWindow(&txtTeamHeading);

	// create player slots
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		playerSlots.push_back( new PlayerSlot() );
	}

	playerSlots[0]->setControl(ctHuman);

	// add player slots to window
	vector<PlayerSlot*>::iterator iter;
	for(iter = playerSlots.begin(); iter != playerSlots.end(); ++iter) {
		this->addChildWindow( *(iter) );
	}

	//TODO: add cmbMap items so it doesn't crash
	//loadMapInfo("maps/"+mapFiles[cmbMap.selectedIndex()]+".gbm", &mapInfo);

	txtMapInfo.setText(mapInfo.desc);

	updateControllers();
	updateNetworkSlots();

	//init controllers
	if(openNetworkSlots){
		for(int i = 1; i < mapInfo.players; ++i){
			playerSlots[i]->setControl(ctNetwork);
		}
	}
	else{
		playerSlots[1]->setControl(ctCpu);
	}

	Config::getInstance().getGsRandStartLocs() ? chkRandomize.enableChecked() : chkRandomize.disableChecked();
	this->addChildWindow( &chkRandomize );

	//initialize network interface
	networkManager.init(nrServer);
	try {
		txtNetwork.setText(lang.get("Address") + ": " + networkManager.getServerInterface()->getIp() + ":" + intToStr(GameConstants::serverPort));
	} catch(const exception &e) {
		txtNetwork.setText(lang.get("Address") + ": ? " + e.what());
	}

	// buttons
	initButton(&btnReturn, lang.get("Return"));
	initButton(&btnPlayNow, lang.get("PlayNow"));

	// arrange the panel's children according to the default flow layouter
	this->arrangeChildren();

	WindowManager::instance().addWindow(this);
}

void MenuStateNewGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	if(msgBox) {
		if(msgBox->mouseClick(x,y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
	}
	/*
	
	else if(listBoxRandomize.mouseClick(x, y)){
		Config::getInstance().setGsRandStartLocs(listBoxRandomize.getSelectedItemIndex());
	}
	else{
		for(int i=0; i<mapInfo.players; ++i){
			//ensure thet only 1 human player is present
			if(listBoxControls[i].mouseClick(x, y)){

				//look for human players
				int humanIndex1= -1;
				int humanIndex2= -1;
				for(int j=0; j<GameConstants::maxPlayers; ++j){
					ControlType ct= static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
					if(ct==ctHuman){
						if(humanIndex1==-1){
							humanIndex1= j;
						}
						else{
							humanIndex2= j;
						}
					}
				}

				//no human
				if(humanIndex1==-1 && humanIndex2==-1){
					listBoxControls[i].setSelectedItemIndex(ctHuman);
				}

				//2 humans
				if(humanIndex1!=-1 && humanIndex2!=-1){
					listBoxControls[humanIndex1==i? humanIndex2: humanIndex1].setSelectedItemIndex(ctClosed);
				}
				updateNetworkSlots();
			}
			else if(listBoxFactions[i].mouseClick(x, y)){
			}
			else if(listBoxTeams[i].mouseClick(x, y)){
			}
		}
	}*/
}

void MenuStateNewGame::mouseMove(int x, int y, const MouseState &ms) {

}

void MenuStateNewGame::render() {
	Renderer &renderer= Renderer::getInstance();

	if(msgBox != NULL){
		renderer.renderMessageBox(msgBox);
	}
}

void MenuStateNewGame::update() {
	//TOOD: add AutoTest to config
	/*
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateNewGame(program, mainMenu);
	}
	*/
	/*TODO: move to PlayerSlot?
	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();
	Lang& lang = Lang::getInstance();

	for (int i = 0; i < mapInfo.players; ++i) {
		if (listBoxControls[i].getSelectedItemIndex() == ctNetwork) {
			ConnectionSlot* connectionSlot = serverInterface->getSlot(i);

			assert(connectionSlot != NULL);

			if (connectionSlot->isConnected()) {
				labelNetStatus[i].setText(connectionSlot->getDescription());
			} else {
				labelNetStatus[i].setText(lang.get("NotConnected"));
			}
		} else {
			labelNetStatus[i].setText("");
		}
	}*/
}

void MenuStateNewGame::loadGameSettings(GameSettings *gameSettings){
	Random rand;
	rand.init(Shared::Platform::Chrono::getCurMillis());

	int factionCount= 0;

	gameSettings->setDescription(formatString(mapFiles[cmbMap.selectedIndex()]));
	gameSettings->setMap(mapFiles[cmbMap.selectedIndex()]);
    gameSettings->setTileset(tilesetFiles[cmbTileset.selectedIndex()]);
    gameSettings->setTech(techTreeFiles[cmbTechTree.selectedIndex()]);
	gameSettings->setDefaultVictoryConditions(true);
	gameSettings->setDefaultResources(true);
	gameSettings->setDefaultUnits(true);

    for(int i = 0; i < mapInfo.players; ++i){
		ControlType ct = playerSlots[i]->getControl();
		if (ct != ctClosed) {
			if (ct == ctHuman) {
				gameSettings->setThisFactionIndex(factionCount);
			}
			gameSettings->setFactionControl(factionCount, ct);
			gameSettings->setTeam(factionCount, playerSlots[i]->getTeam());
			gameSettings->setStartLocationIndex(factionCount, i);
			if (playerSlots[i]->getFactionIndex() >= factionFiles.size()) {
				gameSettings->setFactionTypeName(factionCount, factionFiles[rand.randRange(0, factionFiles.size() - 1)]);
			} else {
				gameSettings->setFactionTypeName(factionCount, factionFiles[playerSlots[i]->getFactionIndex()]);
			}
			factionCount++;
		}
    }
	gameSettings->setFactionCount(factionCount);

	if( chkRandomize.isChecked() ) {
		gameSettings->randomizeLocs(mapInfo.players);
	}
}

MenuStateNewGame::~MenuStateNewGame() {
	vector<PlayerSlot*>::iterator iter;

	/*for(iter = playerSlots.begin(); iter != playerSlots.end(); ++iter) {
			delete *(iter); //???
	}*/
}

// ============ PRIVATE ===========================

void MenuStateNewGame::initButton(Button *btn, const std::string &text) {
	// size, text, appearance, slot, add to panel
	btn->setSize(Core::Vector2(256, 32));
	btn->setText(text);
	btn->loadAppearance("data/gui/default/buttons.xml", "standard");
	//b->pressed.connect(this, &MenuStateRoot::buttonPressed);
	this->addChildWindow(btn);
}

void MenuStateNewGame::initFileComboBox(
		Gooey::ComboBox *cmb, 
		const std::string &path, 
		vector<std::string> &files, 
		const std::string &errorMsg, 
		bool cutExtension) {
	vector<string> results;

	// size
	cmb->setSize(Core::Vector2(256, 32));

	//TODO: add style

	// fetch raw file results
	findAll(path, results, cutExtension);
	if (results.size() == 0) {
		throw runtime_error(errorMsg);
	}

	// add options to combo box
	for (int i = 0; i < results.size(); ++i) {
		cmb->addString( formatString( results[i] ) );
	}

	this->addChildWindow(cmb);

	files = results;
}

void MenuStateNewGame::updateControllers() {
	/*TODO: replace with new gui components
	bool humanPlayer= false;

	for(int i= 0; i<mapInfo.players; ++i){
		if(listBoxControls[i].getSelectedItemIndex() == ctHuman){
			humanPlayer= true;
		}
	}

	if(!humanPlayer){
		listBoxControls[0].setSelectedItemIndex(ctHuman);
	}

	for(int i= mapInfo.players; i<GameConstants::maxPlayers; ++i){
		listBoxControls[i].setSelectedItemIndex(ctClosed);
	}
	
//---------------
//from mouse click?
	for(int i=0; i<mapInfo.players; ++i){
		//ensure thet only 1 human player is present
		if(listBoxControls[i].mouseClick(x, y)){

			//look for human players
			int humanIndex1= -1;
			int humanIndex2= -1;
			for(int j=0; j<GameConstants::maxPlayers; ++j){
				ControlType ct= static_cast<ControlType>(listBoxControls[j].getSelectedItemIndex());
				if(ct==ctHuman){
					if(humanIndex1==-1){
						humanIndex1= j;
					}
					else{
						humanIndex2= j;
					}
				}
			}

			//no human
			if(humanIndex1==-1 && humanIndex2==-1){
				listBoxControls[i].setSelectedItemIndex(ctHuman);
			}

			//2 humans
			if(humanIndex1!=-1 && humanIndex2!=-1){
				listBoxControls[humanIndex1==i? humanIndex2: humanIndex1].setSelectedItemIndex(ctClosed);
			}
			updateNetworkSlots();
			*/
}

bool MenuStateNewGame::isUnconnectedSlots() {
	ServerInterface* serverInterface = NetworkManager::getInstance().getServerInterface();
	for(int i = 0; i < mapInfo.players; ++i){
		if(playerSlots[i]->getControl() == ctNetwork){
			if(!serverInterface->getSlot(i)->isConnected()){
				return true;
			}
		}
	}
	return false;
}

void MenuStateNewGame::updateNetworkSlots() {
/*	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();

	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(serverInterface->getSlot(i) == NULL && playerSlots[i]->getControl() == ctNetwork){
			serverInterface->addSlot(i);
		}
		if(serverInterface->getSlot(i) != NULL && playerSlots[i]->getControl() != ctNetwork){
			serverInterface->removeSlot(i);
		}
	}
	*/
}

void MenuStateNewGame::buttonClicked() {
	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	ServerInterface *serverInterface = NetworkManager::getInstance().getServerInterface();

	if ( btnReturn.state() == Button::State::down ) {
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    } else if( btnPlayNow.state() == Button::State::down ) {
		if ( isUnconnectedSlots() ) {
			//btnPlayNow.mouseMove(1, 1);
			msgBox = new GraphicMessageBox();
			msgBox->init(Lang::getInstance().get("WaitingForConnections"), Lang::getInstance().get("Ok"));
		} else {
			GameSettings gameSettings;

			Config::getInstance().save();
			soundRenderer.playFx(coreData.getClickSoundC());
			loadGameSettings(&gameSettings);
			serverInterface->launchGame(&gameSettings);
			program.setState(new Game(program, gameSettings));
		}
	}
}

void MenuStateNewGame::mapSelected() {
	loadMapInfo("maps/"+mapFiles[cmbMap.selectedIndex()]+".gbm", &mapInfo);
	txtMapInfo.setText(mapInfo.desc);
	updateControllers();
}

void MenuStateNewGame::techSelected() {
	vector<string> results;

	findAll("techs/" + techTreeFiles[cmbTechTree.selectedIndex()] + "/factions/*.", results);

	if (results.size() == 0) {
        throw runtime_error("There is no factions for this tech tree");
	}
	factionFiles.clear();
	factionFiles = results;
   	for(int i = 0; i < results.size(); ++i){
		results[i] = formatString(results[i]);
	}

	// apply new factions to player slots
	vector<PlayerSlot*>::iterator iter;
	for(iter = playerSlots.begin(); iter != playerSlots.end(); ++iter) {
		PlayerSlot *ps = *(iter);
		ps->reloadFactions(results);
	}
}

}}//end namespace
