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

AbstractMap::AbstractMap ( AnnotatedMap *am ) {
	aMap = am;

	assert ( aMap->getWidth () % CLUSTER_SIZE == 0 );
	assert ( aMap->getHeight () % CLUSTER_SIZE == 0 );

	w = aMap->getWidth () / CLUSTER_SIZE;
	h = aMap->getHeight () / CLUSTER_SIZE;

	vertBorders = new Border[(w-1)*h];
	horizBorders = new Border[w*(h-1)];

	// the 'sentinel' (for borders on the edges of the map)
	for ( int f = 0; f < FieldCount; ++f ) {
		sentinel.transitions[f].clearance = 0;
		sentinel.transitions[f].position = -1;
		for ( int i = 0; i < 6; ++i ) {
			sentinel.weights[f][i] = -1;
		}
	}

	// init Borders (and hence inter-cluster edges) & evaluate clusters (intra-cluster edges)
	for ( int i = h - 1; i >= 0; --i ) {
		for ( int j = w - 1; j >= 0; --j ) {
			Vec2i cluster ( j, i );
			initCluster ( cluster );
			evalCluster ( cluster );
		}
	}
	
}

#define POS_X ((cluster.x+1)*CLUSTER_SIZE-i-1)
#define POS_Y ((cluster.y+1)*CLUSTER_SIZE-i-1)

void AbstractMap::initCluster ( Vec2i cluster ) {
	
	Border *west = getWestBorder ( cluster );
	if ( west != &sentinel ) {
		int x1 = cluster.x * CLUSTER_SIZE;
		int x2 = x1 - 1;
		memset ( west->transitions, 0, sizeof (Entrance) * FieldCount );
		for ( int i=0; i < CLUSTER_SIZE; ++i ) {
			for ( int f = 0; f < FieldCount; ++f ) {
				int clear1 = aMap->metrics[Vec2i(x1,POS_Y)].get ((Field)f);
				int clear2 = aMap->metrics[Vec2i(x2,POS_Y)].get ((Field)f);
				int local = min ( clear1, clear2 );
				if ( local > west->transitions[f].clearance ) {
					west->transitions[f].clearance = local;
					west->transitions[f].position = POS_Y; // store i instead ???
				}
			}
		}
	}
	Border *north = getNorthBorder ( cluster );
	if ( north != &sentinel ) {
		int y1 = cluster.y * CLUSTER_SIZE;
		int y2 = y1 - 1;
		memset ( north->transitions, 0, sizeof (Entrance) * FieldCount );
		for ( int i=0; i < CLUSTER_SIZE; ++i ) {
			for ( int f = 0; f < FieldCount; ++f ) {
				int clear1 = aMap->metrics[Vec2i(POS_X,y1)].get ((Field)f);
				int clear2 = aMap->metrics[Vec2i(POS_X,y2)].get ((Field)f);
				int local = min ( clear1, clear2 );
				if ( local > north->transitions[f].clearance ) {
					north->transitions[f].clearance = local;
					north->transitions[f].position = POS_X;
				}
			}			
		}
	}
}

#define NORTH_Y (cluster.y*CLUSTER_SIZE)
#define SOUTH_Y (cluster.y*CLUSTER_SIZE+CLUSTER_SIZE-1)
#define WEST_X (cluster.x*CLUSTER_SIZE)
#define EAST_X (cluster.x*CLUSTER_SIZE+CLUSTER_SIZE-1)

