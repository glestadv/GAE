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

#ifndef _SHARED_GRAPHICS_GRAPHICSFACTORY_H_
#define _SHARED_GRAPHICS_GRAPHICSFACTORY_H_

#include <cstdlib>
#include "types.h"

using namespace Shared::Platform;

namespace Shared { namespace Graphics {

class Context;

class TextureManager;
class Texture1D;
class Texture2D;
class Texture3D;
class TextureCube;

class ModelManager;
class ModelRenderer;
class Model;

class FontManager;
class TextRenderer2D;
class TextRenderer3D;
class Font2D;
class Font3D;

class ParticleManager;
class ParticleRenderer;

class ShaderManager;
class ShaderProgram;
class VertexShader;
class FragmentShader;

// =====================================================
//	class GraphicsFactory
// =====================================================

class GraphicsFactory {
public:
	virtual ~GraphicsFactory(){}

	//context
	//virtual Context *newContext(uint32 colorBits, uint32 depthBits, uint32 stencilBits) = 0;

	//textures
	virtual TextureManager *newTextureManager() = 0;
	virtual Texture1D *newTexture1D() = 0;
	virtual Texture2D *newTexture2D() = 0;
	virtual Texture3D *newTexture3D() = 0;
	virtual TextureCube *newTextureCube() = 0;

	//models
	virtual ModelManager *newModelManager() = 0;
	virtual ModelRenderer *newModelRenderer() = 0;
	virtual Model *newModel() = 0;

	//text
	virtual FontManager *newFontManager() = 0;
	virtual TextRenderer2D *newTextRenderer2D() = 0;
	virtual TextRenderer3D *newTextRenderer3D() = 0;
	virtual Font2D *newFont2D() = 0;
	virtual Font3D *newFont3D() = 0;

	//particles
	virtual ParticleManager *newParticleManager() = 0;
	virtual ParticleRenderer *newParticleRenderer() = 0;

	//shaders
	virtual ShaderManager *newShaderManager() = 0;
	virtual ShaderProgram *newShaderProgram() = 0;
	virtual VertexShader *newVertexShader() = 0;
	virtual FragmentShader *newFragmentShader() = 0;
};

}} // end namespace

#endif
