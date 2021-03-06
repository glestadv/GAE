// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//				  2010 James McCulloch <silnarm at gmail>
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
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "game.h"
#include "socket.h"

#include "leak_dumper.h"
#include "logger.h"

using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace Menu {

// ===============================
// 	class ConnectThread
// ===============================

ConnectThread::ConnectThread(MenuStateJoinGame &menu, Ip serverIp)
		: m_menu(menu)
		, m_server(serverIp)
		, m_connecting(true)
		, m_result(ConnectResult::INVALID) {
	start();
	cout << "ConnectThread::ConnectThread()\n";
}

ConnectThread::~ConnectThread() {
	cout << "ConnectThread::~ConnectThread()\n";
}

void ConnectThread::execute() {
	ClientInterface* clientInterface = g_simInterface.asClientInterface();
	
	try {
		clientInterface->connect(m_server, g_config.getNetServerPort());
		{
			MutexLock lock(m_mutex);
			m_result = ConnectResult::SUCCESS;
			m_connecting = false;
		}
	} catch (exception &e) {
		MutexLock lock(m_mutex);
		if (m_result == ConnectResult::INVALID) {
			m_result = ConnectResult::FAILED;
		}
		m_errorMsg = e.what();
		m_connecting = false;
	}
	m_menu.connectThreadDone(m_result); // will delete 'this', no more code beyond here!
}

void ConnectThread::cancel() {
	cout << "ConnectThread::cancel()\n";
	MutexLock lock(m_mutex);
	if (m_connecting) {
		cout << "Was connecting, forcing socket exception.\n";
		m_result = ConnectResult::CANCELLED;
		g_simInterface.asClientInterface()->reset();
	} else {
		cout << "Was not connecting.\n";
	}
}

string ConnectThread::getErrorMsg() {
	return m_errorMsg;
}

// ===============================
// 	class FindServerThread
// ===============================

void FindServerThread::execute() {
	const int MSG_SIZE = 100;
	char msg[MSG_SIZE];

	try {
		Ip serverIp = m_socket.receiveAnnounce(g_config.getNetAnnouncePort(), msg, MSG_SIZE); // blocking call
		m_menu.foundServer(serverIp);
	} catch(SocketException &e) {
		// do nothing
	}
}

// ===============================
//  class MenuStateJoinGame
// ===============================

//const int MenuStateJoinGame::newServerIndex = 0;
const string MenuStateJoinGame::serverFileName = "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program &program, MainMenu *mainMenu, bool connect, Ip serverIp)
		: MenuState(program, mainMenu)
		, m_messageBox(0) 
		, m_connectThread(0)
		, m_findServerThread(0)
		, m_connectResult(ConnectResult::INVALID)
		, m_connecting(false)
		, m_searching(false) {
	if (fileExists(serverFileName)) {
		m_servers.load(serverFileName);
	} else {
		m_servers.save(serverFileName);
	}

	program.getSimulationInterface()->changeRole(GameRole::CLIENT);
	m_connected = false;
	m_playerIndex = -1;

	buildConnectPanel();

	// connect if ip provided
	if (serverIp.getString() != "0.0.0.0") {
		m_serverTextBox->setText(serverIp.getString());
		onConnect(0);
	}
	program.setFade(0.f);
}

void MenuStateJoinGame::buildConnectPanel() {
	const int defWidgetHeight = g_widgetConfig.getDefaultItemHeight();
	const int defCellHeight = defWidgetHeight * 3 / 2;

	Vec2i pos, size(500, 300);
	pos = g_metrics.getScreenDims() / 2 - size / 2;

	m_connectPanel = new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::CENTRE, 4);
	m_connectPanel->setSizeHint(0, SizeHint(-1, defCellHeight)); // def px for recent hosts lbl & list
	m_connectPanel->setSizeHint(1, SizeHint(-1, defCellHeight)); // def px for server lbl & txtBox
	m_connectPanel->setSizeHint(2, SizeHint(-1, defCellHeight)); // def px for connected label
	m_connectPanel->setSizeHint(3, SizeHint(25));     // 25 % of the rest for button panel

	Anchors a(Anchor(AnchorType::RIGID, 0)); // fill
	m_connectPanel->setAnchors(a);
	Vec2i pad(45, 45);
	m_connectPanel->setPos(pad);
	m_connectPanel->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	// anchors for sub panels, fill vertical, set 25 % in from left / right edges
	a = Anchors(Anchor(AnchorType::SPRINGY, 25), Anchor(AnchorType::RIGID, 0));

	// Panel to contain recent host label/list
	CellStrip *pnl = new CellStrip(m_connectPanel, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	pnl->setCell(0);
	pnl->setAnchors(a);

	// anchors for label and list, fill horizontal, 2px in from top and bottom
	Anchors a2 = Anchors::getFillAnchors();
	a2.set(Edge::TOP, Anchor(AnchorType::RIGID, 2));
	a2.set(Edge::BOTTOM, Anchor(AnchorType::RIGID, 2));

	StaticText* historyLabel = new StaticText(pnl, Vec2i(0), Vec2i(200, 34));
	historyLabel->setCell(0);
	historyLabel->setText(g_lang.get("RecentHosts"));
	historyLabel->setAnchors(a2);
	m_historyList = new DropList(pnl);
	m_historyList->setCell(1);
	m_historyList->setAnchors(a2);

	// Panel to contain server label/textbox
	pnl = new CellStrip(m_connectPanel, Orientation::HORIZONTAL, Origin::CENTRE, 2);
	pnl->setCell(1);
	pnl->setAnchors(a);

	StaticText* serverLabel = new StaticText(pnl);
	serverLabel->setCell(0);
	serverLabel->setText(g_lang.get("Server") + " Ip: ");
	serverLabel->setAnchors(a2);
	
	m_serverTextBox = new TextBox(pnl);
	m_serverTextBox->setCell(1);
	m_serverTextBox->setText("");
	m_serverTextBox->TextChanged.connect(this, &MenuStateJoinGame::onTextModified);
	m_serverTextBox->setAnchors(a2);

	m_connectLabel = new StaticText(m_connectPanel);
	m_connectLabel->setCell(2);
	m_connectLabel->setText(g_lang.get("NotConnected"));
	m_connectLabel->setAnchors(a2);

	// buttons panel, fill
	a = Anchors::getFillAnchors();
	pnl = new CellStrip(m_connectPanel, Orientation::HORIZONTAL, Origin::CENTRE, 3);
	pnl->setCell(3);
	pnl->setAnchors(a);

	// buttons will be sized explicitly, set anchors to centre in cell
	a2.setCentre(true);

	// buttons
	Button* returnButton = new Button(pnl, Vec2i(0), Vec2i(7 * defWidgetHeight, defWidgetHeight));
	returnButton->setCell(0);
	returnButton->setText(g_lang.get("Return"));
	returnButton->Clicked.connect(this, &MenuStateJoinGame::onReturn);
	returnButton->setAnchors(a2);

	Button* connectButton = new Button(pnl, Vec2i(0), Vec2i(7 * defWidgetHeight, defWidgetHeight));
	connectButton->setCell(1);
	connectButton->setText(g_lang.get("Connect"));
	connectButton->Clicked.connect(this, &MenuStateJoinGame::onConnect);
	connectButton->setAnchors(a2);

	Button* searchButton = new Button(pnl, Vec2i(0), Vec2i(7 * defWidgetHeight, defWidgetHeight));
	searchButton->setCell(2);
	searchButton->setText(g_lang.get("Search"));
	searchButton->Clicked.connect(this, &MenuStateJoinGame::onSearchForGame);
	searchButton->setAnchors(a2);

	const Properties::PropertyMap &pm = m_servers.getPropertyMap();
	if (pm.empty()) {
		m_historyList->setEnabled(false);
	} else {
		foreach_const (Properties::PropertyMap, i, pm) {
			m_historyList->addItem(i->first);
		}
	}
	m_historyList->SelectionChanged.connect(this, &MenuStateJoinGame::onServerSelected);
	program.setFade(0.f);
}

void MenuStateJoinGame::onReturn(Widget*) {
	m_targetTansition = Transition::RETURN;
	g_soundRenderer.playFx(g_coreData.getClickSoundA());
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}

void MenuStateJoinGame::onConnect(Widget*) {
	cout << "MenuStateJoinGame::onConnect()\n";
	g_config.setNetServerIp(m_serverTextBox->getText());
	g_config.save();

	// validate Ip ??

	m_connectPanel->setVisible(false);
	Vec2i size = g_widgetConfig.getDefaultDialogSize();
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	assert(!m_messageBox);
	m_messageBox = MessageDialog::showDialog(pos, size, "Connecting...",
		"Connecting, Please wait.", g_lang.get("Cancel"), "");
	m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onCancelConnect);
	m_messageBox->Close.connect(this, &MenuStateJoinGame::onCancelConnect);
	{
		MutexLock lock(m_connectMutex);
		assert(!m_connectThread && !m_searching && !m_connecting);
		m_connectThread = new ConnectThread(*this, Ip(m_serverTextBox->getText()));
		m_connecting = true;
	}
	cout << "connect panel hidden, message box up, connect thread started, connecting flag set.\n";
}

