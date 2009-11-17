// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#define GAME_CONSTANTS_DEF
#include "game_constants.h"

namespace Glest { namespace Game {

const char *enumGameSpeedDesc[GAME_SPEED_COUNT] = {
	"Slowest",
	"VerySlow",
	"Slow",
	"Normal",
	"Fast",
	"VeryFast",
	"Fastest"
};

const char *enumControlTypeDesc[CT_COUNT] = {
	"Closed",
	"Cpu",
	"CpuUltra",
	"Network",
	"Human"
};

}} // end namespace
