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

#ifndef _SHARED_PLATFORM_WINDOWGL_H_
#define _SHARED_PLATFORM_WINDOWGL_H_

#include "context_gl.h"
#include "window.h"

using Shared::Graphics::Gl::ContextGl;

namespace Shared { namespace Platform {

// =====================================================
//	class WindowGl
// =====================================================

class WindowGl : public Window {
protected:
	ContextGl context;

public:
	WindowGl(WindowStyle windowStyle, const string &text)
	setStyle(windowStyle);
	setText(text);

	void initGl(int colorBits, int depthBits, int stencilBits);
	void makeCurrentGl();
	void swapBuffersGl();
	static void setDisplaySettings(int width, int height, int colorBits, int freq) {

		Config &config = theConfig;
		// bool multisamplingSupported = isGlExtensionSupported("WGL_ARB_multisample");

		if (!config.getDisplayWindowed()) {

			int freq = config.getDisplayRefreshFrequency();
			int colorBits = config.getRenderColorBits();
			int screenWidth = config.getDisplayWidth();
			int screenHeight = config.getDisplayHeight();

			if (!(changeVideoMode(screenWidth, screenHeight, colorBits, freq)
					|| changeVideoMode(screenWidth, screenHeight, colorBits, 0))) {
				throw runtime_error("Error setting video mode: " + Conversion::toStr(screenWidth)
						+ "x" + Conversion::toStr(screenHeight) + "x" + Conversion::toStr(colorBits));
			}
		}
	}

};

}}//end namespace

#endif
