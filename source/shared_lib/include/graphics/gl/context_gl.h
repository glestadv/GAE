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

#ifndef _SHARED_GRAPHICS_GL_CONTEXTGL_H_
#define _SHARED_GRAPHICS_GL_CONTEXTGL_H_

#include <cassert>
#include <stdexcept>
#include <string>

#include "context.h"
#include "gl_wrap.h"
#include "types.h"
#include "conversion.h"

using std::runtime_error;
using std::string;
using Shared::Util::Conversion;
using Shared::Platform::uint32;
using Shared::Platform::PlatformContextGl;

namespace Shared { namespace Graphics { namespace Gl {



// =====================================================
// class ContextGl
// =====================================================

class ContextGl /*: public Context */{
private:
	uint32 colorBits;
	uint32 depthBits;
	uint32 stencilBits;
	PlatformContextGl pcgl;

public:
	ContextGl(uint32 colorBits, uint32 depthBits, uint32 stencilBits);
	~ContextGl();

	uint32 getColorBits() const		{return colorBits;}
	uint32 getDepthBits() const		{return depthBits;}
	uint32 getStencilBits() const	{return stencilBits;}

	void setColorBits(uint32 v)		{colorBits = v;}
	void setDepthBits(uint32 v)		{depthBits = v;}
	void setStencilBits(uint32 v)	{stencilBits = v;}

	void end();
	void reset();
	void makeCurrent();
	void swapBuffers();

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

	static void inline _assertGl(const char *file, int line) {

		GLenum error= glGetError();

		if(error != GL_NO_ERROR){
			const char *errorString= reinterpret_cast<const char*>(gluErrorString(error));
			throw runtime_error("OpenGL error: " + string(errorString) + " at file: "
					+ string(file) + ", line " + Conversion::toStr(line));
		}

	}

//	const PlatformContextGl *getPlatformContextGl() const {return &pcgl;}
};

}}} //end namespace

#if defined(NDEBUG) || defined(NO_GL_ASSERTIONS)
#	define assertGl() ((void) 0)
#else
#	define assertGl() ContextGl::_assertGl(__FILE__, __LINE__)
#endif

#endif
