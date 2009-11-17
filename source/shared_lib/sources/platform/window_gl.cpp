// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
WindowGl::WindowGl(WindowStyle windowStyle, int x, int y, int w, int h, int freq, const string &text, int colorBits, int depthBits, int stencilBits)
	: context()
{
}
void WindowGl::initGl(int colorBits, int depthBits, int stencilBits) {
	context.setColorBits(colorBits);
	context.setDepthBits(depthBits);
	context.setStencilBits(stencilBits);

	context.init();
}

void WindowGl::makeCurrentGl() {
	GraphicsInterface::getInstance().setCurrentContext(&context);
	context.makeCurrent();
}

void WindowGl::swapBuffersGl() {
	context.swapBuffers();
}

}}//end namespace
