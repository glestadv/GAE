// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

/**
 * @file
 * Contains macros and other language-feature specific declarations, includes, etc.  The main
 * purpose of this header is to encapsulate all code who's sole purpose is managing language
 * features.
 */

#ifndef _SHARED_LANGFEATURES_H_
#define _SHARED_LANGFEATURES_H_

// use newer C++0x features of available
#if __cplusplus > 199711L
#	define DELETE_FUNC = delete
#else
#	define DELETE_FUNC
#endif

#endif // _SHARED_LANGFEATURES_H_
