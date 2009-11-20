// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_PROGRAM_H_
#define _GAME_PROGRAM_H_

#include <string>

#include "config.h"
#include "console.h"
#include "lang.h"
#include "patterns.h"

using std::string;
using namespace Shared::Platform;

namespace Game {

class Program : private Uncopyable {
public:
	enum LaunchType {
		START_OPTION_NORMAL,
		START_OPTION_SERVER,
		START_OPTION_CLIENT,
		START_OPTION_DEDICATED
	};

private:
	Config config;
	Console console;
	Lang lang;
	Program::LaunchType launchType;

	static Program *singleton;				/**< The one and only Program object. */

public:
	Program(Program::LaunchType launchType);
	virtual ~Program();

	static Program &getInstance()		{assert(singleton); return *singleton;}

	Config &getConfig()					{return config;}
	const Config &getConfig() const		{return config;}
	const Lang &getLang() const			{return lang;}
	Console &getConsole()				{return console;}
	const Console &getConsole() const	{return console;}
	bool isDedicated() const			{return launchType == START_OPTION_DEDICATED;}

	virtual int main() = 0;

//protected:
	Lang &getNonConstLang()				{return lang;}
};

} // end namespace

#endif
