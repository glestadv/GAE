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


#ifndef _SHARED_GRAPHICS_MODELRENDERER_H_
#define _SHARED_GRAPHICS_MODELRENDERER_H_

#include "model.h"
#include "opengl.h"

namespace Shared{ namespace Graphics{

class Texture;

// =====================================================
//	class MeshCallback
//
/// This gets called before rendering mesh
// =====================================================
/*
class MeshCallback{
public:
	virtual ~MeshCallback(){};
	virtual void execute(const Mesh *mesh)= 0;
};*/
// =====================================================
// 	class MeshCallbackTeamColor
// =====================================================

class MeshCallbackTeamColor/*: public MeshCallback*/{
private:
	const Texture *teamTexture;

public:
	void setTeamTexture(const Texture *teamTexture)	{this->teamTexture= teamTexture;}
	virtual void execute(const Mesh *mesh);
};

// =====================================================
//	class ModelRenderer
// =====================================================

class ModelRenderer {
private:
	bool renderNormals;
	bool renderTextures;
	bool renderColors;
	MeshCallbackTeamColor *meshCallback;

	bool rendering;
	bool duplicateTexCoords;
	int secondaryTexCoordUnit;
	GLuint lastTexture;

public:
//	ModelRenderer() : meshCallback(NULL) {}
	ModelRenderer();
	void begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallbackTeamColor *meshCallback=NULL);

	void end() {
		//assertions
		assert(rendering);
		assertGl();
	
		//set render state
		rendering = false;
	
		//pop
		glPopAttrib();
		glPopClientAttrib();
	
		//assertions
		assertGl();
	}
	
	void render(const Model *model) {
		//assertions
		assert(rendering);
		assertGl();
	
		//render every mesh
		for (uint32 i = 0; i < model->getMeshCount(); ++i) {
			renderMesh(model->getMesh(i));
		}
	
		//assertions
		assertGl();
	}
	
	void renderNormalsOnly(const Model *model) {
		//assertions
		assert(rendering);
		assertGl();
	
		//render every mesh
		for (uint32 i = 0; i < model->getMeshCount(); ++i) {
			renderMeshNormals(model->getMesh(i));
		}
	
		//assertions
		assertGl();
	}

	void setDuplicateTexCoords(bool duplicateTexCoords)			{this->duplicateTexCoords= duplicateTexCoords;}
	void setSecondaryTexCoordUnit(int secondaryTexCoordUnit)	{this->secondaryTexCoordUnit= secondaryTexCoordUnit;}

private:
	
	void renderMesh(const Mesh *mesh);
	void renderMeshNormals(const Mesh *mesh);
};

}}//end namespace

#endif
