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

#include "context.h"
#include "model.h"
#include "model_manager.h"
#include "model_renderer.h"
#include "texture.h"
#include "font.h"
#include "font_manager.h"
#include "text_renderer.h"
#include "particle_renderer.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class GraphicsFactory
// =====================================================

class GraphicsFactory{
public:
	virtual ~GraphicsFactory(){}

	//context
	Context *newContext()					{return new Context();}

	//textures
	TextureManager *newTextureManager()		{return new TextureManager();}
	Texture1D *newTexture1D()				{return new Texture1D();}
	Texture2D *newTexture2D()				{return new Texture2D();}
	Texture3D *newTexture3D()				{return new Texture3D();}
	TextureCube *newTextureCube()			{return new TextureCube();}
	
	//models
	ModelManager *newModelManager()			{return new ModelManager();}
	ModelRenderer *newModelRenderer()		{return new ModelRenderer();}
	Model *newModel()						{return new Model();}

	//text
	FontManager *newFontManager()			{return new FontManager();}
	TextRenderer2D *newTextRenderer2D()		{return new TextRenderer2D();}
	TextRenderer3D *newTextRenderer3D()		{return new TextRenderer3D();}
	Font2D *newFont2D()						{return new Font2D();}
	Font3D *newFont3D()						{return new Font3D();}

	//particles
	ParticleManager *newParticleManager()	{return new ParticleManager();}
	ParticleRenderer *newParticleRenderer()	{return new ParticleRenderer();}
};

}}//end namespace

#endif
