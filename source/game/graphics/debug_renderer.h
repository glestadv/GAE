
#if ! _GAE_DEBUG_EDITION_
#	error debug_renderer.h included without _GAE_DEBUG_EDITION_
#endif

#ifndef _GLEST_GAME_DEBUG_RENDERER_
#define _GLEST_GAME_DEBUG_RENDERER_

#include "route_planner.h"   
#include "influence_map.h"
#include "cartographer.h"
#include "cluster_map.h"

#include "vec.h"
#include "math_util.h"
#include "pixmap.h"
#include "texture.h"
#include "graphics_factory_gl.h"
#include "game.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using Glest::Game::Search::InfluenceMap;
using Glest::Game::Search::ClusterMap;
using Glest::Game::Search::Cartographer;

namespace Glest { namespace Game {

class PathFinderTextureCallBack {
public:
	static set<Vec2i> pathSet, openSet, closedSet;
	static Vec2i pathStart, pathDest;
	static map<Vec2i,uint32> localAnnotations;
	
	static Field debugField;
	static void loadPFDebugTextures ();
	static Texture2D *PFDebugTextures[26];

	Texture2DGl* operator() ( const Vec2i &cell ) {
		int ndx = -1;
		if ( pathStart == cell ) ndx = 9;
		else if ( pathDest == cell ) ndx = 10;
		else if ( pathSet.find(cell) != pathSet.end() ) ndx = 14; // on path
		else if ( closedSet.find(cell) != closedSet.end() ) ndx = 16; // closed nodes
		else if ( openSet.find(cell) != openSet.end() ) ndx = 15; // open nodes
		else if ( localAnnotations.find(cell) != localAnnotations.end() ) // local annotation
			ndx = 17 + localAnnotations.find(cell)->second;
		else ndx = theWorld.getCartographer()->getMasterMap()->metrics[cell].get(debugField); // else use cell metric for debug field
		return (Texture2DGl*)PFDebugTextures[ndx];
   }
};

class RegionHilightCallback {
public:
	static set<Vec2i> cells;

	Vec4f operator()( const Vec2i &cell ) {
		Vec4f colour(0.f, 0.f, 1.f, 0.f);
		if ( cells.find(cell) == cells.end() ) {
			colour.w = 0.f;
		} else {
			colour.w = 0.6f;
		}
		return colour;
	}
};

class VisibleQuadColourCallback {
public:
	static set<Vec2i> quadSet;
	static Vec4f colour;

	Vec4f operator() ( const Vec2i &cell ) {
		if ( quadSet.find( cell ) == quadSet.end() ) {
			colour.w = 0.f;
		} else {
			colour.w = 0.6f;
		}
		return Vec4f(colour);
	}
};

class TeamSightColourCallback {
public:
	Vec4f operator()( const Vec2i &cell ) {
		Vec4f colour(0.f);
		const Vec2i &tile = Map::toTileCoords(cell);
		int vis = theWorld.getCartographer()->getTeamVisibility(theWorld.getThisTeamIndex(), tile);
		if ( !vis ) {
			colour.x = 1.0f;
			colour.w = 0.1f;
		} else {
			colour.z = 1.0f;
			switch ( vis ) {
				case 1:  colour.w = 0.05f;	break;
				case 2:  colour.w = 0.1f;	break;
				case 3:  colour.w = 0.15f;	break;
				case 4:  colour.w = 0.2f;	break;
				case 5:  colour.w = 0.25f;	break;
				default: colour.w = 0.3f;
			}
		}
		return colour;
	}
};

class PathfinderClusterOverlay {
public:
	static set<Vec2i> entranceCells;
	static set<Vec2i> pathCells;

