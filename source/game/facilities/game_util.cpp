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

#include "pch.h"
#include "game_util.h"

#include "util.h"
#include "lang.h"
//#include "game_constants.h"
#include "config.h"

#include "leak_dumper.h"

//using namespace Shared::Util;

namespace Game {

const string glestMailString			= "contact_game@glest.org";
const string gaeMailString				= "daniel.santos@pobox.com";
const Version glestVersion				(3, 2, 2, 0, false);
const Version gaeVersion				(0, 2, 14, 0, true);
const Version netProtocolVersion		(0, 2, 14, 0, true);
string getCrashDumpFileName(){
	return "gae" + gaeVersionString + ".dmp";
}

const string &getGlestMailString()		{return glestMailString;}
const string &getGaeMailString()		{return gaeMailString;}
const Version &getGlestVersion()		{return glestVersion;}
const Version &getGaeVersion()			{return gaeVersion;}
const Version &getNetProtocolVersion()	{return netProtocolVersion;}

string getAboutString1(int i) {
	switch(i) {
	case 0: return "Glest Advanced Engine v" + getGaeVersion().toString() + ", based on Glest v"
			+ getGlestVersion().toString() + " (Shared Library v"
			+ getGaeSharedLibVersion().toString() + ", based on Glest Shared Library v"
			+ getGlestSharedLibVersion().toString() + ")";
	case 1: return "Built: " + string(__DATE__);
	case 2: return "Copyright 2001-2009 The Glest Team, 2008-2009 Daniel Santos, and others";
	}
	return "";
}

string getAboutString2(int i) {
	switch(i){
	case 0: return "Web: http://glest.codemonger.org, http://glest.org";
	case 1: return "Mail: " + gaeMailString + ", " + glestMailString;
	case 2: return "Irc: irc://irc.freenode.net/glest";
	}
	return "";
}

string getTeammateName(int i) {
	switch(i){
	case 0: return "Marti�o Figueroa";
	case 1: return "Jos� Luis Gonz�lez";
	case 2: return "Tucho Fern�ndez";
	case 3: return "Jos� Zanni";
	case 4: return "F�lix Men�ndez";
	case 5: return "Marcos Caruncho";
	case 6: return "Matthias Braun";
	}
	return "";
}

string getTeammateRole(int i) {
	Lang &l= Lang::getInstance();

	switch(i){
	case 0: return l.get("Programming");
	case 1: return l.get("SoundAndMusic");
	case 2: return l.get("3dAnd2dArt");
	case 3: return l.get("2dArtAndWeb");
	case 4: return l.get("Animation");
	case 5: return l.get("3dArt");
	case 6: return l.get("LinuxPort");
	}
	return "";
}

string formatString(const string &str) {
	string outStr = str;

	if (!outStr.empty()) {
		outStr[0] = toupper(outStr[0]);
	}

	bool afterSeparator = false;
	for (int i = 0; i < str.size(); ++i) {
		if (outStr[i] == '_') {
			outStr[i] = ' ';
		} else if (afterSeparator) {
			outStr[i] = toupper(outStr[i]);
			afterSeparator = false;
		}
		if (outStr[i] == '\n' || outStr[i] == '(' || outStr[i] == ' ') {
			afterSeparator = true;
		}
	}
	return outStr;
}

} // end namespace
