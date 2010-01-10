#include "pch.h"
#include "renderer.h"
#include "route_planner.h"   
#include "influence_map.h"
#include "cartographer.h"

#if _GAE_DEBUG_EDITION_

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using Glest::Game::Search::InfluenceMap;
using Glest::Game::Search::Cartographer;

namespace Glest { namespace Game{

list<Vec3f> DebugRenderer::waypoints;
set<Vec2i>	RegionHilightCallback::cells; 

void DebugRenderer::renderArrow(const Vec3f &pos1, const Vec3f &_pos2, const Vec3f &color, float width){
	const int tesselation = 3;
	const float arrowEndSize = 0.5f;

	Vec3f dir = Vec3f(_pos2 - pos1);
	float len = dir.length();
	float alphaFactor = 0.3f;

	dir.normalize();
	Vec3f pos2 = _pos2 - dir;
	Vec3f normal = dir.cross(Vec3f(0, 1, 0));

	Vec3f pos2Left  = pos2 + normal * (width - 0.05f) - dir * arrowEndSize * width;
	Vec3f pos2Right = pos2 - normal * (width - 0.05f) - dir * arrowEndSize * width;
	Vec3f pos1Left  = pos1 + normal * (width + 0.02f);
	Vec3f pos1Right = pos1 - normal * (width + 0.02f);

	//arrow body
	glBegin(GL_TRIANGLE_STRIP);
	for(int i=0; i<=tesselation; ++i){
		float t= static_cast<float>(i)/tesselation;
		Vec3f a= pos1Left.lerp(t, pos2Left);
		Vec3f b= pos1Right.lerp(t, pos2Right);
		Vec4f c= Vec4f(color, t*0.25f*alphaFactor);

		glColor4fv(c.ptr());
		glVertex3fv(a.ptr());
		glVertex3fv(b.ptr());
	}
	glEnd();

	//arrow end
	glBegin(GL_TRIANGLES);
		glVertex3fv((pos2Left + normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2Right - normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2 + dir*(arrowEndSize-0.1f)).ptr());
	glEnd();
}

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

Vec4f VisibleQuadColourCallback::colour( 0.f, 1.f, 0.f, 0.5f );
set<Vec2i> VisibleQuadColourCallback::quadSet;

set<Vec2i> PathfinderClusterOverlay::entranceCells;
set<Vec2i> PathfinderClusterOverlay::pathCells;


}} // end namespace Glest::Game

#endif // _GAE_DEBUG_EDITION_