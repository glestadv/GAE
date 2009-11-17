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

#ifndef _SHARED_GRAPHICS_PARTICLERENDERER_H_
#define _SHARED_GRAPHICS_PARTICLERENDERER_H_

#include "particle.h"

namespace Shared{ namespace Graphics{

class ModelRenderer;

// =====================================================
//	class ParticleRenderer
// =====================================================

class ParticleRenderer{
public:
	static const int bufferSize = 1024;

private:
	bool rendering;
	Vec3f vertexBuffer[bufferSize];
	Vec2f texCoordBuffer[bufferSize];
	Vec4f colorBuffer[bufferSize];

public:
	//particles
	ParticleRenderer();
	void renderManager(ParticleManager *pm, ModelRenderer *mr);
	void renderSystem(ParticleSystem *ps);
	void renderSystemLine(ParticleSystem *ps);
	void renderSingleModel(AttackParticleSystem *ps, ModelRenderer *mr);

private:
	void renderBufferQuads(int quadCount);
	void renderBufferLines(int lineCount);
	void setBlendMode(Particle::BlendMode blendMode);
};

}}//end namespace

#endif
