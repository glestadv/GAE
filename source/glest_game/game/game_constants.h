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

#ifndef _GLEST_GAME_GAMECONSTANTS_H_
#define _GLEST_GAME_GAMECONSTANTS_H_

// The 'Globals'
#define theGame				(*Game::getInstance())
#define theWorld			(World::getInstance())
#define theMap				(*World::getInstance().getMap())
#define theCamera			(*Game::getInstance()->getGameCamera())
#define theGameSettings		(Game::getInstance()->getGameSettings())
#define theGui				(*Gui::getCurrentGui())
#define theConsole			(*Game::getInstance()->getConsole())
#define theConfig			(Config::getInstance())
#define theRoutePlanner		(*Search::RoutePlanner::getInstance())
#define theRenderer			(Renderer::getInstance())
#define theNetworkManager	(NetworkManager::getInstance())
#define theSoundRenderer	(SoundRenderer::getInstance())
#define theLogger			(Logger::getInstance())

#define LOG( x ) Logger::getInstance().add(x)

namespace Glest{ namespace Game{

// =====================================================
//	class GameConstants
// =====================================================

enum ControlType{
    ctClosed,
	ctCpu,
	ctCpuUltra,
	ctNetwork,
	ctHuman,

	ctCount
};

// implemented in game.cpp
extern const char* controlTypeNames[ctCount];

class GameConstants{
public:
	static const int maxPlayers= 4;
	static const int serverPort= 61357;
//	static const int updateFps= 40;
	static const int cameraFps= 100;
	static const int networkFramePeriod= 10;
	static const int networkExtraLatency= 200;
};

}}//end namespace

#endif
