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
#include "menu_state_join_game.h"

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
#include "game.h"
#include "socket.h"

#include "leak_dumper.h"


namespace Game {

using namespace Shared::Util;

// ===============================
//  class MenuStateJoinGame
// ===============================

const int MenuStateJoinGame::newServerIndex = 0;
const string MenuStateJoinGame::serverFileName = "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect, IpAddress serverIp)
		: MenuState(program, mainMenu, "join-game")
		, msgBox(NULL) {
	Lang &lang = Lang::getInstance();
	Config &config = Config::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();

	servers.load(serverFileName);

	//buttons
	buttonReturn.init(325, 270, 125);
	buttonReturn.setText(lang.get("Return"));

	buttonConnect.init(475, 270, 125);
	buttonConnect.setText(lang.get("Connect"));

	//server type label
	labelServerType.init(330, 460);
	labelServerType.setText(lang.get("ServerType") + ":");

	//server type list box
	listBoxServerType.init(465, 460);
	listBoxServerType.pushBackItem(lang.get("ServerTypeNew"));
	listBoxServerType.pushBackItem(lang.get("ServerTypePrevious"));

	//server label
	labelServer.init(330, 430);
	labelServer.setText(lang.get("Server") + ": ");

	//server listbox
	listBoxServers.init(465, 430);

	const Properties::PropertyMap &pm = servers.getPropertyMap();
	for (Properties::PropertyMap::const_iterator i = pm.begin(); i != pm.end(); ++i) {
		listBoxServers.pushBackItem(i->first);
	}

	//server ip
	labelServerIp.init(465, 430);

	labelStatus.init(330, 400);
	labelStatus.setText("");

	labelInfo.init(330, 370);
	labelInfo.setText("");

	networkManager.init(NR_CLIENT);
	connected = false;
	playerIndex = -1;

	//server ip
	if (connect) {
		labelServerIp.setText(serverIp.toString() + "_");
		connectToServer();
	} else {
		labelServerIp.setText(config.getNetServerIp() + "_");
	}
}

void MenuStateJoinGame::mouseClick(int x, int y, MouseButton mouseButton) {
	const Lang &lang = Lang::getInstance();

	CoreData &coreData = CoreData::getInstance();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();
	NetworkManager &networkManager = NetworkManager::getInstance();
	NetworkClientMessenger* clientInterface = networkManager.getNetworkClientMessenger();

	if (msgBox) {
		if (msgBox->mouseClick(x, y)) {
			soundRenderer.playFx(coreData.getClickSoundC());
			delete msgBox;
			msgBox = NULL;
		}
		return;
	}
	if (!clientInterface->isConnected()) {
		//server type
		if (listBoxServerType.mouseClick(x, y)) {
			if (!listBoxServers.getText().empty()) {
				labelServerIp.setText(servers.getString(listBoxServers.getText()) + "_");
			}

		//server list
		} else if (listBoxServerType.getSelectedItemIndex() != newServerIndex) {
			if (listBoxServers.mouseClick(x, y)) {
				labelServerIp.setText(servers.getString(listBoxServers.getText()) + "_");
			}
		}
	}

	//return
	if (buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));

	//connect/disconnect
	} else if (buttonConnect.mouseClick(x, y)) {
		NetworkClientMessenger* clientInterface = networkManager.getNetworkClientMessenger();

		soundRenderer.playFx(coreData.getClickSoundA());
		labelInfo.setText("");

		if (clientInterface->isConnected()) {
			clientInterface->disconnectFromServer();
			buttonConnect.setText(Lang::getInstance().get("Connect"));
		} else {
			try {
				connectToServer();
			} catch(runtime_error &e) {
				msgBox = new GraphicMessageBox();
				msgBox->init(lang.get("ConnectionFailed") + "\n" + e.what(), lang.get("Ok"));
			}
			//buttonConnect.setText(Lang::getInstance().get("Disconnect"));
		}
	}
}

void MenuStateJoinGame::mouseMove(int x, int y, const MouseState &ms) {
	buttonReturn.mouseMove(x, y);
	buttonConnect.mouseMove(x, y);
	listBoxServerType.mouseMove(x, y);

	if (msgBox != NULL) {
		msgBox->mouseMove(x, y);
		return;
	}

	//hide-show options depending on the selection
	if (listBoxServers.getSelectedItemIndex() == newServerIndex) {
		labelServerIp.mouseMove(x, y);
	} else {
		listBoxServers.mouseMove(x, y);
	}
}

void MenuStateJoinGame::render() {
	Renderer &renderer = Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderLabel(&labelServer);
	renderer.renderLabel(&labelServerType);
	renderer.renderLabel(&labelStatus);
	renderer.renderLabel(&labelInfo);
	renderer.renderButton(&buttonConnect);
	renderer.renderListBox(&listBoxServerType);

	if (listBoxServerType.getSelectedItemIndex() == newServerIndex) {
		renderer.renderLabel(&labelServerIp);
	} else {
		renderer.renderListBox(&listBoxServers);
	}

	if (msgBox != NULL) {
		renderer.renderMessageBox(msgBox);
	}
}

