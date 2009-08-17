// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "abstract_map.h"

namespace Glest { namespace Game { namespace Search {

AbstractMap::AbstractMap ( AnnotatedMap *aMap ) {
	clusters.init ( aMap->getWidth() / 4, aMap->getHeight() / 4 );

}

}}}