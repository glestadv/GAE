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


#include "pch.h"
#include "context_gl.h"

#include <cassert>
#include <stdexcept>

#include "opengl.h"

#include "leak_dumper.h"

#ifdef USE_SDL
#elif defined(WIN32) || defined(WIN64)
#	define THROW_WINDOWS_EXCEPTION(msg, func) \
		WindowsException::coldThrow((msg), (func), NULL, __FILE__, __LINE__)
#	define THROW_IF(cond, msg, func) if((cond)) {THROW_WINDOWS_EXCEPTION(msg, func);}
#else
#	error what are we building on?
#endif


using namespace std;

namespace Shared { namespace Graphics { namespace Gl {

// =====================================================
// class ContextGl
// =====================================================

ContextGl::ContextGl(uint32 colorBits, uint32 depthBits, uint32 stencilBits)
		: colorBits(colorBits)
		, depthBits(depthBits)
		, stencilBits(stencilBits)
		, pcgl(colorBits, depthBits, stencilBits) {
#ifdef USE_SDL
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

#elif defined(WIN32) || defined(WIN64)
	int iFormat;
	PIXELFORMATDESCRIPTOR pfd;
	BOOL err;

	//Set8087CW($133F);
	dch = GetDC(GetActiveWindow());
	THROW_IF(!dch, "Failed to retrieve device context handle.", "GetDC");

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_GENERIC_ACCELERATED | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = colorBits;
	pfd.cDepthBits = depthBits;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cStencilBits = stencilBits;

	iFormat = ChoosePixelFormat(dch, &pfd);
	THROW_IF(!iFormat, "ChoosePixelFormat failed.", "ChoosePixelFormat");

	err = SetPixelFormat(dch, iFormat, &pfd);
	THROW_IF(!err, "SetPixelFormat failed.", "SetPixelFormat");

	glch = wglCreateContext(dch);
	THROW_IF(!glch, "Error initing OpenGL device context", "wglCreateContext");

	makeCurrent();
#endif
}

ContextGl::~ContextGl() {
}

void ContextGl::end() {
	pcgl.end();
}

void ContextGl::reset() {
}

void ContextGl::makeCurrent() {
	pcgl.makeCurrent();
}

void ContextGl::swapBuffers() {
	pcgl.swapBuffers();
}


bool ContextGl::isGlExtensionSupported(const char *extensionName) {
	const char *s;
	GLint len;
	const GLubyte *extensionStr = glGetString(GL_EXTENSIONS);

	s = reinterpret_cast<const char *>(extensionStr);
	len = strlen(extensionName);

	while ((s = strstr(s, extensionName)) != NULL) {
		s += len;

		if ((*s == ' ') || (*s == '\0')) {
			return true;
		}
	}

	return false;
}

bool ContextGl::isGlVersionSupported(int major, int minor, int release) {

	string stringVersion = getGlVersion();
	const char *strVersion = stringVersion.c_str();

	//major
	const char *majorTok = strVersion;
	int majorSupported = atoi(majorTok);

	if (majorSupported < major) {
		return false;
	} else if (majorSupported > major) {
		return true;
	}

	//minor
	int i = 0;
	while (strVersion[i] != '.') {
		++i;
	}
	const char *minorTok = &strVersion[i] + 1;
	int minorSupported = atoi(minorTok);

	if (minorSupported < minor) {
		return false;
	} else if (minorSupported > minor) {
		return true;
	}

	//release
	++i;
	while (strVersion[i] != '.') {
		++i;
	}
	const char *releaseTok = &strVersion[i] + 1;

	if (atoi(releaseTok) < release) {
		return false;
	}

	return true;
}

string ContextGl::getGlVersion() {
	return string(reinterpret_cast<const char *>(glGetString(GL_VERSION)));
}

string ContextGl::getGlRenderer() {
	return string(reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
}

string ContextGl::getGlVendor() {
	return string(reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
}

string ContextGl::getGlExtensions() {
	return string(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
}

string ContextGl::getGlPlatformExtensions() {
	return pcgl.getPlatformExtensions();
}

int ContextGl::getGlMaxLights() {
	int i;
	glGetIntegerv(GL_MAX_LIGHTS, &i);
	return i;
}

int ContextGl::getGlMaxTextureSize() {
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &i);
	return i;
}

int ContextGl::getGlMaxTextureUnits() {
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &i);
	return i;
}

int ContextGl::getGlModelviewMatrixStackDepth() {
	int i;
	glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &i);
	return i;
}

int ContextGl::getGlProjectionMatrixStackDepth() {
	int i;
	glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &i);
	return i;
}

void ContextGl::checkGlExtension(const char *extensionName) {
	if (!isGlExtensionSupported(extensionName)) {
		throw runtime_error("OpenGL extension not supported: " + string(extensionName));
	}
}

}}} // end namespace
