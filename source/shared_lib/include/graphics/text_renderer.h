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

#ifndef _SHARED_GRAPHICS_TEXTRENDERER_H_
#define _SHARED_GRAPHICS_TEXTRENDERER_H_

#include <string>

#include "vec.h"
#include "font.h"

using std::string;

namespace Shared{ namespace Graphics{

// =====================================================
//	class TextRenderer2D
// =====================================================

class TextRenderer2D{
private:
	const Font2D *font;
	bool rendering;

public:
	TextRenderer2D();

	void begin(const Font2D *font);
	void render(const string &text, int x, int y, bool centered = false);
	void end();
};

// =====================================================
//	class TextRenderer3D
// =====================================================

class TextRenderer3D{
private:
	const Font3D *font;
	bool rendering;

public:
	TextRenderer3D();

	void begin(const Font3D *font);
	void render(const string &text, float x, float y, float size, bool centered);
	void end();
};

}}//end namespace

#endif
