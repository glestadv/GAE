//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.
#include "platform_util.h"

#include <iostream>
#include <sstream>
#include <cassert>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <SDL.h>

#include "util.h"
#include "conversion.h"
#include "leak_dumper.h"
#include "sdl_private.h"
#include "window.h"
#include "noimpl.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

namespace Private{

bool shouldBeFullscreen = false;
int ScreenWidth;
int ScreenHeight;

}

// =====================================
//          PerformanceTimer
// =====================================

// inlined

// =====================================
//         Chrono
// =====================================

Chrono::Chrono() {
	freq = 1000;
	stopped= true;
	accumCount= 0;
}


// =====================================
//         Misc
// =====================================

char *initDirIterator(const string &path, DirIterator &di) {
	string mypath = path;
	di.i = 0;

	/* Stupid win32 is searching for all files without extension when *. is
	 * specified as wildcard
	 */
	if(mypath.compare(mypath.size() - 2, 2, "*.") == 0) {
		mypath = mypath.substr(0, mypath.size() - 2);
		mypath += "*";
	}

	if(glob(mypath.c_str(), 0, 0, &di.globbuf) < 0) {
		throw runtime_error("Error searching for files '" + path + "': " + strerror(errno));
	}
	return di.globbuf.gl_pathc ? di.globbuf.gl_pathv[di.i] : NULL;
}

void mkdir(const string &path, bool ignoreDirExists) {
	if(::mkdir(path.c_str(), S_IRWXU | S_IROTH | S_IXOTH)) {
		int e = errno;
		if(!(ignoreDirExists && e == EEXIST)) {
			char buf[0x400];
			*buf = 0;
			strerror_r(e, buf, sizeof(buf));
			throw runtime_error("Failed to create directory " + path + ": " + buf);
		}
	}
}

size_t getFileSize(const string &path) {
	struct stat s;
	if(stat(path.c_str(), &s)) {
		char buf[0x400];
		*buf = 0;
		strerror_r(errno, buf, sizeof(buf));
		throw runtime_error("Failed to stat file " + path + ": " + buf);
	}
	return s.st_size;
}

bool changeVideoMode(int resW, int resH, int colorBits, int ) {
	Private::shouldBeFullscreen = true;
	return true;
}

void restoreVideoMode() {
}

void message(string message) {
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";
}

bool ask(string message) {
	std::cerr << "Confirmation: " << message << "\n";
	int res;
	std::cin >> res;
	return res != 0;
}

bool isKeyDown(int virtualKey) {
	char key = static_cast<char> (virtualKey);
	const Uint8* keystate = SDL_GetKeyState(0);

	// kinda hack and wrong...
	if(key >= 0) {
		return keystate[key];
	}
	switch(key) {
		case vkAdd:
			return keystate[SDLK_PLUS] | keystate[SDLK_KP_PLUS];
		case vkSubtract:
			return keystate[SDLK_MINUS] | keystate[SDLK_KP_MINUS];
		case vkAlt:
			return keystate[SDLK_LALT] | keystate[SDLK_RALT];
		case vkControl:
			return keystate[SDLK_LCTRL] | keystate[SDLK_RCTRL];
		case vkShift:
			return keystate[SDLK_LSHIFT] | keystate[SDLK_RSHIFT];
		case vkEscape:
			return keystate[SDLK_ESCAPE];
		case vkUp:
			return keystate[SDLK_UP];
		case vkLeft:
			return keystate[SDLK_LEFT];
		case vkRight:
			return keystate[SDLK_RIGHT];
		case vkDown:
			return keystate[SDLK_DOWN];
		case vkReturn:
			return keystate[SDLK_RETURN] | keystate[SDLK_KP_ENTER];
		case vkBack:
			return keystate[SDLK_BACKSPACE];
		default:
			std::cerr << "isKeyDown called with unknown key.\n";
			break;
	}
	return false;
}

}}//end namespace
