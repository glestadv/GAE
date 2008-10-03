// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "game_settings.h"
#include "random.h"

using Shared::Util::Random;

namespace Glest{ namespace Game{

void GameSettings::randomizeLocs() {
	bool slotUsed[GameConstants::maxPlayers];
	Random rand;

	memset(slotUsed, 0, sizeof(slotUsed));
	rand.init(1234);

	for(int i = 0; i < GameConstants::maxPlayers; i++) {
		int slot = rand.randRange(0, 3 - i);
		for(int j = slot; j < slot + GameConstants::maxPlayers; j++) {
			int k = j % GameConstants::maxPlayers;
			if(!slotUsed[j % GameConstants::maxPlayers]) {
				slotUsed[j % GameConstants::maxPlayers] = true;
				startLocationIndex[k] = i;
				break;
			}
		}
	}
}

}}//end namespace
