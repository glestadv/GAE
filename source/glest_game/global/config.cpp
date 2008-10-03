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

#include "config.h"
#include "util.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class Config
// =====================================================

Config::Config(){
	properties.load("glest.ini");
	fastestSpeed = properties.getFloat("FastestSpeed", 1.0f, 10.0f);
	slowestSpeed = properties.getFloat("SlowestSpeed", 0.01f, 1.0f);
}

}}// end namespace
