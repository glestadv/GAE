// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <clocale>

#include "lang.h"
#include "logger.h"
#include "util.h"

#include "leak_dumper.h"


using namespace std;
using namespace Shared::Util;

namespace Game {

// =====================================================
//  class Lang
// =====================================================

void Lang::setLocale(const string &locale) {
	this->locale = locale;
	strings.clear();
	setlocale(LC_CTYPE, locale.c_str());
	string path = "gae/data/lang/" + locale + ".lng";
	strings.load(path);
}

void Lang::loadScenarioStrings(const string &scenarioDir, const string &scenarioName) {
	string path = scenarioDir + "/" + scenarioName + "_" + locale + ".lng";

	scenarioStrings.clear();

	//try to load the current locale first
	if (fileExists(path)) {
		scenarioStrings.load(path);
	} else {
		//try english otherwise
		string path = scenarioDir + "/" + scenarioName + "/" + scenarioName + "_en.lng";
		if (fileExists(path)) {
			scenarioStrings.load(path);
		}
	}
}

string Lang::get(const string &s) const {
	const string *ret = strings.getStringOrNull(s);
	return ret ? *ret : string("???" + s + "???");
}

string Lang::getScenarioString(const string &s) const {
	const string *ret = scenarioStrings.getStringOrNull(s);
	return ret ? *ret : string("???" + s + "???");
}

string Lang::format(const string &s, ...) const {
	va_list ap;
	const string fmt = get(s);
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
