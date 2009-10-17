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

#include "pch.h"

#include "influence_map.h"
#include "world.h"

using Shared::Graphics::Vec2i;

namespace Glest { namespace Game { namespace Search {

// I'm using memset on a float array... sanity check...
bool InfluenceMap::assertFloatZeroIsAllZeroBits() {
	for ( int i=0; i < width * height; ++i ) {
		if ( iMap[i] != 0.f ) {
			return false;
		}
	}
	return true;
}

InfluenceMap::InfluenceMap() 
		: xOffset( 0 )
		, yOffset( 0 ) {
	width = theMap.getW();
	height = theMap.getH();
	iMap = new float[width * height];
	// is this safe?
	memset( iMap, 0, sizeof(float) * width * height );
	// let's see...
	//assert(assertFloatZeroIsAllZeroBits());
}

InfluenceMap::InfluenceMap( int x, int y, int w, int h ) 
		: xOffset( x )
		, yOffset( y )
		, width( w )
		, height( h ) {
	assert(theMap.isInside(x, y));
	assert(theMap.isInside(x + w - 1, y + h - 1));
	iMap = new float[w * h];
	// is this safe?
	memset(iMap, 0, sizeof(float) * w * h);
	// let's see...
	//assert( assertFloatZeroIsAllZeroBits() );
}

bool InfluenceMap::isInside(const Vec2i &pos) const {
	const int x = pos.x - xOffset;
	const int y = pos.y - yOffset;
	if ( x < 0 || x >= width || y < 0 || y >= height ) {
		return false;
	}
	return true;
}

float InfluenceMap::getInfluence(const Vec2i &pos) const {
	//assert(theMap.isInside(pos));
	const int x = pos.x - xOffset;
	const int y = pos.y - yOffset;
	if ( x < 0 || x >= width || y < 0 || y >= height ) {
		return 0.f;
	}
	return iMap[y * width + x];
}

void InfluenceMap::setInfluence(const Vec2i &pos, float infl) {
	//assert(theMap.isInside(pos));
	const int x = pos.x - xOffset;
	const int y = pos.y - yOffset;
	if ( x < 0 || x >= width || y < 0 || y >= height ) {
		assert(false);
		return;
	}
	iMap[y * width + x] = infl;
}

void InfluenceMap::log() {
	FILE *fp = fopen("influence.log", "w");
	for ( int y=0; y < height; ++y ) {
		for ( int x=0; x < width; ++x ) {
			fprintf(fp, "%2.2f%s", iMap[y * width + x], x == width - 1 ? "\n" : ", ");
		}
	}
	fclose(fp);
}

}}}

