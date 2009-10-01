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

#ifndef _SHARED_GRAPHICS_GL_MODELRENDERERGL_H_
#define _SHARED_GRAPHICS_GL_MODELRENDERERGL_H_

#include "model_renderer.h"
#include "model.h"
#include "opengl.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class ModelRendererGl
// =====================================================

class ModelRendererGl: public ModelRenderer {
private:
	bool rendering;
	bool duplicateTexCoords;
	int secondaryTexCoordUnit;
	GLuint lastTexture;

public:
	ModelRendererGl();
	virtual void begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback);

	virtual void end() {
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
	
	virtual void render(const Model *model) {
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

	virtual void renderMesh( const Model *model, int mesh ) {
		renderMesh( model->getMesh( mesh ) );
	}
	
	virtual void renderNormalsOnly(const Model *model) {
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
	//virtual void end();
	//virtual void render(const Model *model);
	//virtual void renderNormalsOnly(const Model *model);

	void setDuplicateTexCoords(bool duplicateTexCoords)			{this->duplicateTexCoords= duplicateTexCoords;}
	void setSecondaryTexCoordUnit(int secondaryTexCoordUnit)	{this->secondaryTexCoordUnit= secondaryTexCoordUnit;}

private:
	
	void renderMesh(const Mesh *mesh);
	void renderMeshNormals(const Mesh *mesh);
};

}}}//end namespace

#endif
