// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_SIMD_H_
#define _SHARED_UTIL_SIMD_H_

// Header file for explicit simd support

#include <types.h>
#include <lang_features.h>

#if defined(USE_SSE2_INTRINSICS)
	// if we're using intrinsics, we have to align vectors
#	define ALIGN_12BYTE_VECTORS
#	warning Profiling data has shown this code to be slower than allowing GCC to perform its own \
			optimizations.  If you find it to be faster on your system, please submit this \
			information to the glest message board.
#endif

#if defined(ALIGN_12BYTE_VECTORS)
#	define ALIGN_VECTORS
#endif

// simd headers and vector types
#if (defined __i386__ || defined __x86_64__ || defined(_MSC_VER) )
#	if defined(USE_SSE2_INTRINSICS) || defined(ALIGN_VECTORS)
#		include <emmintrin.h>	// SSE2
		typedef __m128 vFloat;
#	else
		typedef float vFloat;
#	endif
#elif defined __ppc__ || defined __ppc64__
#	include <altivec.h>
	typedef vector float vFloat;
#else
	// no simd support.
	typedef float vFloat;
#endif


#ifdef ALIGN_VECTORS
#	define ALIGN_VEC_DECL __aligned_pre(16)
#	define ALIGN_VEC_ATTR __aligned_post(16)
#else
#	define ALIGN_VEC_DECL
#	define ALIGN_VEC_ATTR
#endif

// Rather or not to align 12-byte vectors.
#ifdef ALIGN_12BYTE_VECTORS
#	define ALIGN_VEC12_DECL __aligned_pre(16)
#	define ALIGN_VEC12_ATTR __aligned_post(16)
#else
#	define ALIGN_VEC12_DECL
#	define ALIGN_VEC12_ATTR
#endif

//#define ALIGN16(len) ((len + 15) & ~15)
namespace Shared { namespace Util {



}}//end namespace

#endif // _SHARED_UTIL_SIMD_H_
