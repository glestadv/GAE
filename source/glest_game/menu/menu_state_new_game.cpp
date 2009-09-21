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

class PlayerSlot : public Gooey::Panel {
	// Texts
	Gooey::StaticText txtPlayer;

	// Combo boxes
	Gooey::ComboBox cmbControl;
	Gooey::ComboBox cmbFaction;
	Gooey::SpinBox spinnerTeam;

	/*
	//GraphicLabel labelPlayers[GameConstants::maxPlayers];
	//GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	*/
	static int humanIndex1, humanIndex2;

	void initComboBox(Gooey::ComboBox *cmb) {
		cmb->setSize(Core::Vector2(156, 32));

		this->addChildWindow(cmb);
	}
	
public:
	PlayerSlot() 
		: Panel(Core::Rectangle(50, 50, 490, 100), 0, "")
		, txtPlayer(0, "Player")
		, cmbControl(0, Core::Rectangle(0, 0, 0, 0)) 
		, cmbFaction(0, Core::Rectangle(0, 0, 0, 0))
		, spinnerTeam(0,1) {

			this->addChildWindow(&txtPlayer);

			initComboBox( &cmbControl );
			cmbControl.addString("human");

			initComboBox( &cmbFaction );
			cmbFaction.addString("magic");

			this->addChildWindow(&spinnerTeam);

			this->arrangeChildren();
	}

public:
	// Events
	// - controlSelected

};

class GUITemp : public Gooey::Panel {
private:
	/* Buttons
	//GraphicButton buttonReturn;
	//GraphicButton buttonPlayNow;
	*/
	Gooey::Button btnReturn;
	Gooey::Button btnPlayNow;

	/* File Locations */
	vector<string> mapFiles;
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;

	vector<PlayerSlot*> playerSlots;
	

	/* ComboBoxes */
	Gooey::ComboBox cmbMap;
	Gooey::ComboBox cmbTechTree;
	Gooey::ComboBox cmbTileset;
	

	/* CheckBoxes
	Gooey::CheckBox chkRandomize;
	*/

	/* Label Slot Headings
	Gooey::StaticText txtControlHeading;
	Gooey::StaticText txtFactionHeading;
	Gooey::StaticText txtTeamHeading;
	*/

	/* StaticText */
	Gooey::StaticText txtMap;
	Gooey::StaticText txtTechTree;
	Gooey::StaticText txtTileset;
	/*
	Gooey::StaticText txtNetwork;
	Gooey::StaticText txtRandomize;

	Gooey::StaticText txtMapInfo; //WARN: might not be multi-line
	*/

	//MapInfo mapInfo;
	//GraphicMessageBox *msgBox;

	/* NEW DESIGN IDEAS
	- Show image of map
	- increase amount of possible players

	*/