	Vec4f operator()( const Vec2i &cell ) {
		const int &clusterSize = Search::clusterSize;
		if ( cell.x % clusterSize == clusterSize - 1 
		|| cell.y % clusterSize == clusterSize - 1  ) {
			if ( entranceCells.find(cell) != entranceCells.end() ) {
				return Vec4f(0.f, 1.f, 0.f, 0.7f); // entrance
			} else {
				return Vec4f(1.f, 0.f, 0.f, 0.7f);  // border
			}
		} else if ( pathCells.find(cell) != pathCells.end() ) { // intra-cluster edge
			return Vec4f(0.f, 0.f, 1.f, 0.7f);
		} else {
			return Vec4f(0.f, 0.f, 0.f, 0.f); // nothing interesting
		}
	}
};

class ResourceMapOverlay {
public:
	static const ResourceType *rt;

	Vec4f operator()(const Vec2i &cell) {
		PatchMap<1> *pMap = theWorld.getCartographer()->getResourceMap(rt);
		if (pMap && pMap->getInfluence(cell) == 1) {
			return Vec4f(1.f, 1.f, 0.f, 0.7f);
		} else {
			return Vec4f(1.f, 1.f, 1.f, 0.f);
		}
	}
};

// =====================================================
// 	class DebugRender
//
/// Helper class compiled with _GAE_DEBUG_EDITION_ only
// =====================================================
class DebugRenderer {
private:
	set<Vec2i> clusterEdgesWest;
	set<Vec2i> clusterEdgesNorth;

public:
	DebugRenderer();
	void init();
	void commandLine(string &line);

	bool AAStarTextures, HAAStarOverlay, showVisibleQuad, captureVisibleQuad,
		regionHilights, teamSight, resourceMapOverlay;

private:
	template< typename CellTextureCallback >
	void renderCellTextures ( Quad2i &visibleQuad ) {
		const Rect2i mapBounds(0, 0, theMap.getTileW()-1, theMap.getTileH()-1);
		float coordStep= theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		assertGl();

		glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);
		glEnable(GL_BLEND);
		glEnable(GL_COLOR_MATERIAL); 
		glDisable(GL_ALPHA_TEST);
		glActiveTexture( GL_TEXTURE0 );

		Quad2i scaledQuad = visibleQuad / Map::cellScale;
		PosQuadIterator pqi( scaledQuad );
		CellTextureCallback callback;
		while ( pqi.next() ) {
			const Vec2i &pos= pqi.getPos();
			int cx, cy;
			cx = pos.x * 2;
			cy = pos.y * 2;
			if(mapBounds.isInside(pos)){
				Tile *tc00 = theMap.getTile(pos.x, pos.y), *tc10 = theMap.getTile(pos.x+1, pos.y),
					*tc01 = theMap.getTile(pos.x, pos.y+1), *tc11 = theMap.getTile(pos.x+1, pos.y+1);
				Vec3f tl = tc00->getVertex (), tr = tc10->getVertex (),
					bl = tc01->getVertex (), br = tc11->getVertex ();
				Vec3f tc = tl + (tr - tl) / 2,  ml = tl + (bl - tl) / 2,
					mr = tr + (br - tr) / 2, mc = ml + (mr - ml) / 2, bc = bl + (br - bl) / 2;
				Vec2i cPos ( cx, cy );
				const Texture2DGl *tex = callback( cPos );
				renderCellTextured( tex, tc00->getNormal(), tl, tc, mc, ml );
				cPos = Vec2i( cx+1, cy );
				tex = callback( cPos );
				renderCellTextured( tex, tc00->getNormal(), tc, tr, mr, mc );
				cPos = Vec2i( cx, cy + 1 );
				tex = callback( cPos );
				renderCellTextured( tex, tc00->getNormal(), ml, mc, bc, bl );
				cPos = Vec2i( cx + 1, cy + 1 );
				tex = callback( cPos );
				renderCellTextured( tex, tc00->getNormal(), mc, mr, br, bc );
			}
		}
		//Restore
		glPopAttrib();
		//assert
		glGetError();	//remove when first mtex problem solved
		assertGl();

	} // renderCellTextures ()

