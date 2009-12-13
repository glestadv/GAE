// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "gl_wrap.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

// work-around for nasty bug! http://thread.gmane.org/gmane.comp.lib.boost.user/47996
#include <boost/foreach.hpp>

#include <SDL.h>
#ifdef X11_AVAILABLE
#include <GL/glx.h>
#endif


#include "exception_base.h"
#include "opengl.h"
//#include "sdl_private.h"
#include "noimpl.h"

#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using Shared::Util::GlestException;

namespace Shared { namespace Platform {

// ======================================
// class PlatformContextGl
// ======================================
#if 0
void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits) {

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
	int flags = SDL_OPENGL;

	if (Private::shouldBeFullscreen) {
		flags |= SDL_FULLSCREEN;
	}

	int resW = Private::ScreenWidth;
	int resH = Private::ScreenHeight;
	SDL_Surface* screen = SDL_SetVideoMode(resW, resH, colorBits, flags);
	if (screen == 0) {
		std::ostringstream msg;
		msg << "Couldn't set video mode "
			<< resW << "x" << resH << " (" << colorBits
			<< "bpp " << stencilBits << " stencil "
			<< depthBits << " depth-buffer). SDL Error is: " << SDL_GetError();
		throw std::runtime_error(msg.str());
	}
}
#endif

void PlatformContextGl::end() {
}

void PlatformContextGl::makeCurrent() {
}

void PlatformContextGl::swapBuffers() {

}

void PlatformContextGl::createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
		int charCount, FontMetrics &metrics) {
#ifdef X11_AVAILABLE
	Display* display = glXGetCurrentDisplay();
	if (display == 0) {
		throw std::runtime_error("Couldn't create font: display is 0");
	}
	XFontStruct* fontInfo = XLoadQueryFont(display, type.c_str());
	if (!fontInfo) {
		throw std::runtime_error("Font not found.");
	}

	// we need the height of 'a' which sould ~ be half ascent+descent
	metrics.setHeight(static_cast<float>(fontInfo->ascent + fontInfo->descent) / 2);
	for (unsigned int i = 0; i < static_cast<unsigned int>(charCount); ++i) {
		if (i < fontInfo->min_char_or_byte2 || i > fontInfo->max_char_or_byte2) {
			metrics.setWidth(i, 6.f);
		} else {
			int p = i - fontInfo->min_char_or_byte2;
			metrics.setWidth(i, static_cast<float>(fontInfo->per_char[p].rbearing
					- fontInfo->per_char[p].lbearing));
		}
	}

	glXUseXFont(fontInfo->fid, 0, charCount, base);
	XFreeFont(display, fontInfo);
#else
#	error we badly need a solution portable to more than just glx
#endif
}

void PlatformContextGl::createGlFontOutlines(uint32 &base, const string &type, int width,
		float depth, int charCount, FontMetrics &metrics) {
	throw runtime_error("PlatformContextGl::createGlFontOutlines not implemented");
}

PROC PlatformContextGl::getGlProcAddress(const char *procName) {
	PROC proc = SDL_GL_GetProcAddress(procName);
	if(!proc) {
		string msg = string() + "OpenGL function not found: " + procName;
		GlestException::coldThrow(msg, "SDL_GL_GetProcAddress", NULL, __FILE__, __LINE__);
	}
	assert(proc);
	return proc;
}

string PlatformContextGl::getPlatformExtensions() const {
	return "";
}

}}//end namespace
