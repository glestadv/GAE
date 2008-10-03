// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GAME_H_
#define _GLEST_GAME_GAME_H_

#include <vector>

#include "gui.h"
#include "game_camera.h"
#include "world.h"
#include "ai_interface.h"
#include "program.h"
#include "chat_manager.h"
#include "game_settings.h"
#include "config.h"

using std::vector;

namespace Glest{ namespace Game{

class GraphicMessageBox;

// =====================================================
// 	class Game
//
//	Main game class
// =====================================================

class Game: public ProgramState{
public:
	enum Speed{
		sSlowest,
		sVerySlow,
		sSlow,
		sNormal,
		sFast,
		sVeryFast,
		sFastest,

  		sCount
	};

	static const char*SpeedDesc[sCount];

private:
	typedef vector<Ai*> Ais;
	typedef vector<AiInterface*> AiInterfaces;

private:
	//main data
	World world;
    AiInterfaces aiInterfaces;
    Gui gui;
    GameCamera gameCamera;
    Commander commander;
    Console console;
	ChatManager chatManager;

	//misc
	Checksum checksum;
    string loadingText;
    int mouse2d;
    int mouseX, mouseY; //coords win32Api
	int updateFps, lastUpdateFps;
	int renderFps, lastRenderFps;
	bool paused;
	bool gameOver;
	bool renderNetworkStatus;
	Speed speed;
	float fUpdateLoops;
	float lastUpdateLoopsFraction;
	GraphicMessageBox *exitMessageBox;

	//misc ptr
	ParticleSystem *weatherParticleSystem;
	GameSettings gameSettings;

public:
    Game(Program *program, const GameSettings *gameSettings);
    ~Game();

    //get
	GameSettings *getGameSettings()			{return &gameSettings;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCamera()				{return &gameCamera;}
	const Commander *getCommander() const	{return &commander;}
	Gui *getGui()							{return &gui;}
	const Gui *getGui() const				{return &gui;}
	Commander *getCommander()				{return &commander;}
	Console *getConsole()					{return &console;}
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
    virtual void keyDown(char key);
    virtual void keyUp(char key);
    virtual void keyPress(char c);
    virtual void mouseDownLeft(int x, int y);
    virtual void mouseDownRight(int x, int y);
    virtual void mouseUpLeft(int x, int y);
    virtual void mouseDoubleClickLeft(int x, int y);
    virtual void mouseMove(int x, int y, const MouseState *mouseState);

private:
	//render
    void render3d();
    void render2d();

	//misc
	void checkWinner();
	bool hasBuilding(const Faction *faction);
	void incSpeed();
	void decSpeed();
	void updateSpeed();
	int getUpdateLoops();
	void showExitMessageBox(const string &text, bool toggle);
};

}}//end namespace

#endif
