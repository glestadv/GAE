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

#ifndef _SHARED_GRAPHICS_INTERPOLATION_H_
#define _SHARED_GRAPHICS_INTERPOLATION_H_

#include "vec.h"
#include "model.h"

namespace Shared{ namespace Graphics{

void test_interpolate();

// =====================================================
//	class InterpolationData
// =====================================================

class InterpolationData {
private:
	Mesh *mesh;

	MeshVertexBlock data;

public:
	InterpolationData(Mesh *mesh);
	~InterpolationData();

	const MeshVertexBlock& getVertexBlock() const {
		return data;
	}
	
	void update(float t, bool cycle);
	//void updateVertices(float t, bool cycle);
	//void updateNormals(float t, bool cycle);
};

}}//end namespace

#endif