void AbstractMap::evalCluster ( Vec2i cluster ) {
	Border *north = getNorthBorder ( cluster );
	Border *east = getEastBorder ( cluster );
	Border *south = getSouthBorder ( cluster );
	Border *west = getWestBorder ( cluster );

	SearchParams params;
	params.size = 1;
	params.team = -1;

	for ( int i = 0; i < FieldCount; ++i ) {
		//
		// TODO: when/if AnnotatedMap takes into account elevation
		// fix this, it's currently symmetric
		//
		if ( north->transitions[i].clearance ) {
			params.start.x = north->transitions[i].position;
			params.start.y = NORTH_Y;
			// path to east, south & west
			if ( east->transitions[i].clearance ) {
				params.dest.x = EAST_X;
				params.dest.y = east->transitions[i].position;
				east->weights[i][2] = aMap->AStarPathLength ( params );
				north->weights[i][3] = east->weights[i][2];
			}
			else {
				north->weights[i][3] = -1.f;
			}
			if ( south->transitions[i].clearance ) {
				params.dest.x = south->transitions[i].position;
				params.dest.y = SOUTH_Y;
				south->weights[i][1] = aMap->AStarPathLength ( params );
				north->weights[i][4] = south->weights[i][1];
			}
			else {
				north->weights[i][4] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				params.dest.x = WEST_X;
				params.dest.y = west->transitions[i].position;
				west->weights[i][3] = aMap->AStarPathLength ( params );
				north->weights[i][5] = west->weights[i][3];
			}
			else {
				north->weights[i][5] = -1.f;
			}
		}
		else {
			if ( east->transitions[i].clearance ) {
				east->weights[i][2] = -1.f;
			}
			if ( south->transitions[i].clearance ) {
				south->weights[i][1] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				west->weights[i][3] = -1.f;
			}
		}
		if ( east->transitions[i].clearance ) {
			params.start.x = EAST_X;
			params.start.y = east->transitions[i].position;
			// path to south & west
			if ( south->transitions[i].clearance ) {
				params.dest.x = south->transitions[i].position;
				params.dest.y = SOUTH_Y;
				south->weights[i][2] = aMap->AStarPathLength ( params );
				east->weights[i][0] = south->weights[i][2];
			}
			else {
				east->weights[i][0] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				params.dest.x = WEST_X;
				params.dest.y = west->transitions[i].position;
				west->weights[i][4] = aMap->AStarPathLength ( params );
				east->weights[i][1] = west->weights[i][4];
			}
			else {
				east->weights[i][1] = -1.f;
			}
		}
		else {
			if ( south->transitions[i].clearance ) {
				south->weights[i][2] = -1.f;
			}
			if ( west->transitions[i].clearance ) {
				west->weights[i][4] = -1.f;
			}
		}
		if ( south->transitions[i].clearance ) {
			params.start.x = south->transitions[i].position;
			params.start.y = SOUTH_Y;
			// path to west
			if ( west->transitions[i].clearance ) {
				params.dest.x = WEST_X;
				params.dest.y = west->transitions[i].position;
				west->weights[i][5] = aMap->AStarPathLength ( params );
				south->weights[i][0] = west->weights[i][5];
			}
			else {
				south->weights[i][0] = -1.f;
			}
		}
		else {
			if ( west->transitions[i].clearance ) {
				west->weights[i][5] = -1.f;
			}
		}
	}
}

bool AbstractMap::search ( SearchParams params, list<Vec2i> &apath ) {
	Vec2i startCluster (0), destCluster (0);
	startCluster.x = params.start.x / CLUSTER_SIZE;
	startCluster.y = params.start.y / CLUSTER_SIZE;
	destCluster.x = params.dest.x / CLUSTER_SIZE;
	destCluster.y = params.dest.y / CLUSTER_SIZE;
	
	apath.clear ();
	if ( startCluster.dist ( destCluster ) < 1.5 ) {
		apath.push_back ( params.start );
		apath.push_back ( params.dest );
		return true;
	}

}


void AbstractMap::getBorders ( Vec2i cluster, vector<Border*> &borders, Border *exclude ) {

}

Border* AbstractMap::getNorthBorder ( Vec2i cluster ) {
	if ( cluster.y == 0 ) {
		return &sentinel;
	}
	else {
		return &horizBorders[(cluster.y - 1) * w + cluster.x ];
	}
}

Border* AbstractMap::getEastBorder ( Vec2i cluster ) {
	if ( cluster.x == w - 1 ) {
		return &sentinel;
	}
	else {
		return &vertBorders[cluster.y * (w - 1) + cluster.x ];
	}
}

Border* AbstractMap::getSouthBorder ( Vec2i cluster ) {
	if ( cluster.y == h - 1 ) {
		return &sentinel;
	}
	else {
		return &horizBorders[cluster.y * w + cluster.x];
	}

}

Border* AbstractMap::getWestBorder ( Vec2i cluster ) {
	if ( cluster.x == 0 ) {
		return &sentinel;
	}
	else {
		return &vertBorders[cluster.y * (w - 1) + cluster.x - 1];
	}
}

}}}