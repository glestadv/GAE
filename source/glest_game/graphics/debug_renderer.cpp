#include "pch.h"
#include "renderer.h"
#include "path_finder.h"   
#include "influence_map.h"
#include "cartographer.h"

#if DEBUG_RENDERING_ENABLED

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using Glest::Game::Search::InfluenceMap;
using Glest::Game::Search::Cartographer;

namespace Glest { namespace Game{

#if DEBUG_SEARCH_TEXTURES

Field		PathFinderTextureCallBack::debugField;
Texture2D*	PathFinderTextureCallBack::PFDebugTextures[26];
set<Vec2i>	PathFinderTextureCallBack::pathSet, 
			PathFinderTextureCallBack::openSet, 
			PathFinderTextureCallBack::closedSet;
Vec2i		PathFinderTextureCallBack::pathStart, 
			PathFinderTextureCallBack::pathDest;
map<Vec2i,uint32> 
			PathFinderTextureCallBack::localAnnotations;

#define _load_tex(i,f) \
   PFDebugTextures[i]=Renderer::getInstance().newTexture2D(rsGame);\
   PFDebugTextures[i]->setMipmap(false);\
   PFDebugTextures[i]->getPixmap()->load(f);

void PathFinderTextureCallBack::loadPFDebugTextures()
{
   char buff[128];
   for ( int i=0; i < 8; ++i )
   {
      sprintf ( buff, "data/core/misc_textures/g%02d.bmp", i );
      _load_tex ( i, buff );
   }
   _load_tex ( 9, "data/core/misc_textures/path_start.bmp" );
   _load_tex ( 10, "data/core/misc_textures/path_dest.bmp" );
   _load_tex ( 11, "data/core/misc_textures/path_both.bmp" );
   _load_tex ( 12, "data/core/misc_textures/path_return.bmp" );
   _load_tex ( 13, "data/core/misc_textures/path.bmp" );

   _load_tex ( 14, "data/core/misc_textures/path_node.bmp" );
   _load_tex ( 15, "data/core/misc_textures/open_node.bmp" );
   _load_tex ( 16, "data/core/misc_textures/closed_node.bmp" );

   for ( int i=17; i < 17+8; ++i )
   {
      sprintf ( buff, "data/core/misc_textures/l%02d.bmp", i-17 );
      _load_tex ( i, buff );
   }
}

#undef _load_tex
#endif // DEBUG_SEARCH_TEXTURES

#if DEBUG_SEARCH_OVERLAYS

#define INFLUENCE_SCALE 30.f

void Renderer::renderGoldInfluence() {
	const ResourceType *gold = NULL;
	for ( int i=0; i < theWorld.getTechTree()->getResourceTypeCount(); ++i ) {
		const ResourceType *rt = theWorld.getTechTree()->getResourceType( i );
		if ( rt->getName() == "gold" ) {
			gold = rt;
		}
	}
	if ( !gold ) assert( false );
	InfluenceMap *iMap = thePathManager.cartographer->getResourceMap( theWorld.getThisTeamIndex(), gold );
	renderInfluenceOverlay( Vec3f( 0.f, 1.f, 0.f ), INFLUENCE_SCALE, iMap );
}

void Renderer::renderInfluenceOverlay( Vec3f clr, float scale, InfluenceMap *iMap ) {
	const Rect2i mapBounds( 0, 0, theMap.getTileW() - 1, theMap.getTileH() - 1 );
	float coordStep = theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();

	assertGl();

	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT );

	glEnable( GL_BLEND );
	glEnable( GL_COLOR_MATERIAL ); 
	glDisable( GL_ALPHA_TEST );
	glActiveTexture( baseTexUnit );
	glDisable( GL_TEXTURE_2D );

	Quad2i scaledQuad = visibleQuad / Map::cellScale;

	Vec4f colour( clr );
	float inf = 0.f;

	PosQuadIterator pqi( scaledQuad );
	while ( pqi.next() ){
		const Vec2i &pos = pqi.getPos();
		int cx, cy;
		cx = pos.x * 2;
		cy = pos.y * 2;
		if ( mapBounds.isInside( pos ) ) {

			Tile *tc00= theMap.getTile( pos.x, pos.y );
			Tile *tc10= theMap.getTile( pos.x+1, pos.y );
			Tile *tc01= theMap.getTile( pos.x, pos.y+1 );
			Tile *tc11= theMap.getTile( pos.x+1, pos.y+1 );

			Vec3f tl = tc00->getVertex(); tl.y += 0.25f;
			Vec3f tr = tc10->getVertex(); tr.y += 0.25f;
			Vec3f bl = tc01->getVertex(); bl.y += 0.25f;
			Vec3f br = tc11->getVertex(); br.y += 0.25f;

			Vec3f tc = tl + (tr - tl) / 2;
			Vec3f ml = tl + (bl - tl) / 2;
			Vec3f mr = tr + (br - tr) / 2;
			Vec3f mc = ml + (mr - ml) / 2;
			Vec3f bc = bl + (br - bl) / 2;

			// cx,cy
			Vec2i cPos( cx, cy );
			inf = iMap->getInfluence( cPos );
			colour.w = clamp( inf / scale, 0.f, 1.f );

			glBegin ( GL_TRIANGLE_FAN );
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(tl.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(tc.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(mc.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(ml.ptr());                        
			glEnd ();

			cPos = Vec2i( cx+1, cy );
			inf = iMap->getInfluence( cPos );
			colour.w = clamp( inf / scale, 0.f, 1.f );

			glBegin ( GL_TRIANGLE_FAN );
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(tc.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(tr.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(mr.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(mc.ptr());                        
			glEnd ();

			cPos = Vec2i( cx, cy + 1 );
			inf = iMap->getInfluence( cPos );
			colour.w = clamp( inf / scale, 0.f, 1.f );

			glBegin ( GL_TRIANGLE_FAN );
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(ml.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(mc.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(bc.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(bl.ptr());                        
			glEnd ();

			cPos = Vec2i( cx + 1, cy + 1 );
			inf = iMap->getInfluence( cPos );
			colour.w = clamp( inf / scale, 0.f, 1.f );

			glBegin ( GL_TRIANGLE_FAN );
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(mc.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(mr.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(br.ptr());
			glNormal3fv(tc00->getNormal().ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(bc.ptr());                        
			glEnd ();
		}
	}
	glEnd();

	//Restore
	static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(false);
	glPopAttrib();

	//assert
	glGetError();	//remove when first mtex problem solved
	assertGl();
}
#endif // DEBUG_SEARCH_OVERLAYS


#if DEBUG_RENDERER_VISIBLEQUAD

Vec4f VisibleQuadColourCallback::colour( 0.f, 1.f, 0.f, 0.5f );
set<Vec2i> VisibleQuadColourCallback::quadSet;

#endif // DEBUG_RENDERER_VISIBLEQUAD

}} // end namespace Glest::Game

#endif // DEBUG_RENDERING_ENABLED