	void GUITemp::initButton(Button *btn, const std::string &text) {
		// size, text, appearance, slot, add to panel
		btn->setSize(Core::Vector2(256, 32));
		btn->setText(text);
		btn->loadAppearance("data/gui/default/buttons.xml", "standard");
		//b->pressed.connect(this, &MenuStateRoot::buttonPressed);
		this->addChildWindow(btn);
	}

public:
	GUITemp() 
		: Panel(VRectangle(0, 0, 500, 500), 0, "")
		, cmbMap(0, Core::Rectangle(0, 0, 0, 0))
		, cmbTechTree(0, Core::Rectangle(0, 0, 0, 0))
		, cmbTileset(0, Core::Rectangle(0, 0, 0, 0)) 
		, btnReturn(0, "")
		, btnPlayNow(0, "")
		, txtMap(0, "Map")
		, txtTechTree(0, "Tech Tree") 
		, txtTileset(0, "Tileset") {
		/*
	labelMap.setText(lang.get("Map"));
	labelTileset.setText(lang.get("Tileset"));
	labelTechTree.setText(lang.get("TechTree"));
	//headings
	labelControl.setText(lang.get("Control"));
    labelFaction.setText(lang.get("Faction"));
    labelTeam.setText(lang.get("Team"));		  
	*/
			Lang &lang = Lang::getInstance();

			// combo boxes
			this->addChildWindow(&txtMap);
			initFileComboBox(&cmbMap, "maps/*.gbm", mapFiles, "There are no maps", true);
			this->addChildWindow(&txtTechTree);
			initFileComboBox(&cmbTechTree, "tilesets/*.", tilesetFiles, "There are no tile sets");
			this->addChildWindow(&txtTileset);
			initFileComboBox(&cmbTileset, "techs/*.", techTreeFiles, "There are no tech trees");

			// buttons
			initButton(&btnReturn, lang.get("Return"));
			initButton(&btnPlayNow, lang.get("PlayNow"));

			// create player slots
			for(int i = 0; i < GameConstants::maxPlayers; ++i) {
				playerSlots.push_back( new PlayerSlot() );
			}

			vector<PlayerSlot*>::iterator iter;
			for(iter = playerSlots.begin(); iter != playerSlots.end(); ++iter) {
				this->addChildWindow( *(iter) );
			}

			// arrange the panel's children according to the default flow layouter
			this->arrangeChildren();

	/*
	labelControl.init(300, 550, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.init(500, 550, GraphicListBox::defW, GraphicListBox::defH, true);
    labelTeam.init(700, 550, 60, GraphicListBox::defH, true);
	*/

			  
	}

	~GUITemp() {
		vector<PlayerSlot*>::iterator iter;

		/*for(iter = playerSlots.begin(); iter != playerSlots.end(); ++iter) {
				delete *(iter); //???
		}*/
	}

