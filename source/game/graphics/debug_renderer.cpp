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
#include "renderer.h"

#if DEBUG_RENDERING_ENABLED

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Glest { namespace Game{

set<Vec2i>	RegionHilightCallback::blueCells,
			RegionHilightCallback::greenCells; 

Field		PathFinderTextureCallBack::debugField;
Texture2D*	PathFinderTextureCallBack::PFDebugTextures[26];
set<Vec2i>	PathFinderTextureCallBack::pathSet, 
			PathFinderTextureCallBack::openSet, 
			PathFinderTextureCallBack::closedSet;
Vec2i		PathFinderTextureCallBack::pathStart, 
			PathFinderTextureCallBack::pathDest;
map<Vec2i,uint32> 
			PathFinderTextureCallBack::localAnnotations;

Texture2D*	GridTextureCallback::tex;

void _load_debug_tex(Texture2D* &t, const char *f) {
	t = Renderer::getInstance().newTexture2D(rsGame);
	t->setMipmap(false);
	t->getPixmap()->load(f);
}

void DebugRenderer::init() {
	char buff[128];

#	define _load_tex(i,f) _load_debug_tex(PathFinderTextureCallBack::PFDebugTextures[i],f)
	
	for (int i=0; i < 8; ++i) {
		sprintf(buff, "data/core/misc_textures/g%02d.bmp", i);
		_load_tex(i, buff);
	}
	_load_tex(9, "data/core/misc_textures/path_start.bmp");
	_load_tex(10, "data/core/misc_textures/path_dest.bmp");
	_load_tex(11, "data/core/misc_textures/path_both.bmp");
	_load_tex(12, "data/core/misc_textures/path_return.bmp");
	_load_tex(13, "data/core/misc_textures/path.bmp");

	_load_tex(14, "data/core/misc_textures/path_node.bmp");
	_load_tex(15, "data/core/misc_textures/open_node.bmp");
	_load_tex(16, "data/core/misc_textures/closed_node.bmp");

	for (int i=17; i < 17+8; ++i) {
		sprintf(buff, "data/core/misc_textures/l%02d.bmp", i-17);
		_load_tex(i, buff);
	}
#	undef _load_tex

   _load_debug_tex(GridTextureCallback::tex, "data/core/misc_textures/grid.bmp");
}

}} // end namespace Glest::Game

#endif // DEBUG_RENDERING_ENABLED