void MenuStateJoinGame::onCancelConnect(Widget*) {
	cout << "MenuStateJoinGame::onCancelConnect()\n";
	MutexLock lock(m_connectMutex);
	if (m_connectThread) {
		// remove message box, but do not show connect panel yet
		assert(m_messageBox);
		g_widgetWindow.removeFloatingWidget(m_messageBox);
		m_messageBox = 0;
		cout << "message box removed, cancelling connect thread.\n";
		m_connectThread->cancel();
	} // else it had finished already
	else {
		cout << "connect thread was finished already.\n";
	}
}

void MenuStateJoinGame::connectThreadDone(ConnectResult result) {
	cout << "MenuStateJoinGame::connectThreadDone()\n";
	MutexLock lock(m_connectMutex);
	m_connectResult = result;
	delete m_connectThread;
	m_connectThread = 0;
	cout << "Result = " << result << ", connect thread deleted.\n";
}

//	program.removeFloatingWidget(m_messageBox);
//	m_messageBox = 0;
//
//	if (result == ConnectResult::SUCCESS) {
//		connected = true;
//		Vec2i pos, size(300, 200);
//		pos = g_metrics.getScreenDims() / 2 - size / 2;
//		m_messageBox = MessageDialog::showDialog(pos, size, g_lang.get("Connected"),
//			g_lang.get("Connected") + "\n" + g_lang.get("WaitingHost"), g_lang.get("Disconnect"), "");
//		m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onDisconnect);
//		m_messageBox->Close.connect(this, &MenuStateJoinGame::onDisconnect);
//	} else if (result == ConnectResult::CANCELLED) {
//		m_connectPanel->setVisible(true);
//		m_connectLabel->setText("Not connected. Last attempt cancelled.");///@todo localise
//	} else {
//		m_connectPanel->setVisible(true);
//		m_connectLabel->setText("Not connected. Last attempt failed.");///@todo localise
//		///@todo show a message...
//		//string err = m_connectThread->getErrorMsg();
//		//
//		// don't set m_connectPanel visible, 
//		// recreate Dialog with error msg, 
//		// on dismiss show m_connectPanel
//	}
//	/*** *** CONCURRENCY ERROR *** ***/
//	/*** *** CONCURRENCY ERROR *** ***/
//
//
//	delete m_connectThread;
//	m_connectThread = 0;
//}

