// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "vec.h"
#include "fixed.h"

namespace Shared { namespace Math {


ostream& operator<<(ostream &lhs, const Vec3f &pt) {
	return lhs << "(" << pt.x << ", " << pt.y << ", " << pt.z << ")";
}

ostream& operator<<(ostream &lhs, const Vec2i &pt) {
	return lhs << "(" << pt.x << ", " << pt.y << ")";
}

ostream& operator<<(ostream &lhs, const Vec4i &rhs) {
	return lhs << "(" << rhs.x << ", " << rhs.y << ", " << rhs.z << ", " << rhs.w << ")";
}

ostream& operator<<(ostream &lhs, const Vec2f &rhs) {
	return lhs << "(" << rhs.x << ", " << rhs.y << ")";
}

/// fixed point square root, adapted from code at c.snippets.org
/// @todo make faster? [test perfomance vs sqrtf(), if horrible, make faster.]
/// avennues of optimization : unroll loop (compiler might be doing this for us), use set masks
/// for each iteration, rather than generating them in the macro (uses 'n', can't be replaced by compiler).
fixed fixed::sqRt() const {
	fixed root = 0;		/* accumulator      */
	uint32 r = 0;		/* remainder        */
	uint32 e = 0;		/* trial product    */

#	define GET2BITS(x, n) ((x & (3L << (DATUM_BITS - 2 * (n + 1)))) >> (DATUM_BITS - 2 * (n + 1)))
	for (int i = 0; i < DATUM_BITS / 2; ++i) {
		r = (r << 2) + GET2BITS(datum, i);
		root.datum <<= 1;
		e = (root.datum << 1) + 1;
		if (r >= e) {
			r -= e;
			++root.datum;
		}
	}
	root.datum <<= HALF_SHIFT;
	return root;
}

}}

