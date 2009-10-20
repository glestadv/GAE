// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_GAMECONNECTOR_H_
#define _GAME_GAMECONNECTOR_H_

#include <string>

#include "patterns.h"

using std::string;

namespace Game {

class Command;

class Messenger : private Uncopyable {
public:
	virtual ~Messenger() {}
	virtual void postCommand(Command *command) = 0;
	virtual void postTextMessage(const string &text, int teamId) = 0;
};

class LocalMessenger : public Messenger {
public:
	virtual ~LocalMessenger() {}

	virtual void postCommand(Command *command);
	virtual void postTextMessage(const string &text, int teamId);
};

} // end namespace

#endif // _GAME_NET_GAMEINTERFACE_H_
