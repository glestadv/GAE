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

namespace Glest{ namespace Game{

using namespace	Shared::Xml;

// =====================================================
// 	class MenuStateScenario
// =====================================================

MenuStateScenario::MenuStateScenario(Program &program, MainMenu *mainMenu):
    MenuState(program, mainMenu, "scenario")
{
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
    vector<string> results;

    //MERGE ADD : related to the parameter
    //this->dir = dir;
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
	findAll("scenarios/*.", results);
	categories= results;
	
	if(results.size()==0){
        throw runtime_error("There are no categories");
	}

	listBoxCategory.setItems(results);
	updateScenarioList( categories[listBoxCategory.getSelectedItemIndex()] );

	networkManager.init(nrServer);
}


void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu)); //TO CHANGE
    }
	else if(buttonPlayNow.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		//MERGE DELETE START
		/*
		GameSettings *gameSettings= new GameSettings();
        loadGameSettings(&scenarioInfo, gameSettings);
		program.setState(new Game(program, *gameSettings));
		*/
		//MERGE DELETE END
        //MERGE ADD
        launchGame();
	}
    else if(listBoxScenario.mouseClick(x, y)){
        loadScenarioInfo( scenarioFiles[listBoxScenario.getSelectedItemIndex()], &scenarioInfo );
        labelInfo.setText(scenarioInfo.desc);
	}
	else if(listBoxCategory.mouseClick(x, y)){
        updateScenarioList( categories[listBoxCategory.getSelectedItemIndex()] );
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState &ms){

	listBoxScenario.mouseMove(x, y);
	listBoxCategory.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateScenario::render(){

	Renderer &renderer= Renderer::getInstance();

	renderer.renderLabel(&labelInfo);

	renderer.renderLabel(&labelCategory);
	renderer.renderListBox(&listBoxCategory);

	renderer.renderLabel(&labelScenario);
	renderer.renderListBox(&listBoxScenario);

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonPlayNow);
}

//MERGE ADD START
void MenuStateScenario::update(){
	/*
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateScenario(this);
	}
	*/
}

void MenuStateScenario::launchGame(){
	GameSettings gameSettings;
    loadGameSettings(&scenarioInfo, &gameSettings);
	program.setState(new Game(program, gameSettings));
}

void MenuStateScenario::setScenario(int i){
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo( scenarioFiles[listBoxScenario.getSelectedItemIndex()], &scenarioInfo );
}
//MERGE ADD END

void MenuStateScenario::updateScenarioList(const string category){
	vector<string> results;

	findAll("scenarios/" + category + "/*.", results);

	//update scenarioFiles
	scenarioFiles= results;
	if(results.size()==0){
        throw runtime_error("There are no scenarios for category, " + category + ".");
	}
	for(int i= 0; i<results.size(); ++i){
		results[i]= formatString(results[i]);
	}
    listBoxScenario.setItems(results);

	//update scenario info
	loadScenarioInfo( scenarioFiles[listBoxScenario.getSelectedItemIndex()], &scenarioInfo );
    labelInfo.setText(scenarioInfo.desc);
}

void MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo){

    Lang &lang= Lang::getInstance();

    XmlTree xmlTree;
	//MERGE it was being duplicated in the two cases
	xmlTree.load("scenarios/"+categories[listBoxCategory.getSelectedItemIndex()]+"/"+file+"/"+file+".xml");

    const XmlNode *scenarioNode= xmlTree.getRootNode();
	const XmlNode *difficultyNode= scenarioNode->getChild("difficulty");
	scenarioInfo->difficulty = difficultyNode->getAttribute("value")->getIntValue();
	if( scenarioInfo->difficulty < dVeryEasy || scenarioInfo->difficulty > dInsane )
	{
		throw std::runtime_error("Invalid difficulty");
	}

	const XmlNode *playersNode= scenarioNode->getChild("players");
    for(int i= 0; i<GameConstants::maxPlayers; ++i){
        const XmlNode* playerNode = playersNode->getChild("player", i);
        ControlType factionControl = strToControllerType( playerNode->getAttribute("control")->getValue() );
        string factionTypeName;

        scenarioInfo->factionControls[i] = factionControl;

        if(factionControl != ctClosed){
            int teamIndex = playerNode->getAttribute("team")->getIntValue();

            if( teamIndex < 1 || teamIndex > GameConstants::maxPlayers )
            {
                throw runtime_error("Team out of range: " + intToStr(teamIndex) );
            }

            scenarioInfo->teams[i]= playerNode->getAttribute("team")->getIntValue();
            scenarioInfo->factionTypeNames[i]= playerNode->getAttribute("faction")->getValue();
        }

        scenarioInfo->mapName = scenarioNode->getChild("map")->getAttribute("value")->getValue();
        scenarioInfo->tilesetName = scenarioNode->getChild("tileset")->getAttribute("value")->getValue();
        scenarioInfo->techTreeName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();
        //MERGE ADD START
        scenarioInfo->defaultUnits = scenarioNode->getChild("default-units")->getAttribute("value")->getBoolValue();
        scenarioInfo->defaultResources = scenarioNode->getChild("default-resources")->getAttribute("value")->getBoolValue();
        scenarioInfo->defaultVictoryConditions = scenarioNode->getChild("default-victory-conditions")->getAttribute("value")->getBoolValue();
        //MERGE ADD END
    }

	//add player info
    scenarioInfo->desc= lang.get("Player") + ": ";
	for(int i=0; i<GameConstants::maxPlayers; ++i )
	{
		if(scenarioInfo->factionControls[i] == ctHuman )
		{
			scenarioInfo->desc+= formatString(scenarioInfo->factionTypeNames[i]);
			break;
		}
	}

	//add misc info
	string difficultyString = "Difficulty" + intToStr(scenarioInfo->difficulty);

	scenarioInfo->desc+= "\n";
    scenarioInfo->desc+= lang.get("Difficulty") + ": " + lang.get(difficultyString) +"\n";
    scenarioInfo->desc+= lang.get("Map") + ": " + formatString(scenarioInfo->mapName) + "\n";
    scenarioInfo->desc+= lang.get("Tileset") + ": " + formatString(scenarioInfo->tilesetName) + "\n";
	scenarioInfo->desc+= lang.get("TechTree") + ": " + formatString(scenarioInfo->techTreeName) + "\n";
}

void MenuStateScenario::loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings){

	gameSettings->setDescription(formatString(scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
	gameSettings->setMap( scenarioInfo->mapName );
    gameSettings->setTileset( scenarioInfo->tilesetName );
    gameSettings->setTech( scenarioInfo->techTreeName );
	//MERGE ADD START
	gameSettings->setScenario(scenarioFiles[listBoxScenario.getSelectedItemIndex()]);
	gameSettings->setScenarioDir("scenarios/" + categories[listBoxCategory.getSelectedItemIndex()] + "/" + gameSettings->getScenario());
	gameSettings->setDefaultUnits(scenarioInfo->defaultUnits);
	gameSettings->setDefaultResources(scenarioInfo->defaultResources);
	gameSettings->setDefaultVictoryConditions(scenarioInfo->defaultVictoryConditions);
	//MERGE ADD END

	int factionCount= 0;
    for(int i=0; i<GameConstants::maxPlayers; ++i){
        ControlType ct= static_cast<ControlType>(scenarioInfo->factionControls[i]);
		if(ct!=ctClosed){
			if(ct==ctHuman){
				gameSettings->setThisFactionIndex(factionCount);
			}
			gameSettings->setFactionControl(factionCount, ct);
            gameSettings->setTeam(factionCount, scenarioInfo->teams[i]-1);
			gameSettings->setStartLocationIndex(factionCount, i);
            gameSettings->setFactionTypeName(factionCount, scenarioInfo->factionTypeNames[i]);
			factionCount++;
		}
    }

	gameSettings->setFactionCount(factionCount);
}

ControlType MenuStateScenario::strToControllerType(const string &str){
    if(str=="closed"){
        return ctClosed;
    }
    else if(str=="cpu"){
	    return ctCpu;
    }
    else if(str=="cpu-ultra"){
        return ctCpuUltra;
    }
    else if(str=="human"){
	    return ctHuman;
    }

    throw std::runtime_error("Unknown controller type: " + str);
}

}}//end namespace
