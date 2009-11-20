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
using Shared::Graphics::Context;

namespace Shared { namespace Graphics { namespace Gl {



// =====================================================
// class ContextGl
// =====================================================

class ContextGl : public Context {
private:
	bool fullScreen;
 	size_t width;
	size_t height;
	float freq;
	size_t colorBits;
	size_t depthBits;
	size_t stencilBits;

public:
	ContextGl(bool fullScreen, size_t width, size_t height, float freq, size_t colorBits, size_t depthBits, size_t stencilBits);
	~ContextGl();

	bool isFullScreen() const		{return fullScreen;}
 	size_t getWidth() const			{return width;}
	size_t getHeight() const		{return height;}
	float getFreq() const			{return freq;}
	size_t getColorBits() const		{return colorBits;}
	size_t getDepthBits() const		{return depthBits;}
	size_t getStencilBits() const	{return stencilBits;}

#if 0
	void setColorBits(size_t v)		{colorBits = v;}
	void setDepthBits(size_t v)		{depthBits = v;}
	void setStencilBits(size_t v)	{stencilBits = v;}
#endif

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
#	define assertGl() ::Shared::Graphics::Gl::ContextGl::_assertGl(__FILE__, __LINE__)
#endif

#endif
