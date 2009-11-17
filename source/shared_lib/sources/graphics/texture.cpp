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

#include "pch.h"
#include "texture.h"

#include "leak_dumper.h"

namespace Shared { namespace Graphics {

// =====================================================
// class Texture
// =====================================================

Texture::Texture()
		: path()
		, mipmap(true)
		, wrapMode(WRAP_MODE_REPEAT)
		, pixmapInit(true)
		, format(FORMAT_AUTO)
		, inited(false) {
}

Texture::~Texture() {}

// =====================================================
// class Texture1D
// =====================================================

//Texture1D::~Texture1D() {}

void Texture1D::load(const string &path) {
	this->path = path;
	pixmap.load(path);
}

// =====================================================
// class Texture2D
// =====================================================

//Texture2D::~Texture2D() {}

void Texture2D::load(const string &path) {
	this->path = path;
	pixmap.load(path);
}

// =====================================================
// class Texture3D
// =====================================================

//Texture3D::~Texture3D() {}

void Texture3D::loadSlice(const string &path, int slice) {
	this->path = path;
	pixmap.loadSlice(path, slice);
}

// =====================================================
// class TextureCube
// =====================================================

//TextureCube::~TextureCube() {}

void TextureCube::loadFace(const string &path, int face) {
	this->path = path;
	pixmap.loadFace(path, face);
}

}}//end namespace
