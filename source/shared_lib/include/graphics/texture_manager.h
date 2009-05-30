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

#ifndef _SHARED_GRAPHICS_TEXTUREMANAGER_H_
#define _SHARED_GRAPHICS_TEXTUREMANAGER_H_

#include <map>
#include <string>

#include "texture.h"
#include "util.h"

using std::map;
using std::string;

namespace Shared { namespace Graphics {

class GraphicsFactory; 

// =====================================================
// class TextureManager
// =====================================================

//manages textures, creation on request and deletion on destruction
class TextureManager {
private:
//	typedef map<string, shared_ptr<Texture> > Textures;
	typedef vector<shared_ptr<Texture> > Textures;

private:
	Textures textures;
	Texture::Filter textureFilter;
	int maxAnisotropy;
	GraphicsFactory &factory;

public:
	TextureManager();
	~TextureManager();
	void init();
	void end();

	void setFilter(Texture::Filter textureFilter);
	void setMaxAnisotropy(int maxAnisotropy);

//	Texture *load(const XmlNode &node);
	Texture *getTexture(const string &path);
	Texture1D *newTexture1D();
	Texture2D *newTexture2D();
	Texture3D *newTexture3D();
	TextureCube *newTextureCube();
	Texture1D *loadTexture1D(const XmlNode &node);
	Texture2D *loadTexture2D(const XmlNode &node);
	Texture3D *loadTexture3D(const XmlNode &node);
	TextureCube *loadTextureCube(const XmlNode &node);
};


}}//end namespace

#endif
