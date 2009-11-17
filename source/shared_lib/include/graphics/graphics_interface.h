// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#if 0  // We don't need no stinkin' badges!

#ifndef _SHARED_GRAPHICS_GRAPHICSINTERFACE_H_
#define _SHARED_GRAPHICS_GRAPHICSINTERFACE_H_

#include "patterns.h"

namespace Shared { namespace Graphics {

class GraphicsFactory;
class Context;

// =====================================================
//	class GraphicsInterface
//
///	Interface for the graphic engine
// =====================================================

class GraphicsInterface : Uncopyable {
private:
	GraphicsFactory *graphicsFactory;
	Context *currentContext;

private:
	friend class TextureManager;
	friend class FontManager;

private:
	GraphicsInterface();

public:
	static GraphicsInterface &getInstance();

	void setFactory(GraphicsFactory *graphicsFactory);
	void setCurrentContext(Context *context);

	Context *getCurrentContext() const		{return currentContext;}
	GraphicsFactory *getFactory() const		{return graphicsFactory;}
};

}}//end namespace

#endif
#endif