void MenuStateJoinGame::onDisconnect(Widget*) {
	program.removeFloatingWidget(m_messageBox);
	m_messageBox = 0;
	g_simInterface.asClientInterface()->reset();
	m_connectPanel->setVisible(true);
	m_connectLabel->setText(g_lang.get("ConnectSevered"));
}

void MenuStateJoinGame::onTextModified(Widget*) {
	m_historyList->setSelected(-1);
}

void MenuStateJoinGame::onSearchForGame(Widget*) {
	m_historyList->setSelected(-1);
	m_serverTextBox->setText("");
	m_connectPanel->setVisible(false);
	Vec2i size = g_widgetConfig.getDefaultDialogSize();
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	assert(!m_messageBox);
	m_messageBox = MessageDialog::showDialog(pos, size, g_lang.get("SearchingHdr"),
		g_lang.get("SearchingWait"), g_lang.get("Cancel"), "");
	m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onCancelSearch);
	m_messageBox->Close.connect(this, &MenuStateJoinGame::onCancelSearch);
	{
		MutexLock lock(m_findServerMutex);
		assert(!m_findServerThread && !m_searching && !m_connecting);
		m_findServerThread = new FindServerThread(*this);
		m_searching = true;
	}
}

void MenuStateJoinGame::onCancelSearch(Widget*) {
	MutexLock lock(m_findServerMutex);
	if (m_findServerThread) {
		m_findServerThread->stop();
		delete m_findServerThread;
		m_findServerThread = 0;
		assert(m_messageBox);

		//TODO Fix this

		///@todo Fix this

		/*** *** CONCURRENCY ERROR *** ***/
		/*** *** CONCURRENCY ERROR *** ***/
		/*
		 *  This is called from the FindServerThread, can't mess with widgets here...
		 */
		program.removeFloatingWidget(m_messageBox);
		m_messageBox = 0;
		m_connectPanel->setVisible(true);
		m_connectLabel->setText(g_lang.get("ConnectAborted"));
		/*** *** CONCURRENCY ERROR *** ***/
		/*** *** CONCURRENCY ERROR *** ***/

	} // else it had finished already
}

