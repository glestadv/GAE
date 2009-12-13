// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CONSOLE_H_
#define _GLEST_GAME_CONSOLE_H_

#include <utility>
#include <string>
#include <vector>

#include "config.h"
#include "patterns.h"

using std::string;
using std::vector;
using std::pair;

namespace Glest { namespace Game {

class Lang;

// =====================================================
//  class Console
//
/// In-game console that shows various types of messages
// =====================================================

class Console : Uncopyable {
public:
	typedef pair<string, float> StringTimePair;
	typedef vector<StringTimePair> Lines;
	typedef Lines::const_iterator LineIterator;

private:
	const Lang &lang;
	float timeElapsed;
	Lines lines;

	//config
	int maxLines;
	float timeout;

public:
	Console(const Config &config, const Lang &lang);

	int getLineCount() const	{return lines.size();}
	string getLine(int i) const	{return lines[i].first;}

	void addStdMessage(const string &s);
	void addStdMessage(const string &s, const string &param1, const string &param2 = "", const string &param3 = "");
	void addLine(string line, bool playSound = false);
	//void update();
	bool isEmpty();
};

}}//end namespace

#endif
