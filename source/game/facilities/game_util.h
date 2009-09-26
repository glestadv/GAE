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

#ifndef _GAME_GAMEUTIL_H_
#define _GAME_GAMEUTIL_H_

#include <string>
#include <vector>
#include <sstream>

#include "socket.h"
#include "util.h"

using std::string;
using std::stringstream;
using namespace Shared::Util;

namespace Game {

const string &getGlestMailString();
const string &getGaeMailString();
const Version &getGlestVersion();
const Version &getGaeVersion();
const Version &getNetProtocolVersion();

string getAboutString1(int i);
string getAboutString2(int i);
string getTeammateName(int i);
string getTeammateRole(int i);
string getCrashDumpFileName();
string formatString(const string &str);

} // end namespace

#endif
