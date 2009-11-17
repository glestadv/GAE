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

#ifndef _SHARED_PLATFORM_GLWRAP_H_
#define _SHARED_PLATFORM_GLWRAP_H_

#if USE_SDL
#	include <SDL.h>
#	define GL_GLEXT_PROTOTYPES
#	include <SDL_opengl.h>
#elif defined(WIN32) || defined(WIN64)
#	include <windows.h>
#	include <GL/gl.h>
#	include <GL/glu.h>
#	include <glprocs.h>
#else
#	error No OpenGL implementation :(
#endif

#include <string>
#include <cassert>

#include "font.h"
#include "types.h"

using std::string;
using Shared::Graphics::FontMetrics;

namespace Shared { namespace Platform {

// =====================================================
// class PlatformContextGl
// =====================================================

class PlatformContextGl {
private:
	DeviceContextHandle dch;
	GlContextHandle glch;

public:
	PlatformContextGl(int colorBits, int depthBits, int stencilBits);
	virtual ~PlatformContextGl();

	void end();

	/**
	 * Makes this OpenGL context the calling thread's rendering context.  This is a Windows concept
	 * that other platforms could give a shit about.
	 */
	void makeCurrent();
	void swapBuffers();

	DeviceContextHandle getHandle() const {return dch;}

	// miscd
	void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics);
	void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics);
	string getPlatformExtensions() const;
	PROC getGlProcAddress(const char *procName);
};

}}//end namespace

#endif
