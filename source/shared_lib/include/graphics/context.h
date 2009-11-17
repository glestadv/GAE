// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//					   2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#if 0  // We don't need no stinkin' badges!

#ifndef _SHARED_GRAPHICS_CONTEXT_H_
#define _SHARED_GRAPHICS_CONTEXT_H_

#include "types.h"

using Shared::Platform::uint32;

namespace Shared { namespace Graphics {


// =====================================================
// class Context
// =====================================================

/**
 * A generic graphics context.  In practice, this class is pretty stupid because we only suport
 * OpenGL and may never support anything else.  None the less, it keeps the concept abstracted so
 * that supporting anything other than OpenGL is possible --but why would you want to?
 */
class Context {
private:
	uint32 colorBits;
	uint32 depthBits;
	uint32 stencilBits;

public:
//	Context(uint32 colorBits = 32, uint32 depthBits = 24, uint32 stencilBits = 0);
	Context(uint32 colorBits, uint32 depthBits, uint32 stencilBits);
	virtual ~Context();

	uint32 getColorBits() const		{return colorBits;}
	uint32 getDepthBits() const		{return depthBits;}
	uint32 getStencilBits() const	{return stencilBits;}

	void setColorBits(uint32 v)		{colorBits = v;}
	void setDepthBits(uint32 v)		{depthBits = v;}
	void setStencilBits(uint32 v)	{stencilBits = v;}

//	virtual void init() = 0;
	virtual void end() = 0;
	virtual void reset() = 0;

	virtual void makeCurrent() = 0;
	virtual void swapBuffers() = 0;
};

}}//end namespace

#endif
#endif