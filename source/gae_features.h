// ==============================================================
//	This file is part of the Glest Advanced Engine
//	http://glest.org/glest_board/index.php?board=15.0
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef USE_PTHREAD
#	define USE_PTHREAD 0
#endif

#ifndef USE_SDL
#	define USE_SDL 0
#endif

#ifndef USE_OPENAL
#	define USE_OPENAL 0
#endif

#ifndef USE_DS8
#	define USE_DS8 0
#endif

#ifndef USE_POSIX_SOCKETS
#	define USE_POSIX_SOCKETS 0
#endif

#ifndef SL_LEAK_DUMP
#	define SL_LEAK_DUMP 0
#endif

#ifndef USE_SSE_INTRINSICS
#	define USE_SSE_INTRINSICS 0
#endif

#ifndef ALIGN_12BYTE_VECTORS
#	define ALIGN_12BYTE_VECTORS USE_SSE_INTRINSICS
#endif

#ifndef ALIGN_16BYTE_VECTORS
#	define ALIGN_16BYTE_VECTORS USE_SSE_INTRINSICS
#endif

#ifndef DEBUG_NETWORK
#	define DEBUG_NETWORK 0
#endif

#ifndef DEBUG_NETWORK_DELAY
#	define DEBUG_NETWORK_DELAY 0
#endif

#ifndef DEBUG_NETWORK_DELAY_VAR
#	define DEBUG_NETWORK_DELAY_VAR 0
#endif

#ifndef DEBUG_WORLD
#	define DEBUG_WORLD 0
#endif

#ifndef DEBUG_PATHFINDER
#	define DEBUG_PATHFINDER 0
#endif

#ifndef DEBUG_TEXTURES
#	define DEBUG_TEXTURES 0
#endif


#ifndef USE_xxxx
#	define USE_xxxx 0
#endif



// Sanity checks
#if defined(DEBUG) && defined(NDEBUG)
#	error "Don't set both DEBUG and NDEBUG in the same build please."
#endif

#if DEBUG_NETWORK_DELAY && !DEBUG_NETWORK
#	error DEBUG_NETWORK_DELAY requires DEBUG_NETWORK to be set
#endif

#if USE_SSE_INTRINSICS && !(ALIGN_12BYTE_VECTORS && ALIGN_16BYTE_VECTORS)
#	error if USE_SSE_INTRINSICS is set, then ALIGN_12BYTE_VECTORS and ALIGN_16BYTE_VECTORS must both be set.
#endif
