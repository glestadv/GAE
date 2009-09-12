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

#define CLUSTER_SIZE 16

// This will actually be 'packed' using bitfields...
struct Entrance {
	int clearance;
	int position;
};

struct Border {
	// Entrances of max clearance for each field
	Entrance transitions[FieldCount];

	/* intra cluster edge weights
	 for vertical borders...        for horizontal borders...
	       2      3					       1
	   +------+------+				   +------+		[0] = NW
	   |      |      |				   |      |		[1] = N	
	 1 |  C1  |  C2  | 4			 0 |  C1  | 2	[2] = NE
	   |      |      |				   |      |		[3] = SE
	   +------+------+				   +------+		[4] = S
	       0      5					   |      |		[5] = SW
	   [0] == SW					 5 |  C2  | 3
	   [1] == W						   |      |
	   [2] == NW					   +------+
	   [3] == NE					       4
	   [4] == E
	   [5] == SE
	*/
	float weights [FieldCount][6];
};

class AbstractMap {
	
	Border *vertBorders, *horizBorders, sentinel;
	int w, h; // width and height, in clusters
	AnnotatedMap *aMap;

public:
	AbstractMap ( AnnotatedMap *aMap );
	~AbstractMap ();

	// 'Initialise' a cluster (evaluates north and west borders)
	void initCluster ( Vec2i cluster );

	// Update a cluster, evaluates all borders
	void updateCluster ( Vec2i cluster );

	// compute intra-cluster path lengths
	void evalCluster ( Vec2i cluster );

	bool search ( SearchParams params, list<Vec2i> &apath );

	void getBorders ( Vec2i cluster, vector<Border*> &borders, Border *exclude = NULL );

	// Border getters
	Border* getNorthBorder ( Vec2i cluster );
	Border* getEastBorder ( Vec2i cluster );
	Border* getSouthBorder ( Vec2i cluster );
	Border* getWestBorder ( Vec2i cluster );	
};

}}}