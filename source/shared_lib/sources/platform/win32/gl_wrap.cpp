// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "gl_wrap.h"

#include <cassert>
#include <windows.h>

#include "opengl.h"
#include "platform_exception.h"

#include "leak_dumper.h"

#define THROW_WINDOWS_EXCEPTION(msg, func) \
	WindowsException::coldThrow((msg), (func), NULL, __FILE__, __LINE__)

#define THROW_IF(cond, msg, func) if((cond)) {THROW_WINDOWS_EXCEPTION(msg, func);}

using namespace Shared::Graphics::Gl;

namespace Shared { namespace Platform {

// =====================================================
// class PlatformContextGl
// =====================================================

void PlatformContextGl::PlatformContextGl(int colorBits, int depthBits, int stencilBits) {

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
}

void PlatformContextGl::end() {
	THROW_IF(!wglDeleteContext(glch), "Failed to delete OpenGL context.", "wglDeleteContext");
}

void PlatformContextGl::makeCurrent() {
	THROW_IF(!wglMakeCurrent(dch, glch), "Failed to set the OpenGL context.", "wglMakeCurrent");
}

void PlatformContextGl::swapBuffers() {
	THROW_IF(!SwapBuffers(dch), "Failed swap frame buffers.", "SwapBuffers");
}

// ======================================
// Global Fcs
// ======================================

void PlatformContextGl::createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
		int charCount,, FontMetrics &metrics) {
	HFONT font = CreateFont(
			size, 0, 0, 0, width, 0, FALSE, FALSE, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
			DEFAULT_PITCH, type.c_str());

	THROW_IF(!font, "Failed to create font.", "CreateFont");

	HDC dc = wglGetCurrentDC();
	SelectObject(dc, font);
	BOOL err = wglUseFontBitmaps(dc, 0, charCount, base);
	THROW_IF(!err, "wglUseFontBitmaps failed.", "wglUseFontBitmaps");

	FIXED one;
	one.value = 1;
	one.fract = 0;

	FIXED zero;
	zero.value = 0;
	zero.fract = 0;

	MAT2 mat2;
	mat2.eM11 = one;
	mat2.eM12 = zero;
	mat2.eM21 = zero;
	mat2.eM22 = one;

	//metrics
	GLYPHMETRICS glyphMetrics;
	int errorCode = GetGlyphOutline(dc, 'a', GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);
	if (errorCode != GDI_ERROR) {
		metrics.setHeight(static_cast<float>(glyphMetrics.gmBlackBoxY));
	}
	for (int i = 0; i < charCount; ++i) {
		int errorCode = GetGlyphOutline(dc, i, GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);
		if (errorCode != GDI_ERROR) {
			metrics.setWidth(i, static_cast<float>(glyphMetrics.gmCellIncX));
		}
	}

	DeleteObject(font);
}

void PlatformContextGl::createGlFontOutlines(uint32 &base, const string &type, int width,
		float depth, int charCount, FontMetrics &metrics) {
	HFONT font = CreateFont(
			10, 0, 0, 0, width, 0, FALSE, FALSE, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
			DEFAULT_PITCH, type.c_str());

	THROW_IF(!font, "Failed to create font.", "CreateFont");

	GLYPHMETRICSFLOAT *glyphMetrics = new GLYPHMETRICSFLOAT[charCount];

	HDC dc = wglGetCurrentDC();
	SelectObject(dc, font);
	BOOL err = wglUseFontOutlines(dc, 0, charCount, base, 1000, depth, WGL_FONT_POLYGONS, glyphMetrics);
	THROW_IF(!err, "wglUseFontOutlines failed.", "wglUseFontOutlines");

	//load metrics
	metrics.setHeight(glyphMetrics['a'].gmfBlackBoxY);
	for (int i = 0; i < charCount; ++i) {
		metrics.setWidth(i, glyphMetrics[i].gmfCellIncX);
	}

	DeleteObject(font);
	delete [] glyphMetrics;
}

PROC PlatformContextGl::getGlProcAddress(const char *procName) {
	PROC proc = wglGetProcAddress(procName);
	if(!proc) {
		string msg = string() + "wglGetProcAddress(\"" + procName + "\") failed";
		THROW_WINDOWS_EXCEPTION(msg, "wglGetProcAddress";
	}
	assert(proc);
	return proc;
}

string PlatformContextGl::getPlatformExtensions() const {
	typedef const char*(WINAPI * PROCTYPE)(HDC hdc);
	PROCTYPE proc = reinterpret_cast<PROCTYPE>(getGlProcAddress("wglGetExtensionsStringARB"));
	return proc == NULL ? "" : proc(getHandle());
}

}}//end namespace
