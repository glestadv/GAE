// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009	James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#ifndef _GAME_SEARCH_INLUENCE_MAP_H_
#define _GAME_SEARCH_INLUENCE_MAP_H_

#include "vec.h"

using Shared::Graphics::Vec2i;

namespace Glest { namespace Game { namespace Search {


class InfluenceMap {
	float *iMap;
	int xOffset, yOffset, width, height;

	bool assertFloatZeroIsAllZeroBits();

public:
	InfluenceMap();
	InfluenceMap( int x, int y, int w, int h );

	void clear () { memset( iMap, (int)0.f, sizeof(float) * width * height ); }
	void clear ( float val ) { std::fill_n( iMap, width*height, val ); }
	bool isInside( const Vec2i &pos ) const;
	float getInfluence( const Vec2i &pos ) const;
	void setInfluence( const Vec2i &pos, float infl );

	void log();
};



}}}


#endif
