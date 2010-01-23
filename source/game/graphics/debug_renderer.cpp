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

namespace Glest { namespace Game {


// =====================================================
//  class RegionHilightCallback
// =====================================================
set<Vec2i>	RegionHilightCallback::cells; 

// =====================================================
//  class ResourceMapOverlay
// =====================================================
const ResourceType *ResourceMapOverlay::rt;

const Unit* StoreMapOverlay::store = NULL;


// =====================================================
//  class PathFinderTextureCallback
// =====================================================
Field		PathFinderTextureCallBack::debugField;
Texture2D*	PathFinderTextureCallBack::PFDebugTextures[26];
set<Vec2i>	PathFinderTextureCallBack::pathSet, 
			PathFinderTextureCallBack::openSet, 
			PathFinderTextureCallBack::closedSet;
Vec2i		PathFinderTextureCallBack::pathStart, 
			PathFinderTextureCallBack::pathDest;
map<Vec2i,uint32> 
			PathFinderTextureCallBack::localAnnotations;

#define _load_tex(i,f)												\
   PFDebugTextures[i]=Renderer::getInstance().newTexture2D(rsGame);	\
   PFDebugTextures[i]->setMipmap(false);							\
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

// =====================================================
//  class VisibleQuadColourCallback
// =====================================================
Vec4f VisibleQuadColourCallback::colour( 0.f, 1.f, 0.f, 0.5f );
set<Vec2i> VisibleQuadColourCallback::quadSet;

// =====================================================
//  class PathfinderClusterOverlay
// =====================================================
set<Vec2i> PathfinderClusterOverlay::entranceCells;
set<Vec2i> PathfinderClusterOverlay::pathCells;

// =====================================================
// 	class DebugRender
// =====================================================
list<Vec3f> DebugRenderer::waypoints;

DebugRenderer::DebugRenderer() {
	AAStarTextures = HAAStarOverlay = showVisibleQuad = 
		captureVisibleQuad = regionHilights = 
		teamSight = resourceMapOverlay = storeMapOverlay = false;
}

bool findResourceMapRes(string &res) {
	ResourceMapOverlay::rt = NULL;
	const int &n = theWorld.getTechTree()->getResourceTypeCount();
	for (int i=0; i < n; ++i) {
		const ResourceType *rt = theWorld.getTechTree()->getResourceType(i);
		if (rt->getName() == res) {
			ResourceMapOverlay::rt = rt;
			return true;
		}
	}
	return false;
}

void DebugRenderer::init() {
	HAAStarOverlay = true;
	PathFinderTextureCallBack::debugField = Field::LAND;

	ResourceMapOverlay::rt = NULL;
	findResourceMapRes(string("wood"));
}

void DebugRenderer::commandLine(string &line) {
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
	} else if (key == "ResourceMap") {
		if ( val == "" ) { // no val supplied, toggle
			resourceMapOverlay = !resourceMapOverlay;
		} else {
			if ( val == "on" || val == "On" ) {
				resourceMapOverlay = true;
				storeMapOverlay = true;
			} else if (val == "off" || val == "Off") {
				resourceMapOverlay = false;
				storeMapOverlay = false;
			} else {
				// else find resource
				if (!findResourceMapRes(val)) {
					theConsole.addLine("Error: value=" + val + " not valid.");
					resourceMapOverlay = false;
					storeMapOverlay = false;
				}
				resourceMapOverlay = true;
				storeMapOverlay = true;
			}
			if (storeMapOverlay) {
				StoreMapOverlay::store = theWorld.findUnitById(0);
			}
		}
	} else if (key == "AssertClusterMap") {
		theWorld.getCartographer()->getClusterMap()->assertValid();
	} else if (key == "TransitionEdges") {
		if (val == "clear") {
			clusterEdgesNorth.clear();
			clusterEdgesWest.clear();
		} else {
			n = val.find(',');
			if (n == string::npos) {
				theConsole.addLine("Error: value=" + val + "not valid");
				return;
			}
			string xs = val.substr(0, n);
			val = val.substr(n + 1);
			int x = atoi(xs.c_str());
			n = val.find(':');
			if (n == string::npos) {
				theConsole.addLine("Error: value=" + val + "not valid");
				return;
			}
			string ys = val.substr(0, n);
			val = val.substr(n + 1);
			int y = atoi(ys.c_str());
			if (val == "north") {
				clusterEdgesNorth.insert(Vec2i(x, y));
			} else if ( val == "west") {
				clusterEdgesWest.insert(Vec2i(x, y));
			} else if ( val == "south") {
				clusterEdgesNorth.insert(Vec2i(x, y + 1));
			} else if ( val == "east") {
				clusterEdgesWest.insert(Vec2i(x + 1, y));
			} else if ( val == "all") {
				clusterEdgesNorth.insert(Vec2i(x, y));
				clusterEdgesNorth.insert(Vec2i(x, y + 1));
				clusterEdgesWest.insert(Vec2i(x, y));
				clusterEdgesWest.insert(Vec2i(x + 1, y));
			} else {
				theConsole.addLine("Error: value=" + val + "not valid");
			}
		}
	}
}

void DebugRenderer::renderCellTextured(const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
			const Vec3f &v1, const Vec3f &v2, const Vec3f &v3) {
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

void DebugRenderer::renderCellOverlay(const Vec4f colour,  const Vec3f &norm, 
		const Vec3f &v0, const Vec3f &v1, const Vec3f &v2, const Vec3f &v3) {
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

void DebugRenderer::renderArrow(
		const Vec3f &pos1, const Vec3f &_pos2, const Vec3f &color, float width) {
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

void DebugRenderer::renderPathOverlay() {
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

void DebugRenderer::renderIntraClusterEdges(const Vec2i &cluster, CardinalDir dir) {
	ClusterMap *cm = World::getInstance().getCartographer()->getClusterMap();
	const Map *map = World::getInstance().getMap();
	
	if (cluster.x < 0 || cluster.x >= cm->getWidth()
	|| cluster.y < 0 || cluster.y >= cm->getHeight()) {
		return;
	}

	Transitions transitions;
	if (dir != CardinalDir::COUNT) {
		TransitionCollection &tc = cm->getBorder(cluster, dir)->transitions[Field::LAND];
		for (int i=0; i < tc.n; ++i) {
			transitions.push_back(tc.transitions[i]);
		}
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
			//if (e->cost(1) != numeric_limits<float>::infinity()) {
				const Transition* t2 = e->transition();
				h = map->getCell(t2->nwPos)->getHeight();
				Vec3f t2Pos(t2->nwPos.x + 0.5f, h + 0.1f, t2->nwPos.y + 0.5f);
				renderArrow(t1Pos, t2Pos, Vec3f(1.f, 0.f, 1.f), 0.2f);
			//}
		}
	}
	//Restore
	glPopAttrib();
}

void DebugRenderer::renderEffects(Quad2i &quad) {
	if (regionHilights) {
		renderRegionHilight(quad);
	}
	if (showVisibleQuad) {
		renderCapturedQuad(quad);
	}
	if (teamSight) {
		renderTeamSightOverlay(quad);
	}
	if (HAAStarOverlay) {
		renderClusterOverlay(quad);
		renderPathOverlay();
		set<Vec2i>::iterator it;
		for (it = clusterEdgesWest.begin(); it != clusterEdgesWest.end(); ++it) {
			renderIntraClusterEdges(*it, CardinalDir::WEST);
		}		
		for (it = clusterEdgesNorth.begin(); it != clusterEdgesNorth.end(); ++it) {
			renderIntraClusterEdges(*it, CardinalDir::NORTH);
		}
	}
	if (resourceMapOverlay) {
		renderResourceMapOverlay(quad);
	}
	if (storeMapOverlay) {
		renderStoreMapOverlay(quad);
	}
}

}} // end namespace Glest::Game

#endif // _GAE_DEBUG_EDITION_