// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_GAME_H_
#define _GAME_GAME_H_

#include <vector>

#include "gui.h"
#include "game_camera.h"
#include "world.h"
#include "ai_interface.h"
#include "gui_program.h"
#include "chat_manager.h"
#include "script_manager.h"
#include "game_settings.h"
#include "config.h"
#include "keymap.h"
#include "messenger.h"
#include "game_manager.h"

// weather system not yet ready
//#include "../physics/weather.h"

using std::vector;

namespace Game {

class GraphicMessageBox;
class GraphicTextEntryBox;

// =====================================================
// 	class Game
//
//	Main game class
// =====================================================

class Game: public GuiProgramState {
private:
	typedef vector<Ai*> Ais;
	typedef vector<AiInterface*> AiInterfaces;

private:
	static Game *singleton;

	GameManager &manager;
	//main data
	Keymap &keymap;
	const Input &input;
	const Config &config;
    Console &console;
	World world;
    AiInterfaces aiInterfaces;
    Gui gui;
    GameCamera gameCamera;
    Commander commander;
	ChatManager chatManager;

	//misc
	Checksums checksums;
    string loadingText;
    int mouse2d;
    int mouseX, mouseY; //coords win32Api
	int updateFps, lastUpdateFps;
	int renderFps, lastRenderFps;
	bool paused;
	bool gameOver;
	bool renderNetworkStatus;
	float scrollSpeed;
	GameSpeed speed;
	float fUpdateLoops;
	float lastUpdateLoopsFraction;
	GraphicMessageBox mainMessageBox;

	GraphicTextEntryBox *saveBox;
	Vec2i lastMousePos;

	//misc ptr
	ParticleSystem *weatherParticleSystem;

	vector<string> loadingStrings;

public:
	Game(GameManager &manager, GuiProgram &program);
    ~Game();
	static Game *getInstance()				{return singleton;}

    //get
//	const GameSettings &getGameSettings()	{return *manager.getGameSettings();}
//	Messenger &getMessenger()		{return *interface;}

	const Keymap &getKeymap() const			{return keymap;}
	const Input &getInput() const			{return input;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCamera()				{return &gameCamera;}
	const Commander *getCommander() const	{return &commander;}
	Gui *getGui()							{return &gui;}
	const Gui *getGui() const				{return &gui;}
	Commander *getCommander()				{return &commander;}
	Console &getConsole()					{return getProgram().getConsole();}
	World *getWorld()						{return &world;}
	const World *getWorld() const			{return &world;}

    //init
    virtual void load();
    virtual void init();
	virtual void update();
	virtual void updateCamera();
	virtual void render();
	virtual void tick();


    //Event managing
    virtual void keyDown(const Key &key);
    virtual void keyUp(const Key &key);
    virtual void keyPress(char c);
    virtual void mouseDownLeft(int x, int y);
    virtual void mouseDownRight(int x, int y)			{gui.mouseDownRight(x, y);}
    virtual void mouseUpLeft(int x, int y)				{gui.mouseUpLeft(x, y);}
    virtual void mouseUpRight(int x, int y)				{gui.mouseUpRight(x, y);}
    virtual void mouseDownCenter(int x, int y)			{gameCamera.stop();}
    virtual void mouseUpCenter(int x, int y)			{}
    virtual void mouseDoubleClickLeft(int x, int y);
	virtual void eventMouseWheel(int x, int y, int zDelta);
    virtual void mouseMove(int x, int y, const MouseState &mouseState);

	void setCameraCell(int x, int y) {
		gameCamera.setPos(Vec2f(static_cast<float>(x), static_cast<float>(y)));
	}
	void autoSaveAndPrompt(string msg, string remotePlayerName, int slot = -1);
	void quitGame();
	void pause()							{paused = true;}
	void resume()							{paused = false;}

private:
	//render
    void render3d();
    void render2d();
	void printToLoadingScreen(const string &msg);

	//misc
	void _init();
	void checkWinner();
	void checkWinnerStandard();
	void checkWinnerScripted();
	bool hasBuilding(const Faction *faction);
	void incSpeed();
	void decSpeed();
	void resetSpeed();
	void updateSpeed();
	int getUpdateLoops();

	void showLoseMessageBox();
	void showWinMessageBox();
	void showMessageBox(const string &text, const string &header, bool toggle);

//	void showExitMessageBox(const string &text, bool toggle);
	string controllerTypeToStr(ControlType ct);
	Unit *findUnit(int id);
	char getStringFromFile(ifstream *fileStream, string *str);
	void saveGame(string name) const;
//	void displayError(SocketException &e);
};

} // end namespace

#endif
