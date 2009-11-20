// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "window_gl.h"

#include "gl_wrap.h"
#include "graphics_interface.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;

namespace Shared{ namespace Platform{

// =====================================================
//	class WindowGl
// =====================================================
WindowGl::WindowGl(WindowStyle windowStyle, int x, int y, size_t width, size_t height, float freq,
		size_t colorBits, size_t depthBits, size_t stencilBits, const string &text)
		: Window(windowStyle, x, y, width, height, freq, colorBits, depthBits, stencilBits, text)
		, context(windowStyle == wsFullscreen, width, height, freq, colorBits, depthBits, stencilBits) {
}

void WindowGl::makeCurrentGl() {
	context.makeCurrent();
}

void WindowGl::swapBuffersGl() {
	context.swapBuffers();
}

}}//end namespace
