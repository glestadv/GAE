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

#ifndef _GLEST_GAME_NETWORKUTIL_H_
#define _GLEST_GAME_NETWORKUTIL_H_

#include "network_manager.h"

namespace Glest{ namespace Game{

class ServerInterface;
class ClientInterface;

bool isLocal()							{return NetworkManager::getInstance().isLocal();}
bool isNetworkGame()					{return NetworkManager::getInstance().isNetworkGame();}
bool isNetworkServer()					{return NetworkManager::getInstance().isNetworkServer();}
bool isNetworkClient()					{return NetworkManager::getInstance().isNetworkClient();}
ServerInterface *getServerInterface()	{return NetworkManager::getInstance().getServerInterface();}
ClientInterface *getClientInterface()	{return NetworkManager::getInstance().getClientInterface();}

}}//end namespace

#endif