void MenuStateJoinGame::foundServer(Ip ip) {
	MutexLock lock(m_findServerMutex);
	program.removeFloatingWidget(m_messageBox); /*** *** CONCURRENCY ERROR *** ***/
	m_messageBox = 0;
	m_serverTextBox->setText(ip.getString()); /*** *** CONCURRENCY ERROR *** ***/
	delete m_findServerThread;
	m_findServerThread = 0;
	m_searching = false;
	onConnect(0); /*** *** CONCURRENCY ERROR *** ***/
}

void MenuStateJoinGame::onServerSelected(Widget* source) {
	DropList* historyList = static_cast<DropList*>(source);
	if (historyList->getSelectedIndex() != -1) {
		string selected = historyList->getSelectedItem()->getText();
		string ipString = m_servers.getString(selected);
		m_serverTextBox->setText(ipString);
	}
}

void MenuStateJoinGame::update() {
	MenuState::update();

	if (m_connectThread) { // don't touch ClientInterface if ConnectThread is alive
		return;
	}
	if (m_connecting) { // connecting flag set, but thread null => finished, check results
		cout << "MenuStateJoinGame::update() ... connect thread finsihed.\n";
		if (m_messageBox) {
			program.removeFloatingWidget(m_messageBox);
			m_messageBox = 0;
			cout << "message box was up, has been removed.\n";
		}
		if (m_connectResult == ConnectResult::SUCCESS) {
			m_connected = true;
			Vec2i size = g_widgetConfig.getDefaultDialogSize();
			Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
			m_messageBox = MessageDialog::showDialog(pos, size, g_lang.get("Connected"),
				g_lang.get("Connected") + "\n" + g_lang.get("WaitingHost"), g_lang.get("Disconnect"), "");
			m_messageBox->Button1Clicked.connect(this, &MenuStateJoinGame::onDisconnect);
			m_messageBox->Close.connect(this, &MenuStateJoinGame::onDisconnect);
			cout << "Connected to server, connected message box shown, connected flag set.\n";
		} else if (m_connectResult == ConnectResult::CANCELLED) {
			m_connectPanel->setVisible(true);
			m_connectLabel->setText("Not connected. Last attempt cancelled.");///@todo localise
			cout << "Connect cancelled, connect panel shown.\n";
		} else {
			m_connectPanel->setVisible(true);
			m_connectLabel->setText("Not connected. Last attempt failed.");///@todo localise
			cout << "Connect failed, connect panel shown.\n";
		}
		m_connecting = false;
		cout << "connecting flag unset.\n";
	}

	ClientInterface* clientInterface = g_simInterface.asClientInterface();

	if (m_connected && !clientInterface->isConnected()) {
		m_connected = false;
		if (m_messageBox) {
			program.removeFloatingWidget(m_messageBox);
			m_messageBox = 0;
		}
		m_connectPanel->setVisible(true);
		m_connectLabel->setText("Not connected. Last connection was severed."); ///@todo localise
	}

	// process network messages
	if (clientInterface->isConnected()) {
		// update lobby
		clientInterface->updateLobby();

		// intro
		if (clientInterface->getIntroDone()) {
			m_servers.setString(clientInterface->getDescription(), m_serverTextBox->getText());
		}

		// launch
		if (clientInterface->getLaunchGame()) {
			m_servers.save(serverFileName);
			m_targetTansition = Transition::PLAY;
			program.clear();
			program.setState(new GameState(program));
			return;
		}
	}
	if (m_transition) {
		program.clear();
		switch (m_targetTansition) {
			case Transition::RETURN:
				mainMenu->setState(new MenuStateRoot(program, mainMenu));
				break;
		}
	}
}

}}//end namespace