	template< typename CellColourCallback >
	void renderCellOverlay ( Quad2i &visibleQuad ) {
		const Rect2i mapBounds( 0, 0, theMap.getTileW() - 1, theMap.getTileH() - 1 );
		float coordStep = theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		Vec4f colour;
		assertGl();
		glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT );
		glEnable( GL_BLEND );
		glEnable( GL_COLOR_MATERIAL ); 
		glDisable( GL_ALPHA_TEST );
		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );
		Quad2i scaledQuad = visibleQuad / Map::cellScale;
		PosQuadIterator pqi( scaledQuad );
		CellColourCallback callback;
		while ( pqi.next() ){
			const Vec2i &pos = pqi.getPos();
			int cx, cy;
			cx = pos.x * 2;
			cy = pos.y * 2;
			if ( mapBounds.isInside( pos ) ) {
				Tile *tc00= theMap.getTile( pos.x, pos.y ),		*tc10= theMap.getTile( pos.x+1, pos.y ),
					 *tc01= theMap.getTile( pos.x, pos.y+1 ),	*tc11= theMap.getTile( pos.x+1, pos.y+1 );
				Vec3f tl = tc00->getVertex(),	tr = tc10->getVertex(),
					  bl = tc01->getVertex(),	br = tc11->getVertex(); 
				tl.y += 0.25f; tr.y += 0.25f; bl.y += 0.25f; br.y += 0.25f;
				Vec3f tc = tl + (tr - tl) / 2,	ml = tl + (bl - tl) / 2,	mr = tr + (br - tr) / 2,
					  mc = ml + (mr - ml) / 2,	bc = bl + (br - bl) / 2;

				colour = callback( Vec2i(cx,cy ) );
				renderCellOverlay( colour, tc00->getNormal(), tl, tc, mc, ml );
				colour = callback( Vec2i(cx+1, cy) );
				renderCellOverlay( colour, tc00->getNormal(), tc, tr, mr, mc );
				colour = callback( Vec2i(cx, cy + 1) );
				renderCellOverlay( colour, tc00->getNormal(), ml, mc, bc, bl );
				colour = callback( Vec2i(cx + 1, cy + 1) );
				renderCellOverlay( colour, tc00->getNormal(), mc, mr, br, bc );
			}
		}
		//Restore
		glPopAttrib();
		//assert
		glGetError();	//remove when first mtex problem solved
		assertGl();
	}

	void renderClusterOverlay( Quad2i &visibleQuad ) {
		renderCellOverlay<PathfinderClusterOverlay>(visibleQuad);
	}
	void renderRegionHilight(Quad2i &visibleQuad) {
		renderCellOverlay<RegionHilightCallback>(visibleQuad);
	}
	void renderCapturedQuad( Quad2i &visibleQuad ) {
		renderCellOverlay< VisibleQuadColourCallback >( visibleQuad );
	}
	void renderTeamSightOverlay(Quad2i &visibleQuad) {
		renderCellOverlay<TeamSightColourCallback>(visibleQuad);
	}
	void renderResourceMapOverlay(Quad2i &visibleQuad) {
		renderCellOverlay<ResourceMapOverlay>(visibleQuad);
	}

	void renderCellTextured(const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3);
	void renderCellOverlay(const Vec4f colour,  const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3);
	void renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width);

	static list<Vec3f> waypoints;

	void renderPathOverlay();
	void renderIntraClusterEdges(const Vec2i &cluster, CardinalDir dir = CardinalDir::COUNT);

public:
	static void clearWaypoints()		{ waypoints.clear();		}
	static void addWaypoint(Vec3f v)	{ waypoints.push_back(v);	}

	bool willRenderSurface() const { return AAStarTextures; }
	void renderSurface(Quad2i &quad) { renderCellTextures< PathFinderTextureCallBack >(quad); }
	void renderEffects(Quad2i &quad);
};

}}

#endif
