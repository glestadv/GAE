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
#include "graphics_factory_gl.h"

#include "texture_manager.h"
#include "model_manager.h"
#include "particle.h"
#include "font_manager.h"
#include "text_renderer_gl.h"
#include "model_renderer_gl.h"
#include "particle_renderer_gl.h"
#include "context_gl.h"
#include "model_gl.h"
#include "texture_gl.h"
#include "font_gl.h"
#include "shader_gl.h"
#include "shader_manager.h"

#include "leak_dumper.h"

namespace Shared { namespace Graphics { namespace Gl {

//textures
TextureManager *GraphicsFactoryGl::newTextureManager()		{return new TextureManager(*this);}
Texture1D *GraphicsFactoryGl::newTexture1D()				{return new Texture1DGl(context);}
Texture2D *GraphicsFactoryGl::newTexture2D()				{return new Texture2DGl(context);}
Texture3D *GraphicsFactoryGl::newTexture3D()				{return new Texture3DGl(context);}
TextureCube *GraphicsFactoryGl::newTextureCube()			{return new TextureCubeGl(context);}

//models
ModelManager *GraphicsFactoryGl::newModelManager()			{return new ModelManager(*this);}
ModelRenderer *GraphicsFactoryGl::newModelRenderer()		{return new ModelRendererGl();}
Model *GraphicsFactoryGl::newModel()						{return new ModelGl();}

//text
FontManager *GraphicsFactoryGl::newFontManager()			{return new FontManager(*this);}
TextRenderer2D *GraphicsFactoryGl::newTextRenderer2D()		{return new TextRenderer2DGl();}
TextRenderer3D *GraphicsFactoryGl::newTextRenderer3D()		{return new TextRenderer3DGl();}
Font2D *GraphicsFactoryGl::newFont2D()						{return new Font2DGl();}
Font3D *GraphicsFactoryGl::newFont3D()						{return new Font3DGl();}

//particles
ParticleManager *GraphicsFactoryGl::newParticleManager()	{return new ParticleManager(*this);}
ParticleRenderer *GraphicsFactoryGl::newParticleRenderer()	{return new ParticleRendererGl();}

//shaders
ShaderManager *GraphicsFactoryGl::newShaderManager()		{return new ShaderManager(*this);}
ShaderProgram *GraphicsFactoryGl::newShaderProgram()		{return new ShaderProgramGl();}
VertexShader *GraphicsFactoryGl::newVertexShader()			{return new VertexShaderGl();}
FragmentShader *GraphicsFactoryGl::newFragmentShader()		{return new FragmentShaderGl();}

}}}//end namespace
