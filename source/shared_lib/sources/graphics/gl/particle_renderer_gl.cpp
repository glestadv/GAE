// ==============================================================
// This file is part of Glest Shared Library (www.glest.org)
//
// Copyright (C) 2001-2008 Martiño Figueroa
//
// You can redistribute this code and/or modify it under
// the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "particle_renderer_gl.h"

#include "opengl.h"
#include "texture_gl.h"
#include "model_renderer.h"
#include "math_util.h"

#include "leak_dumper.h"


namespace Shared { namespace Graphics { namespace Gl {

// =====================================================
// class ParticleRendererGl
// =====================================================

const GLenum ParticleRendererGl::glBlendFactors[BlendFactor::COUNT] = {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_CONSTANT_COLOR,
	GL_ONE_MINUS_CONSTANT_COLOR,
	GL_CONSTANT_ALPHA,
	GL_ONE_MINUS_CONSTANT_ALPHA,
	GL_SRC_ALPHA_SATURATE
};

const GLenum ParticleRendererGl::glBlendEquations[BlendMode::COUNT] = {
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX
};

// ===================== PUBLIC ========================

ParticleRendererGl::ParticleRendererGl() {
	assert(bufferSize % 4 == 0);

	rendering = false;

	// init texture coordinates for quads
	for (int i = 0; i < bufferSize; i += 4) {
		for (int j=0; j < MAX_PARTICLE_BUFFERS; ++j) {
			texCoordBuffer[j][i] = Vec2f(0.0f, 1.0f);
			texCoordBuffer[j][i+1] = Vec2f(0.0f, 0.0f);
			texCoordBuffer[j][i+2] = Vec2f(1.0f, 0.0f);
			texCoordBuffer[j][i+3] = Vec2f(1.0f, 1.0f);
		}
	}
}

void ParticleRendererGl::renderManager(ParticleManager *pm, ModelRenderer *mr) {

	//assertions
	assertGl();

	//push state
	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT  | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	//init state
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_STENCIL_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	//glEnable(GL_BLEND);

	//render
	rendering = true;
	pm->render(this, mr);
	rendering = false;

	//pop state
	glPopClientAttrib();
	glPopAttrib();

	//assertions
	assertGl();
}

GLMatrix buildRotationMatrix(float angle, Vec3f axis) {
	float sinTheta = sinf(angle);
	float cosTheta = cosf(angle);
	float oneMinusCosTheta = 1.f - cosTheta;
	GLMatrix result(true);
	result(0, 0) = cosTheta + axis.x * axis.x * oneMinusCosTheta;
	result(0, 1) = axis.x * axis.y * oneMinusCosTheta - axis.z * sinTheta;
	result(0, 2) = axis.x * axis.z * oneMinusCosTheta + axis.y * sinTheta;
	result(1, 0) = axis.x * axis.y * oneMinusCosTheta + axis.z * sinTheta;
	result(1, 1) = cosTheta + axis.y * axis.y * oneMinusCosTheta;
	result(1, 2) = axis.y * axis.z * oneMinusCosTheta - axis.x * sinTheta;
	result(2, 0) = axis.x * axis.z * oneMinusCosTheta - axis.y * sinTheta;
	result(2, 1) = axis.y * axis.z * oneMinusCosTheta + axis.x * sinTheta;
	result(2, 2) = cosTheta + axis.z * axis.z * oneMinusCosTheta;
	return result;
}

void ParticleRendererGl::renderSystem(ParticleSystem *ps) {
	assertGl();
	assert(rendering);

	Vec3f rightVector;
	Vec3f upVector;
	float modelview[16];

	//render particles
	setBlendFunc(ps->getSrcBlendFactor(), ps->getDestBlendFactor());
	setBlendEquation(ps->getBlendEquationMode());
	glEnable(GL_BLEND);

	// get the current modelview state
	glGetFloatv(GL_MODELVIEW_MATRIX , modelview);
	rightVector = Vec3f(modelview[0], modelview[4], modelview[8]);
	upVector = Vec3f(modelview[1], modelview[5], modelview[9]);

	Vec3f rotAxis = rightVector.cross(upVector);
	rotAxis.normalize();

	// set state
	memset(textures, 0, sizeof(int) * MAX_PARTICLE_BUFFERS);
	for (int i=0; i < ps->getNumTextures(); ++i) {
		textures[i] = static_cast<Texture2DGl*>(ps->getTexture(i));
	}
	if (ps->getNumTextures() == 1) {
		lastTexture = textures[0]->getHandle();
		glBindTexture(GL_TEXTURE_2D, textures[0]->getHandle());
	} else {
		lastTexture = 0;
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	//fill vertex buffer with billboards
	int bufferIndex[MAX_PARTICLE_BUFFERS] = { 0 };

	for (int i = 0; i < ps->getAliveParticleCount(); ++i) {
		const Particle *particle = ps->getParticle(i);
		int tex = particle->texture;
		float size = particle->getSize() * 0.5f;
		Vec3f pos = particle->getPos();
		Vec4f color = particle->getColor();
		if (particle->getAngle() != 0.f) {
			GLMatrix rotMat = buildRotationMatrix(particle->getAngle(), rotAxis);
			Vec3f myRightVec = rotMat * rightVector;
			Vec3f myUpVec = rotMat * upVector;
			vertexBuffer[tex][bufferIndex[tex]] = pos - (myRightVec - myUpVec) * size;
			vertexBuffer[tex][bufferIndex[tex]+1] = pos - (myRightVec + myUpVec) * size;
			vertexBuffer[tex][bufferIndex[tex]+2] = pos + (myRightVec - myUpVec) * size;
			vertexBuffer[tex][bufferIndex[tex]+3] = pos + (myRightVec + myUpVec) * size;
		} else {
			vertexBuffer[tex][bufferIndex[tex]] = pos - (rightVector - upVector) * size;
			vertexBuffer[tex][bufferIndex[tex]+1] = pos - (rightVector + upVector) * size;
			vertexBuffer[tex][bufferIndex[tex]+2] = pos + (rightVector - upVector) * size;
			vertexBuffer[tex][bufferIndex[tex]+3] = pos + (rightVector + upVector) * size;
		}
		colorBuffer[tex][bufferIndex[tex]] = color;
		colorBuffer[tex][bufferIndex[tex]+1] = color;
		colorBuffer[tex][bufferIndex[tex]+2] = color;
		colorBuffer[tex][bufferIndex[tex]+3] = color;

		bufferIndex[tex] += 4;

		if (bufferIndex[tex] >= bufferSize) {
			bufferIndex[tex] = 0;
			renderBufferQuads(bufferSize, tex);
		}
	}
	for (int i=0; i < MAX_PARTICLE_BUFFERS; ++i) {
		if (bufferIndex[i]) {
			renderBufferQuads(bufferIndex[i], i);
		}
	}
	assertGl();
}

void ParticleRendererGl::renderSystemLine(ParticleSystem *ps) {

	assertGl();
	assert(rendering);

	if (ps->anyParticle()) {
		const Particle *particle = ps->getParticle(0);

		setBlendFunc(ps->getSrcBlendFactor(), ps->getDestBlendFactor());
		setBlendEquation(ps->getBlendEquationMode());
		glEnable(GL_BLEND);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		//fill vertex buffer with lines
		int bufferIndex = 0;
		glLineWidth(particle->getSize());

		for (int i = 0; i < ps->getAliveParticleCount(); ++i) {
			particle = ps->getParticle(i);

			vertexBuffer[0][bufferIndex] = particle->getPos();
			vertexBuffer[0][bufferIndex + 1] = particle->getLastPos();

			colorBuffer[0][bufferIndex] = particle->getColor();
			colorBuffer[0][bufferIndex + 1] = particle->getColor2();

			bufferIndex += 2;

			if (bufferIndex >= bufferSize) {
				bufferIndex = 0;
				renderBufferLines(bufferSize);
			}
		}
		renderBufferLines(bufferIndex);
	}

	assertGl();
}

void ParticleRendererGl::renderSingleModel(ParticleSystem *ps, ModelRenderer *mr, bool fog) {
	//render model
	if (ps->getModel() != NULL) {

		//init
		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.f, 1.f, 1.f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		//translate
		Vec3f pos = ps->getPos();
		glTranslatef(pos.x, pos.y, pos.z);

		//rotate
		Vec3f direction = ps->getDirection();
		Vec3f flatDirection = Vec3f(direction.x, 0.f, direction.z);
		Vec3f rotVector = Vec3f(0.f, 1.f, 0.f).cross(flatDirection);

		float angleV = radToDeg(atan2(flatDirection.length(), direction.y)) - 90.f;
		glRotatef(angleV, rotVector.x, rotVector.y, rotVector.z);
		float angleH = radToDeg(atan2(direction.x, direction.z));
		glRotatef(angleH, 0.f, 1.f, 0.f);

		//render
		mr->begin(RenderMode::OBJECTS, fog);
		mr->render(ps->getModel());
		mr->end();

		//end
		glPopMatrix();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
	}
}


// ============== PRIVATE =====================================

void ParticleRendererGl::renderBufferQuads(int quadCount, int buffer) {
	if (lastTexture != textures[buffer]->getHandle()) {
		lastTexture = textures[buffer]->getHandle();
		glBindTexture(GL_TEXTURE_2D, lastTexture);
	}
	glVertexPointer(3, GL_FLOAT, 0, vertexBuffer[buffer]);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoordBuffer[buffer]);
	glColorPointer(4, GL_FLOAT, 0, colorBuffer[buffer]);

	glDrawArrays(GL_QUADS, 0, quadCount);
}

void ParticleRendererGl::renderBufferLines(int lineCount) {
	glVertexPointer(3, GL_FLOAT, 0, vertexBuffer[0]);
	glColorPointer(4, GL_FLOAT, 0, colorBuffer[0]);

	glDrawArrays(GL_LINES, 0, lineCount);
}

}}} //end namespace
