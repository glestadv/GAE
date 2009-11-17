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

#include "context_gl.h"

#if 0

#ifndef _SHARED_GRAPHICS_GL_OPENGL_H_
#define _SHARED_GRAPHICS_GL_OPENGL_H_

#include <cassert>
#include <stdexcept>
#include <string>

#include "conversion.h"
#include "gl_wrap.h"

using std::runtime_error;
using std::string;

namespace Shared{ namespace Graphics{ namespace Gl{

using Util::Conversion;

#define WGL_SAMPLE_BUFFERS_ARB	0x2041
#define WGL_SAMPLES_ARB			0x2042

// =====================================================
//	Globals
// =====================================================

bool isGlExtensionSupported(const char *extensionName);
bool isGlVersionSupported(int major, int minor, int release);
string getGlVersion();
string getGlRenderer();
string getGlVendor();
string getGlExtensions();
string getGlPlatformExtensions();
int getGlMaxLights();
int getGlMaxTextureSize();
int getGlMaxTextureUnits();
int getGlModelviewMatrixStackDepth();
int getGlProjectionMatrixStackDepth();
void checkGlExtension(const char *extensionName);

void inline _assertGl(const char *file, int line){

	GLenum error= glGetError();

	if(error != GL_NO_ERROR){
		const char *errorString= reinterpret_cast<const char*>(gluErrorString(error));
		throw runtime_error("OpenGL error: " + string(errorString) + " at file: "
				+ string(file) + ", line " + Conversion::toStr(line));
	}

}

#if defined(NDEBUG) || defined(NO_GL_ASSERTIONS)
#define assertGl() ((void) 0);

#else

#define assertGl() _assertGl(__FILE__, __LINE__);

#endif

}}}//end namespace

#endif
#endif