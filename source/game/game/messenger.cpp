// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008-2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "game_connector.h"

#include "commander.h"
#include "chat_manager.h"

namespace Game {

// =====================================================
//	class LocalMessenger
// =====================================================

void LocalMessenger::postCommand(Command *command) {
}

void LocalMessenger::postTextMessage(const string &text, int teamId) {
}

} // end namespace