void MenuStateJoinGame::update() {
	NetworkClientMessenger* clientInterface = NetworkManager::getInstance().getNetworkClientMessenger();
	Lang &lang = Lang::getInstance();

	//update status label
	if (clientInterface->isConnected()) {
		buttonConnect.setText(lang.get("Disconnect"));
		string statusStr;
		if (clientInterface->getState() < STATE_INITIALIZED) {
			statusStr = lang.get("WaitingHost");
			labelInfo.setText("");
		} else {
			statusStr = lang.get("Connected") + ": "
					+ clientInterface->getRemoteServerInterface().getStatusStr();

			// keep a local shared_ptr in case the network interface replaces it (in another thread)
			// before we're finished reading from it.
			const shared_ptr<GameSettings> &gssp = clientInterface->getGameSettings();
			const GameSettings &gs = *gssp;
			stringstream str;

			str << lang.get("Map") << ": " << gs.getMapPath() << endl
				<< lang.get("Tileset") << ": " << gs.getTilesetPath() << endl
				<< lang.get("TechTree") << ": " << gs.getTechPath() << endl;
			foreach(const GameSettings::Factions::value_type &f, gs.getFactions()) {
				str << lang.get("Player") << " " << (f->getMapSlot() + 1) << " - "
					<< lang.get("Team") << " " << (f->getTeam().getId() + 1) << " - "
					<< (f->isRandomType() ? lang.get("Random") : f->getTypeName()) << " - "
					<< lang.get(enumControlTypeDesc[f->getControlType()]) << endl;
			}
			labelInfo.setText(str.str());
		}
		labelStatus.setText(statusStr);

		string ipString = labelServerIp.getText();
		ipString.resize(ipString.size() - 1);
		servers.setString(clientInterface->getDescription(), IpAddress(ipString).toString());

		//launch
		if (clientInterface->getState() == STATE_LAUNCHING) {
			servers.save(serverFileName);
			program.setState(new Game(program, clientInterface->getGameSettings(), clientInterface->getSavedGame()));

			// this is now dead, so return before we blow something up :)
			return;
			/*
			if (clientInterface->getSavedGameFileName() == "") {
				program.setState(new Game(program, clientInterface->getGameSettings()));
			} else {
 				XmlNode *root = XmlIo::getInstance().load(clientInterface->getSavedGameFileName());
				program.setState(new Game(program, clientInterface->getGameSettings(), root));
			}*/
		}

		//game info changed
		if (clientInterface->isGameSettingsChanged()) {
			const GameSettings &gs = *clientInterface->getGameSettings();

		}
	} else {
		buttonConnect.setText(lang.get("Connect"));
		labelStatus.setText(lang.get("NotConnected"));
		labelInfo.setText("");
	}
}

void MenuStateJoinGame::keyDown(const Key &key) {
	NetworkClientMessenger* clientInterface = NetworkManager::getInstance().getNetworkClientMessenger();

	if (!clientInterface->isConnected()) {
		if (key == keyBackspace) {
			string text = labelServerIp.getText();

			if (text.size() > 1) {
				text.erase(text.end() - 2);
			}

			labelServerIp.setText(text);
		}
	}

	if(key == keyEscape) {
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	}
}

void MenuStateJoinGame::keyPress(char c) {
	NetworkClientMessenger* clientInterface = NetworkManager::getInstance().getNetworkClientMessenger();

	if (!clientInterface->isConnected()) {
		int maxTextSize = 16;

		if (c >= '0' && c <= '9') {

			if (labelServerIp.getText().size() < maxTextSize) {
				string text = labelServerIp.getText();

				text.insert(text.end() - 1, c);

				labelServerIp.setText(text);
			}
		} else if (c == '.') {
			if (labelServerIp.getText().size() < maxTextSize) {
				string text = labelServerIp.getText();

				text.insert(text.end() - 1, '.');

				labelServerIp.setText(text);
			}
		}
	}
}

void MenuStateJoinGame::connectToServer() {
	NetworkClientMessenger &clientInterface = *NetworkManager::getInstance().getNetworkClientMessenger();
	Config& config = Config::getInstance();
	string ipString = labelServerIp.getText();
	// remove annoying trailing underscore
	ipString.resize(ipString.size() - 1);
	IpAddress serverIp(ipString);

	clientInterface.connectToServer(serverIp, config.getNetServerPort());
	labelServerIp.setText(serverIp.toString() + '_');
	labelInfo.setText("");

	//save server ip
	config.setNetServerIp(serverIp.toString());
	config.save();
}

} // end namespace
