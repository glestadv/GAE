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

#include "annotated_map.h"

namespace Glest { namespace Game { namespace Search {

struct MapCluster {
	Vec2i pos;
	float edgeCosts[8]; // edgeCosts[FieldCount][maxClearanceValue][8];
};

// class ClusterMap
class ClusterMap {
private:
	MapCluster *clusters;
	int stride;

public:
	ClusterMap () { stride = 0; clusters = NULL; }
	virtual ~ClusterMap () { delete [] clusters; }

	void init ( int w, int h ) { 
		assert (w>0&&h>0); stride = w; clusters = new MapCluster[w*h]; 
	}
	MapCluster& operator [] ( const Vec2i &pos ) const { 
		return clusters[pos.y*stride+pos.x]; 
	}
};

class AbstractMap {
	ClusterMap clusters;
public:
	AbstractMap ( AnnotatedMap *aMap );

};

}}}