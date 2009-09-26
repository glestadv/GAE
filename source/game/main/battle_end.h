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

#ifndef _GAME_BATTLEEND_H_
#define _GAME_BATTLEEND_H_

#include "program.h"
#include "stats.h"

namespace Game {

// =====================================================
// 	class BattleEnd  
//
///	ProgramState representing the end of the game
// =====================================================

class BattleEnd: public ProgramState {
private:
	shared_ptr<GameSettings> gs;
	Stats stats;

public:
	BattleEnd(Program &program, const shared_ptr<GameSettings> &gs, const Stats &stats);
	~BattleEnd();
	virtual void update();
	virtual void render();
	virtual void keyDown(const Key &key);
	virtual void mouseDownLeft(int x, int y);
};

} // end namespace

#endif
