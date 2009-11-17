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

#include "pch.h"
#include "context.h"

#include "leak_dumper.h"

namespace Shared { namespace Graphics {

// =====================================================
// class Context
// =====================================================

Context::Context(uint32 colorBits, uint32 depthBits, uint32 stencilBits)
		: colorBits(colorBits)
		, depthBits(depthBits)
		, stencilBits(stencilBits) {
}

Context::~Context() {
}

}}//end namespace
#endif