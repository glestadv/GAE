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

#ifndef _GAME_SCENE_CULLER_INCLUDED_
#define _GAME_SCENE_CULLER_INCLUDED_

#include "vec.h"
#include "math_util.h"

#include "game_camera.h"

#include <vector>
#include <list>
#include <limits>
#include <cassert>

namespace Glest {

namespace Debug {
	class DebugRenderer;
}

namespace Graphics {
using std::list;
using std::vector;
using std::pair;
using namespace Shared::Math;
using std::swap;

class SceneCuller {
	friend class Debug::DebugRenderer;
private:
	enum { Left, Right, Top, Bottom, Near, Far };
	Plane frstmPlanes[6];

	enum { 
		NearBottomLeft, NearTopLeft, NearTopRight, NearBottomRight,
		FarBottomLeft, FarTopLeft, FarTopRight, FarBottomRight
	};
	Vec3f frstmPoints[8];
	
	enum { TopRight, BottomRight, BottomLeft, TopLeft };

	struct Line {
		Vec3f origin, magnitude;
	};
	struct RayInfo {
		Line  line;
		float last_u;
		Vec2f last_intersect;
		bool valid;

		// construct from two points (near,far)
		RayInfo(Vec3f pt1, Vec3f pt2) {
			line.origin = pt1;
			if (pt2.y >= pt1.y) { 
				// a bit hacky, but means we don't need special case code elsewhere
				pt2.y = pt1.y - 0.1f;
			}
			line.magnitude = pt2 - pt1;
			castRay();
			if (last_u > 0.f) valid = true;
			else valid = false;
		}
		// constrcut from two other rays, interpolating line 
		RayInfo(const RayInfo &ri1, const RayInfo &ri2, const float frac) {
			line.origin = (ri1.line.origin + ri2.line.origin) * frac;
			line.magnitude = (ri1.line.magnitude + ri2.line.magnitude) * frac;
			castRay();
			if (last_u > 0.f) valid = true;
			else valid = false;
		}
		void castRay();
	};
	list<RayInfo> rays;

	vector<Vec2f> boundingPoints;
	vector<Vec2f> visiblePoly;

	typedef pair<Vec2i,Vec2i> Edge;

	static bool EdgeComp(const Edge &e0, const Edge &e1) {
		return (e0.first.y < e1.first.y);
	}

	typedef pair<int,int> RowExtrema;

	struct Extrema {
		int min_y, max_y;
		int min_x, max_x;
		vector<RowExtrema> spans;

		Rect2i getBounds() const { return Rect2i(min_x - 1, min_y - 1, max_x + 1, max_y + 1); }
		void reset(int minY, int maxY);
		void invalidate() { min_y = max_y = min_x = max_x = 0;  }

	} cellExtrema, tileExtrema;

	void extractFrustum();
	bool isPointInFrustum(const Vec3f &point) const;
	bool isSphereInFrustum(const Vec3f &centre, const float radius) const;

	Vec3f intersection(const Plane &p1, const Plane &p2, const Plane &p3) const;
	bool getFrustumExtents();
	void clipVisibleQuad(vector<Vec2f> &in);
	void setVisibleExtrema();

	template < typename EdgeIt >
	void setVisibleExtrema(const EdgeIt &start, const EdgeIt &end);

	///@todo remove, use Shared::Util::line()
	void scanLine(int x0, int y0, int x1, int y1);
	///@todo plug into line(), no need for inlining, just use adapter from functional.
	void cellVisit(int x, int y);
	
	int activeEdge;
	bool line_mirrored;
	int mirror_x;

public:
	SceneCuller()
			: activeEdge(Left)
			, line_mirrored(false)
			, mirror_x(0) {
		visiblePoly.reserve(10);
	}
	void establishScene();
	bool isInside(Vec2i pos) const;

	class iterator {
		friend class SceneCuller;
		iterator() : extrema(NULL), x(-1), y(-1) {}
		iterator(const Extrema *extrema, bool start=true) : extrema(extrema) {
			if (start) {
				x = (extrema->max_y - extrema->min_y) ? extrema->spans[0].first : -1;
				y = extrema->min_y;
			} else { // end
				x = -1;
				y = extrema->min_y + extrema->spans.size();
			}
		}
		const Extrema *extrema;
		int x, y;
		
	public:
		void operator++() {
			const int &row = y - extrema->min_y;
			if (x == extrema->spans[row].second) {
				++y;
				x = (row == extrema->spans.size() - 1 ? -1 : extrema->spans[row+1].first);
			} else {
				++x;
			}
		}
		Vec2i operator*() const { return Vec2i(x,y); }
		bool operator==(const iterator &that) const { return (x == that.x && y == that.y); }
		bool operator!=(const iterator &that) const { return !(*this == that); }
	};

	iterator cell_end_cached, tile_end_cached;

	iterator cell_begin() { 
		cell_end_cached = iterator(&cellExtrema, false);
		return iterator(&cellExtrema);
	}
	iterator cell_end()	{ 
		return cell_end_cached; 
	}

	iterator tile_begin() { 
		tile_end_cached = iterator(&tileExtrema, false);
		return iterator(&tileExtrema);
	}
	iterator tile_end() {
		return tile_end_cached;
	}

	Rect2i getBoundingRectCell() const { return cellExtrema.getBounds();	}
	Rect2i getBoundingRectTile() const { return tileExtrema.getBounds();	}
};


}}

#endif