    void initFileComboBox(
			Gooey::ComboBox *cmb, 
			const std::string &path, 
			vector<std::string> &files, 
			const std::string &errorMsg, 
			bool cutExtension = false) {
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

public:
	// Events
	// - button press
	// -- start game
	// -- return
	// - reload factions
	// - reload player slots

	void reloadFactions() {

	}

	/*
	bool handleReturnClick(const CEGUI::EventArgs& e);
	bool handlePlayClick(const CEGUI::EventArgs& e);
	bool handleMapSelected(const CEGUI::EventArgs& e);
	bool handleTechSelected(const CEGUI::EventArgs& e);
	*/

};

Panel *guiTemp = NULL;

// =====================================================
// 	class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program &program, MainMenu *mainMenu, bool openNetworkSlots) :
		MenuStateStartGameBase(program, mainMenu, "new-game") {

			guiTemp = new GUITemp();

			WindowManager::instance().addWindow(guiTemp);

	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();

	vector<string> results, teamItems, controlItems;

	//create
	buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

    //map listBox
    findAll("maps/*.gbm", results, true);
	if(results.size()==0){
        throw runtime_error("There are no maps");
	}
	mapFiles= results;
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
    listBoxMap.init(200, 320, 150);
    listBoxMap.setItems(results);
	labelMap.init(200, 350);
	labelMapInfo.init(200, 290, 200, 40);

    //tileset listBox
    findAll("tilesets/*.", results);
	if(results.size()==0){
        throw runtime_error("There are no tile sets");
	}
    tilesetFiles= results;
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
	listBoxTileset.init(400, 320, 150);
    listBoxTileset.setItems(results);
	labelTileset.init(400, 350);

    //tech Tree listBox
    findAll("techs/*.", results);
	if(results.size()==0){
        throw runtime_error("There are no tech trees");
	}
    techTreeFiles= results;
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
	listBoxTechTree.init(600, 320, 150);
    listBoxTechTree.setItems(results);
	labelTechTree.init(600, 350);

	//list boxes
    for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].init(200, 500-i*30);
        listBoxControls[i].init(300, 500-i*30);
        listBoxFactions[i].init(500, 500-i*30);
		listBoxTeams[i].init(700, 500-i*30, 60);
		labelNetStatus[i].init(800, 500-i*30, 60);
    }

	labelControl.init(300, 550, GraphicListBox::defW, GraphicListBox::defH, true);
    labelFaction.init(500, 550, GraphicListBox::defW, GraphicListBox::defH, true);
    labelTeam.init(700, 550, 60, GraphicListBox::defH, true);

	//texts
	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

	for(int i = 0; i < ctCount; ++i) {
	    controlItems.push_back(lang.get(controlTypeNames[i]));
	}
	teamItems.push_back("1");
	teamItems.push_back("2");
	teamItems.push_back("3");
	teamItems.push_back("4");

	reloadFactions();

	findAll("techs/"+techTreeFiles[listBoxTechTree.getSelectedItemIndex()]+"/factions/*.", results);
	if(results.size()==0){
        throw runtime_error("There are no factions for this tech tree");
	}

	for(int i=0; i<GameConstants::maxPlayers; ++i){
		labelPlayers[i].setText(lang.get("Player")+" "+intToStr(i));
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

	loadMapInfo("maps/"+mapFiles[listBoxMap.getSelectedItemIndex()]+".gbm", &mapInfo);

	labelMapInfo.setText(mapInfo.desc);

	//initialize network interface
	networkManager.init(nrServer);
	labelNetwork.init(50, 50);
	try {
		labelNetwork.setText(lang.get("Address") + ": " + networkManager.getServerInterface()->getIp() + ":" + intToStr(GameConstants::serverPort));
	} catch(const exception &e) {
		labelNetwork.setText(lang.get("Address") + ": ? " + e.what());
	}

	//init controllers
	listBoxControls[0].setSelectedItemIndex(ctHuman);
	if(openNetworkSlots){
		for(int i= 1; i<mapInfo.players; ++i){
			listBoxControls[i].setSelectedItemIndex(ctNetwork);
		}
	}
	else{
		listBoxControls[1].setSelectedItemIndex(ctCpu);
	}
	updateControlers();
	updateNetworkSlots();

	labelRandomize.init(200, 500 - GameConstants::maxPlayers * 30);
	labelRandomize.setText(lang.get("RandomizeLocations"));
	listBoxRandomize.init(332, 500 - GameConstants::maxPlayers * 30, 75);
	listBoxRandomize.pushBackItem(lang.get("No"));
	listBoxRandomize.pushBackItem(lang.get("Yes"));
	listBoxRandomize.setSelectedItemIndex(Config::getInstance().getGsRandStartLocs() ? 1 : 0);

	//msgBox = NULL;
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
	else if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(buttonPlayNow.mouseClick(x,y)){
		if(isUnconnectedSlots()) {
			buttonPlayNow.mouseMove(1, 1);
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
	else if(listBoxMap.mouseClick(x, y)){
		loadMapInfo("maps/"+mapFiles[listBoxMap.getSelectedItemIndex()]+".gbm", &mapInfo);
		labelMapInfo.setText(mapInfo.desc);
		updateControlers();
	}
	else if(listBoxTileset.mouseClick(x, y)){
	}
	else if(listBoxTechTree.mouseClick(x, y)){
		reloadFactions();
	}
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
	}
}

void MenuStateNewGame::mouseMove(int x, int y, const MouseState &ms){

	if (msgBox != NULL){
		msgBox->mouseMove(x,y);
		return;
	}

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);

	for(int i=0; i<GameConstants::maxPlayers; ++i){
        listBoxControls[i].mouseMove(x, y);
        listBoxFactions[i].mouseMove(x, y);
		listBoxTeams[i].mouseMove(x, y);
    }
	listBoxMap.mouseMove(x, y);
	listBoxTileset.mouseMove(x, y);
	listBoxTechTree.mouseMove(x, y);
	listBoxRandomize.mouseMove(x, y);
}

