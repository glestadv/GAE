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

#ifndef _SHARED_GRAPHICS_GL_GRAPHICSFACTORYGL_H_
#define _SHARED_GRAPHICS_GL_GRAPHICSFACTORYGL_H_

#include "graphics_factory.h"


namespace Shared { namespace Graphics { namespace Gl {

class ContextGl;

// =====================================================
//	class GraphicsFactoryGl
// =====================================================

class GraphicsFactoryGl : public GraphicsFactory {
	ContextGl &context;

public:
	GraphicsFactoryGl(ContextGl &context) : context(context) { }
	~GraphicsFactoryGl() { }

	ContextGl &getContext() {return context;}

	//textures
	virtual TextureManager *newTextureManager();
	virtual Texture1D *newTexture1D();
	virtual Texture2D *newTexture2D();
	virtual Texture3D *newTexture3D();
	virtual TextureCube *newTextureCube();

	//models
	virtual ModelManager *newModelManager();
	virtual ModelRenderer *newModelRenderer();
	virtual Model *newModel();

	//text
	virtual FontManager *newFontManager();
	virtual TextRenderer2D *newTextRenderer2D();
	virtual TextRenderer3D *newTextRenderer3D();
	virtual Font2D *newFont2D();
	virtual Font3D *newFont3D();

	//particles
	virtual ParticleManager *newParticleManager();
	virtual ParticleRenderer *newParticleRenderer();

	//shaders
	virtual ShaderManager *newShaderManager();
	virtual ShaderProgram *newShaderProgram();
	virtual VertexShader *newVertexShader();
	virtual FragmentShader *newFragmentShader();
};

}}}//end namespace

#endif
