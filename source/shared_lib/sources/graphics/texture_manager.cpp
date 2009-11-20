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
#include "texture_manager.h"

#include <cstdlib>

#include "graphics_interface.h"
#include "graphics_factory.h"
#include "util.h"

#include "leak_dumper.h"

namespace Shared { namespace Graphics {

// =====================================================
// class TextureManager
// =====================================================

TextureManager::TextureManager(GraphicsFactory &factory)
		: textures()
		, textureFilter(Texture::FILTER_BILINEAR)
		, maxAnisotropy(1)
		, factory(factory) {
}

TextureManager::~TextureManager() {
	end();
}

void TextureManager::init() {
	foreach(Textures::value_type &v, textures) {
		v->init(textureFilter, maxAnisotropy);
	}
}

void TextureManager::end() {
	foreach(Textures::value_type &v, textures) {
		v->end();
	}
	/*for(int i=0; i<textures.size(); ++i){
	 textures[i]->end();
	 delete textures[i];
	}*/
	textures.clear();
}

void TextureManager::setFilter(Texture::Filter textureFilter) {
	this->textureFilter = textureFilter;
}

void TextureManager::setMaxAnisotropy(int maxAnisotropy) {
	this->maxAnisotropy = maxAnisotropy;
}

Texture *TextureManager::getTexture(const string &path) {
	foreach(Textures::value_type &v, textures) {
		if (v->getPath() == path) {
			return v.get();
		}
	}

	return NULL;
}

Texture1D *TextureManager::newTexture1D() {
	Texture1D *t = factory.newTexture1D();
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}

Texture2D *TextureManager::newTexture2D() {
	Texture2D *t = factory.newTexture2D();
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}

Texture3D *TextureManager::newTexture3D() {
	Texture3D *t = factory.newTexture3D();
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}

TextureCube *TextureManager::newTextureCube() {
	TextureCube *t = factory.newTextureCube();
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}


/*
Texture1D *TextureManager::loadTexture1D(const XmlNode &node) {
	Texture1D *t = factory.loadTexture1D(node);
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}

Texture2D *TextureManager::loadTexture2D(const XmlNode &node) {
	Texture2D *t = factory.loadTexture2D(node);
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}

Texture3D *TextureManager::loadTexture3D(const XmlNode &node) {
	Texture3D *t = factory.loadTexture3D(node);
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}

TextureCube *TextureManager::loadTextureCube(const XmlNode &node) {
	TextureCube *t = factory.loadTextureCube(node);
	textures.push_back(shared_ptr<Texture>(t));

	return t;
}
*/

}}//end namespace
