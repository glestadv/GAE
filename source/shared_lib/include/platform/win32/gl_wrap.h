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

#ifndef _SHARED_PLATFORM_GLWRAP_H_
#define _SHARED_PLATFORM_GLWRAP_H_

#include <windows.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include <glprocs.h>

#include <string>
#include <cassert>

#include "font.h"
#include "types.h"


#define GLEST_GLPROC(X, Y) inline X( static a = wglGetProcAddress(a); return a;)

using std::string;

using Shared::Graphics::FontMetrics;

namespace Shared { namespace Platform {

// =====================================================
// class PlatformContextGl
// =====================================================

class PlatformContextGl {
protected:
	DeviceContextHandle dch;
	GlContextHandle glch;
	int m_colorBits;
	int m_depthBits;
	int m_stencilBits;

public:
	void init(int colorBits, int depthBits, int stencilBits);
	void end() {
		int makeCurrentError = wglDeleteContext(glch);
		assert(makeCurrentError);
	}

	void makeCurrent() {
		int makeCurrentError = wglMakeCurrent(dch, glch);
		assert(makeCurrentError);
	}

	void swapBuffers() {
		int swapErr = SwapBuffers(dch);
		assert(swapErr);
	}

	void changeWindow(WindowHandle wnd) {
		this->dch = GetDC(wnd);
		assert(dch!=NULL);
		setupDeviceContext();
		makeCurrent();
	}

	DeviceContextHandle getHandle() const {return dch;}

private:
	void setupDeviceContext();
};


// =====================================================
// Global Fcs
// =====================================================

inline PROC getGlProcAddress(const char *procName) {
	PROC proc = wglGetProcAddress(procName);
	assert(proc != NULL);
	return proc;
}

inline const char *getPlatformExtensions(const PlatformContextGl *pcgl) {
	typedef const char*(WINAPI * PROCTYPE)(HDC hdc);
	PROCTYPE proc = reinterpret_cast<PROCTYPE>(getGlProcAddress("wglGetExtensionsStringARB"));
	return proc == NULL ? "" : proc(pcgl->getHandle());
}

}}//end namespace

#endif
