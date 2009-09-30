// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "lang.h"

#include <stdarg.h>

#include "leak_dumper.h"


using namespace std;

namespace Game {

// =====================================================
//  class Lang
// =====================================================

string Lang::format(const string &s, ...) const {
	va_list ap;
	const string &fmt = get(s);
	size_t bufsize = fmt.size() + 2048;
	char *buf = new char[bufsize];

	va_start(ap, s);
	vsnprintf(buf, bufsize, fmt.c_str(), ap);
	va_end(ap);

	string ret(buf);
	delete[] buf;
	return ret;
}

} // end namespace
