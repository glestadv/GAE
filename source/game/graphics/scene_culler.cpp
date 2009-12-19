// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2009	James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "scene_culler.h"

#include "game_camera.h"
#include "game.h"

namespace Glest { namespace Game {

#undef max

void SceneCuller::Extrema::reset(int minY, int maxY) {
	edges.clear();
	min_y = minY;
	max_y = maxY;
	min_x = numeric_limits<int>::max();
	max_x = -1;
}

/** determine visibility of cells & tiles */
void SceneCuller::establishScene() {
	extractFrustum();
	getFrustumExtents();
	setVisibleExtrema();
}

/** Intersection of 3 planes */
Vec3f SceneCuller::intersection(const Plane &p1, const Plane &p2, const Plane &p3) const {
	float denominator = p1.n.dot(p2.n.cross(p3.n));	
	Vec3f quotient	= p2.n.cross(p3.n) * p1.d
					+ p3.n.cross(p1.n) * p2.d
					+ p1.n.cross(p2.n) * p3.d;
	return quotient / denominator;
}

/** Extract view frustum planes from projection and view matrices */
void SceneCuller::extractFrustum() {
	GLMatrix mv, proj, fm;
	glGetFloatv(GL_MODELVIEW_MATRIX, mv.raw);
	glGetFloatv(GL_PROJECTION_MATRIX, proj.raw);
	fm = proj * mv;

	frstmPlanes[Left].n.x	=	fm._41 + fm._11;
	frstmPlanes[Left].n.y	=	fm._42 + fm._12;
	frstmPlanes[Left].n.z	=	fm._43 + fm._13;
	frstmPlanes[Left].d		= -(fm._44 + fm._14);

	frstmPlanes[Right].n.x	=	fm._41 - fm._11;
	frstmPlanes[Right].n.y	=	fm._42 - fm._12;
	frstmPlanes[Right].n.z	=	fm._43 - fm._13;
	frstmPlanes[Right].d	= -(fm._44 - fm._14);

	frstmPlanes[Top].n.x	=	fm._41 - fm._21;
	frstmPlanes[Top].n.y	=	fm._42 - fm._22;
	frstmPlanes[Top].n.z	=	fm._43 - fm._23;
	frstmPlanes[Top].d		= -(fm._44 - fm._24);

	frstmPlanes[Bottom].n.x	=	fm._41 + fm._21;
	frstmPlanes[Bottom].n.y	=	fm._42 + fm._22;
	frstmPlanes[Bottom].n.z	=	fm._43 + fm._23;
	frstmPlanes[Bottom].d	= -(fm._44 + fm._24);

	frstmPlanes[Near].n.x	=	fm._41 + fm._31;
	frstmPlanes[Near].n.y	=	fm._42 + fm._32;
	frstmPlanes[Near].n.z	=	fm._43 + fm._33;
	frstmPlanes[Near].d		= -(fm._44 + fm._34);

	frstmPlanes[Far].n.x	=	fm._41 - fm._31;
	frstmPlanes[Far].n.y	=	fm._42 - fm._32;
	frstmPlanes[Far].n.z	=	fm._43 - fm._33;
	frstmPlanes[Far].d		= -(fm._44 - fm._34);

	// find near points (intersections of 'side' planes eith near plane)
	frstmPoints[0] = intersection(frstmPlanes[Near], frstmPlanes[Left], frstmPlanes[Top]);		// ntl
	frstmPoints[1] = intersection(frstmPlanes[Near], frstmPlanes[Right], frstmPlanes[Top]);		// ntr
	frstmPoints[2] = intersection(frstmPlanes[Near], frstmPlanes[Right], frstmPlanes[Bottom]);	// nbr
	frstmPoints[3] = intersection(frstmPlanes[Near], frstmPlanes[Left], frstmPlanes[Bottom]);	// nbl
	
	// far points
	frstmPoints[4] = intersection(frstmPlanes[Far], frstmPlanes[Left], frstmPlanes[Top]);		// ftl
	frstmPoints[5] = intersection(frstmPlanes[Far], frstmPlanes[Right], frstmPlanes[Top]);		// ftr
	frstmPoints[6] = intersection(frstmPlanes[Far], frstmPlanes[Right], frstmPlanes[Bottom]);	// fbr
	frstmPoints[7] = intersection(frstmPlanes[Far], frstmPlanes[Left], frstmPlanes[Bottom]);	// fbl

	// normalise planes
	for (int i=0; i < 6; ++i) {
		frstmPlanes[i].normalise();
	}
}

/** project frustum edges onto a plane at avg map height & clip result to map bounds */
void SceneCuller::getFrustumExtents() {
	const GameCamera *cam = Game::getInstance()->getGameCamera();
	float alt = World::getCurrWorld()->getMap()->getAvgHeight();
	for (int i=0; i < 4; ++i) {
		Vec3f &pt = frstmPoints[i];
		Vec3f pt2 = frstmPoints[i+4];
		if (pt2.y >= pt.y) {
			pt2.y = pt.y - 0.1f;
		}
		Vec3f dir = pt2 - pt;
		float u = (alt - pt.y) / dir.y;
		intersectPoints[i] = pt + dir * u;
	}

	vector<Vec2f> in;
	for (int i=0; i < 4; ++i) {
		Vec2f p(intersectPoints[i].x, intersectPoints[i].z);
		in.push_back(p);
	}
	// push them out a bit, to avoid jaggies...
	Vec2f west = in[TopLeft] - in[TopRight];
	Vec2f south = in[BottomLeft] - in[TopLeft];
	west.normalize();
	south.normalize();
	in[BottomRight] += -west + south;
	in[BottomLeft]	+= west + south;
	in[TopLeft]		+= west - south;
	in[TopRight]	+= -west - south;

	in.push_back(in.front()); // close poly
	clipVisibleQuad(in);
}

/** the visit function of the line algorithm, sets cell & tile extrema as edges are evaluated */
void SceneCuller::cellVisit(int x, int y) {
	if (line_mirrored) {
		x = mirror_x + mirror_x - x;
	}
	int ty = y / 2 - tileExtrema.min_y;
	int tx = x / 2;
	y = y - cellExtrema.min_y;
	if (activeEdge == Left) {
		if (cellExtrema.edges.size() < y + 1) {
			cellExtrema.edges.push_back(pair<int,int>(x,-1));
		} else if (x < cellExtrema.edges[y].first) {
			cellExtrema.edges[y].first = x;
		}
		if (x < cellExtrema.min_x) {
			cellExtrema.min_x = x;
		}
		if (tileExtrema.edges.size() < ty + 1) {
			tileExtrema.edges.push_back(pair<int,int>(tx,-1));
		} else if ( tx < tileExtrema.edges[ty].first) {
			tileExtrema.edges[ty].first = tx;
		}
		if (tx < tileExtrema.min_x) {
			tileExtrema.min_x = tx;
		}
	} else {
		if (x > cellExtrema.edges[y].second) {
			cellExtrema.edges[y].second = x;
		}
		if (x > cellExtrema.max_x) {
			cellExtrema.max_x = x;
		}
		if (tx > tileExtrema.edges[ty].second) {
			tileExtrema.edges[ty].second = tx;
		}
		if (tx > tileExtrema.max_x) {
			tileExtrema.max_x = tx;
		}
	}
}

/** MidPoint line algorithm */
void SceneCuller::scanLine(int x0, int y0, int x1, int y1) {
	int dx = x1 - x0,
		dy = y1 - y0;
	int x = x0,
		y = y0;

	if ( dx == 0 ) {
		while (y <= y1) {
			cellVisit(x,y);
			++y;
		}
	} else if (dy > dx) {
		int d = 2 * dx - dy;
		int incrS = 2 * dx;
		int incrSE = 2 * (dx - dy);
		
		cellVisit(x,y);
		while (y < y1) {
			if (d <= 0) {
				d = d + incrS;
				y = y + 1;
			} else {
				d = d + incrSE;
				x = x + 1;
				y = y + 1;
			}
			cellVisit(x,y);
		}
	} else {
		int d = 2 * dy - dx;
		int incrE = 2 * dy;
		int incrSE = 2 * (dy - dx);
		
		cellVisit(x,y);
		while (x < x1) {
			if (d <= 0) {
				d = d + incrE;
				x = x + 1;
			} else {
				d = d + incrSE;
				x = x + 1;
				y = y + 1;
			}
			cellVisit(x,y);
		}
	}
}

/** evaluate a set of edges (the left or right side), calls scanLine() for each edge */
void SceneCuller::setVisibleExtrema(const vector<Edge> &edges) {
	for (vector<Edge>::const_iterator it = edges.begin(); it != edges.end(); ++it) {
		assert(it->first.y < it->second.y);
		if (it->first.x < it->second.x)  {
			line_mirrored = false;
			scanLine(it->first.x, it->first.y, it->second.x, it->second.y);
		} else {
			line_mirrored = true;
			mirror_x = it->first.x;
			int mx = it->first.x * 2 - it->second.x;
			scanLine(it->first.x, it->first.y, mx, it->second.y);
		}
	}
}

/** sort the edges into left and right edge lists, then evaluate edge lists */
void SceneCuller::setVisibleExtrema() {
	vector<Edge> allEdges;
	// get edges...
	for (int i=0; i < visiblePoly.size(); ++i) {
		Vec2i pt0 = Vec2i(visiblePoly[i]);
		Vec2i pt1 = Vec2i(visiblePoly[(i + 1) % visiblePoly.size()]);
		// put them all in allEdges, the 'right way' up (lower y val first)
		if (pt0.y == pt1.y) {
			continue; // cull horizontal
		} else if (pt0.y < pt1.y) {
			Edge foo(pt0, pt1);
			allEdges.push_back(foo);
		} else { // pt1.y < pt0.y
			Edge foo(pt1, pt0);
			allEdges.push_back(foo);
		}
	}
	// sort, putting lower first.y edges at the back (because that's where we'll remove them from)
	sort(allEdges.begin(), allEdges.end(), EdgeComp);

	if (allEdges.size() < 2) {
		cellExtrema.invalidate();
		tileExtrema.invalidate();
		return;
	}
	//REMOVE
	assert(allEdges.size() >= 2);
	const size_t &n = allEdges.size();
	assert(allEdges[n-1].first.y == allEdges[n-2].first.y);
	//REMOVE END

	int min_y = allEdges.back().first.y,
		max_y = allEdges.front().second.y;

	//DEBUG!!!
	vector<Edge> boo = vector<Edge>(allEdges);

	Edge e0 = allEdges.back();
	allEdges.pop_back();
	Edge e1 = allEdges.back();
	allEdges.pop_back();

	vector<Edge> edgeList1, edgeList2;

	bool firstIsLeft = false;

	edgeList1.push_back(e0);
	edgeList2.push_back(e1);

	while (!allEdges.empty()) {
		e0 = allEdges.back();
		allEdges.pop_back();
		if (e0.first == edgeList1.back().second) {
			edgeList1.push_back(e0);
		} else if (e0.first == edgeList2.back().second) {
			edgeList2.push_back(e0);
		} else {
			cellExtrema.invalidate();
			tileExtrema.invalidate();
			return;
		}
	}

	vector<Edge>::iterator it1 = edgeList1.begin();
	vector<Edge>::iterator it2 = edgeList2.begin();
	if (it1->first.x < it2->first.x) {
		firstIsLeft = true;
	} else if (it2->first.x < it2->first.x) {
		firstIsLeft = false;
	} else {
		do {
			if (it1->second.x < it2->second.x) {
				firstIsLeft = true;
				break;
			} else if (it2->second.x < it1->second.x) {
				firstIsLeft = false;
				break;
			}
			if ( it1->second.y < it2->second.y) {
				++it1;
			} else {
				++it2;
			}
		} while (it1 != edgeList1.end() && it2 != edgeList2.end());
		if (it1 == edgeList1.end() || it2 == edgeList2.end()) {
			cellExtrema.invalidate();
			tileExtrema.invalidate();
			return;
		}
	}
	assert(edgeList1.front().first.y == edgeList2.front().first.y);
	assert(edgeList1.back().second.y == edgeList2.back().second.y);

	cellExtrema.reset(min_y, max_y);
	tileExtrema.reset(min_y / 2,max_y / 2);

	activeEdge = Left;
	setVisibleExtrema(firstIsLeft ? edgeList1 : edgeList2);
	activeEdge = Right;
	setVisibleExtrema(firstIsLeft ? edgeList2 : edgeList1);

	assert(cellExtrema.min_x != cellExtrema.max_x);
}

/** Liang-Barsky polygon clip.  [see Foley & Van Damn 19.1.3] */
void SceneCuller::clipVisibleQuad(vector<Vec2f> &in) {
	const float min_x = 0.f, min_y = 0.f;
	const float max_x = World::getCurrWorld()->getMap()->getW() - 2.01f;
	const float max_y = World::getCurrWorld()->getMap()->getH() - 2.01f;

	vector<Vec2f> &out = visiblePoly;

	float	xIn, yIn, xOut, yOut;
	float	tOut1, tIn2, tOut2;
	float	tInX, tOutX, tInY, tOutY;
	float	deltaX, deltaY;
	
	out.clear();

	for (unsigned i=0; i < in.size() - 1; ++i) {
		deltaX = in[i+1].x - in[i].x;
		deltaY = in[i+1].y - in[i].y;
		
		// find paramater values for x,y 'entry' points
		if (deltaX > 0 || (deltaX == 0.f && in[i].x > max_x)) {
			xIn = min_x; xOut = max_x;
		} else {
			xIn = max_x; xOut = min_x;
		}
		if (deltaY > 0 || (deltaY == 0.f && in[i].y > max_y)) {
			yIn = min_y; yOut = max_y;
		} else {
			yIn = max_y; yOut = min_y;
		}

		// find parameter values for x,y 'exit' points
		if (deltaX) {
			tOutX = (xOut - in[i].x) / deltaX;
		} else if (in[i].x <= max_x && min_x <= in[i].x ) {
			tOutX = numeric_limits<float>::infinity();
		} else {
			tOutX = -numeric_limits<float>::infinity();
		}
		if (deltaY) {
			tOutY = (yOut - in[i].y) / deltaY;
		} else if (in[i].y <= max_y && min_y <= in[i].y) {
			tOutY = numeric_limits<float>::infinity();
		} else {
			tOutY = -numeric_limits<float>::infinity();
		}
		
		if (tOutX < tOutY) {
			tOut1 = tOutX;  tOut2 = tOutY;
		} else {
			tOut1 = tOutY;  tOut2 = tOutX;
		}

		if (tOut2 > 0.f) {
			if (deltaX) {
				tInX = (xIn - in[i].x) / deltaX;
			} else {
				tInX = -numeric_limits<float>::infinity();
			}
			if (deltaY) {
				tInY = (yIn - in[i].y) / deltaY;
			} else {
				tInY = -numeric_limits<float>::infinity();
			}

			if (tInX < tInY) {
				tIn2 = tInY;
			} else {
				tIn2 = tInX;
			}

			if (tOut1 < tIn2) { // edge does not cross map
				if (0.f < tOut1 && tOut1 <= 1.f) {
					// but it does go across a corner segment, add corner...
					if (tInX < tInY) {
						out.push_back(Vec2f(xOut, yIn));
					} else {
						out.push_back(Vec2f(xIn, yOut));
					}
				}
			} else {
				if (0.f < tOut1 && tIn2 <= 1.f) {
					if (0.f < tIn2) { // edge enters map
						if (tInX > tInY) {
							out.push_back(Vec2f(xIn, in[i].y + tInX * deltaY));
						} else {
							out.push_back(Vec2f(in[i].x + tInY * deltaX, yIn));
						}
					}
					if (1.f > tOut1) { // edge exits map
						if (tOutX < tOutY) {
							out.push_back(Vec2f(xOut, in[i].y + tOutX * deltaY));
						} else {
							out.push_back(Vec2f(in[i].x + tOutY * deltaX, yOut));
						}
					} else {
						out.push_back(Vec2f(in[i+1].x, in[i+1].y));
					}
				}
			}
			// edge end point on map
			if (0.f < tOut2 && tOut2 <= 1.f) {
				out.push_back(Vec2f(xOut, yOut));
			}
		} // if (tOut2 > 0.f)
	} // for each edge
}

}}