void MenuStateNewGame::render(){

	Renderer &renderer= Renderer::getInstance();

	int i;

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);

	for(i=0; i<GameConstants::maxPlayers; ++i){
		renderer.renderLabel(&labelPlayers[i]);
        renderer.renderListBox(&listBoxControls[i]);
		if(listBoxControls[i].getSelectedItemIndex()!=ctClosed){
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
	}
}

void MenuStateNewGame::loadGameSettings(GameSettings *gameSettings){
	Random rand;
	rand.init(Shared::Platform::Chrono::getCurMillis());

	int factionCount= 0;

	gameSettings->setDescription(formatString(mapFiles[listBoxMap.getSelectedItemIndex()]));
	gameSettings->setMap(mapFiles[listBoxMap.getSelectedItemIndex()]);
    gameSettings->setTileset(tilesetFiles[listBoxTileset.getSelectedItemIndex()]);
    gameSettings->setTech(techTreeFiles[listBoxTechTree.getSelectedItemIndex()]);
	gameSettings->setDefaultVictoryConditions(true);
   gameSettings->setDefaultResources (true);
   gameSettings->setDefaultUnits (true);
  

    for(int i=0; i<mapInfo.players; ++i){
		ControlType ct= static_cast<ControlType>(listBoxControls[i].getSelectedItemIndex());
		if(ct!=ctClosed){
			if(ct==ctHuman){
				gameSettings->setThisFactionIndex(factionCount);
			}
			gameSettings->setFactionControl(factionCount, ct);
			gameSettings->setTeam(factionCount, listBoxTeams[i].getSelectedItemIndex());
			gameSettings->setStartLocationIndex(factionCount, i);
			if(listBoxFactions[i].getSelectedItemIndex() >= factionFiles.size()) {
				gameSettings->setFactionTypeName(factionCount, factionFiles[rand.randRange(0, factionFiles.size() - 1)]);
			} else {
				gameSettings->setFactionTypeName(factionCount, factionFiles[listBoxFactions[i].getSelectedItemIndex()]);
			}
			factionCount++;
		}
    }
	gameSettings->setFactionCount(factionCount);

	if(listBoxRandomize.getSelectedItemIndex()) {
		gameSettings->randomizeLocs(mapInfo.players);
	}
}

// ============ PRIVATE ===========================


void MenuStateNewGame::reloadFactions(){

	vector<string> results;

	findAll("techs/"+techTreeFiles[listBoxTechTree.getSelectedItemIndex()]+"/factions/*.", results);

	if(results.size()==0){
        throw runtime_error("There is no factions for this tech tree");
	}
	factionFiles.clear();
	factionFiles= results;
   	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
	for(int i=0; i<GameConstants::maxPlayers; ++i){
		listBoxFactions[i].setItems(results);
		listBoxFactions[i].pushBackItem(Lang::getInstance().get("Random"));
		listBoxFactions[i].setSelectedItemIndex(i % results.size());
    }
}

void MenuStateNewGame::updateControlers(){
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
}

bool MenuStateNewGame::isUnconnectedSlots(){
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();
	for(int i= 0; i<mapInfo.players; ++i){
		if(listBoxControls[i].getSelectedItemIndex()==ctNetwork){
			if(!serverInterface->getSlot(i)->isConnected()){
				return true;
			}
		}
	}
	return false;
}

void MenuStateNewGame::updateNetworkSlots(){
	ServerInterface* serverInterface= NetworkManager::getInstance().getServerInterface();


	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		if(serverInterface->getSlot(i)==NULL && listBoxControls[i].getSelectedItemIndex()==ctNetwork){
			serverInterface->addSlot(i);
		}
		if(serverInterface->getSlot(i) != NULL && listBoxControls[i].getSelectedItemIndex()!=ctNetwork){
			serverInterface->removeSlot(i);
		}
	}
}

}}//end namespace
