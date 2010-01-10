
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
		const int &clusterSize = Search::ClusterMap::clusterSize;
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

class DebugRenderer {
public:
	bool AAStarTextures, HAAStarOverlay, showVisibleQuad, captureVisibleQuad,
		regionHilights, teamSight;

	DebugRenderer () {
		AAStarTextures = HAAStarOverlay = true;
		showVisibleQuad = captureVisibleQuad = regionHilights = teamSight = false;
		PathFinderTextureCallBack::debugField = Field::LAND;
	}

	void commandLine(string &line) {
		string key, val;
		size_t n = line.find('=');
		if ( n != string::npos ) {
			key = line.substr(0, n);
			val = line.substr(n+1);
		} else {
			key = line;
		}
		if ( key == "AStarTextures" ) {
			if ( val == "" ) { // no val supplied, toggle
				AAStarTextures = !AAStarTextures;
			} else {
				if ( val == "on" || val == "On" ) {
					AAStarTextures = true;
				} else {
					AAStarTextures = false;
				}
			}
		} else if ( key == "ClusterOverlay" ) {
			if ( val == "" ) { // no val supplied, toggle
				HAAStarOverlay = !HAAStarOverlay;
			} else {
				if ( val == "on" || val == "On" ) {
					HAAStarOverlay = true;
				} else {
					HAAStarOverlay = false;
				}
			}
		} else if ( key == "CaptuereQuad" ) {
			captureVisibleQuad = true;
		} else if ( key == "RegionColouring" ) {
			if ( val == "" ) { // no val supplied, toggle
				regionHilights = !regionHilights;
			} else {
				if ( val == "on" || val == "On" ) {
					regionHilights = true;
				} else {
					regionHilights = false;
				}
			}
		} else if ( key == "DebugField" ) {
			Field f = FieldNames.match(val.c_str());
			if ( f != Field::INVALID ) {
				PathFinderTextureCallBack::debugField = f;
			} else {
				theConsole.addLine("Bad field: " + val);
			}
		}
	}

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
	void renderPFDebug( Quad2i &visibleQuad ) {
		renderCellTextures< PathFinderTextureCallBack >( visibleQuad );
	}
	void renderCapturedQuad( Quad2i &visibleQuad ) {
		renderCellOverlay< VisibleQuadColourCallback >( visibleQuad );
	}
	void renderTeamSightOverlay(Quad2i &visibleQuad) {
		renderCellOverlay<TeamSightColourCallback>(visibleQuad);
	}

private:
	void renderCellTextured( const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3  ) {
		glBindTexture( GL_TEXTURE_2D, tex->getHandle() );
		glBegin( GL_TRIANGLE_FAN );
			glTexCoord2f( 0.f, 1.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v0.ptr() );

			glTexCoord2f( 1.f, 1.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v1.ptr() );

			glTexCoord2f( 1.f, 0.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v2.ptr() );

			glTexCoord2f( 0.f, 0.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v3.ptr() );                        
		glEnd ();
	}

	void renderCellOverlay( const Vec4f colour,  const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3  ) {
		glBegin ( GL_TRIANGLE_FAN );
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v0.ptr());
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v1.ptr());
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v2.ptr());
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v3.ptr());                        
		glEnd ();
	}

	void renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width);

	static list<Vec3f> waypoints;
public:
	static void clearWaypoints()		{ waypoints.clear();		}
	static void addWaypoint(Vec3f v)	{ waypoints.push_back(v);	}

	void renderPathOverlay() {
		//return;
		Vec3f one, two;
		if ( waypoints.size() < 2 ) return;

		assertGl();
		glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT );
		glEnable( GL_COLOR_MATERIAL ); 
		glDisable( GL_ALPHA_TEST );
		glDepthFunc(GL_ALWAYS);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glLineWidth(2.f);
		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );

		list<Vec3f>::iterator it = waypoints.begin(); 
		one = *it;
		++it;
		two = *it;
		while ( true ) {
			renderArrow(one,two,Vec3f(1.0f, 1.0f, 0.f), 0.15f);
			one = two;
			++it;
			if ( it == waypoints.end() ) break;
			two = *it;
		}
		//Restore
		glPopAttrib();
	}

	void renderIntraCusterEdges(const Vec2i &cluster, CardinalDir dir = CardinalDir::COUNT) {
		ClusterMap *cm = World::getInstance().getCartographer()->getClusterMap();
		const Map *map = World::getInstance().getMap();
		
		Transitions transitions;
		if (dir != CardinalDir::COUNT) {
			Transitions &bt = cm->getBorder(cluster, dir)->transitions[Field::LAND];
			transitions.insert(transitions.end(), bt.begin(), bt.end());
		} else {
			cm->getTransitions(cluster, Field::LAND, transitions);
		}		
		assertGl();
		glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT );
		glEnable( GL_COLOR_MATERIAL ); 
		glDisable( GL_ALPHA_TEST );
		glDepthFunc(GL_ALWAYS);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glLineWidth(2.f);
		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );

		for (Transitions::iterator ti = transitions.begin(); ti != transitions.end(); ++ti) {
			const Transition* &t = *ti;
			float h = map->getCell(t->nwPos)->getHeight();
			Vec3f t1Pos(t->nwPos.x + 0.5f, h + 0.1f, t->nwPos.y + 0.5f);
			for (Edges::const_iterator ei = t->edges.begin(); ei != t->edges.end(); ++ei) {
				Edge * const &e = *ei;
				if (e->cost(1) != numeric_limits<float>::infinity()) {
					Transition* &t2 = e->first;
					h = map->getCell(t2->nwPos)->getHeight();
					Vec3f t2Pos(t2->nwPos.x + 0.5f, h + 0.1f, t2->nwPos.y + 0.5f);
					renderArrow(t1Pos, t2Pos, Vec3f(1.f, 0.f, 1.f), 0.2f);
				}
			}
		}
		//Restore
		glPopAttrib();
	}
};

}}

#endif
