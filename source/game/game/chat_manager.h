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

#ifndef _GAME_CHATMANAGER_H_
#define _GAME_CHATMANAGER_H_

#include <string>
#include <queue>

#include "keymap.h"

using std::string;
using Shared::Platform::Key;

namespace Game {

class Console;

// =====================================================
// class ChatMessage
// =====================================================

class ChatMessage {
private:
	string text;
	string sender;
	int teamIndex;

public:
	ChatMessage(const string &text, const string &sender, int teamIndex);
	// allow default copy ctor

	const string &getText() const	{return text;}
	const string &getSender() const	{return sender;}
	int getTeamIndex() const		{return teamIndex;}
};

typedef std::queue<ChatMessage*> ChatMessages;

// =====================================================
// class ChatManager
// =====================================================

class ChatManager {
private:
	static const int maxTextLenght = 256;

private:
	const Keymap &keymap;
	bool editEnabled;
	bool teamMode;
	Console* console;
	int thisTeamIndex;
	string text;

public:
	ChatManager(const Keymap &keymap);
	void init(Console* console, int thisTeamIndex);

	bool keyDown(const Key &key);
	void keyPress(char c);
	void updateNetwork();

	bool getEditEnabled() const	{return editEnabled;}
	bool getTeamMode() const	{return teamMode;}
	string getText() const		{return text;}
};

} // end namespace

#endif
