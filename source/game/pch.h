// ==============================================================
//	This file is part of Glest Advanced Engine (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_PCH_H_
#define _GAME_PCH_H_

#include "lang_features.h"
#include "projectConfig.h"

// some local headers of importance
#include "types.h"
#include "game_constants.h"

#ifndef USE_PCH
#	define USE_PCH 0
#endif

#define INVARIANT(condition, message) assert((condition) && message)
#ifdef WIN32
#	define CHECK_HEAP() assert(_CrtCheckMemory())
#else
#	define CHECK_HEAP()
#endif

#if USE_PCH

#if defined(WIN32)
#	if defined (USE_POSIX_SOCKETS)
#		error USE_POSIX_SOCKETS is not compatible with WIN32 or WIN64
#	endif
#	if defined (USE_SDL)
#		error USE_SDL is not compatible with WIN32 or WIN64
#	endif
#	if _GAE_USE_XAUDIO2_
#		include <xaudio2.h>
#	endif
#	define NOMINMAX // see http://support.microsoft.com/kb/143208
#	include <windows.h>
#	include <crtdbg.h>
#	include <io.h>
#else
#	include <unistd.h>
#	include <signal.h>
#	if !defined(USE_SDL)
#		error not WIN32 and USE_SDL not defined
#	endif
#	if !defined(USE_POSIX_SOCKETS)
#		error not WIN32 || WIN64 and USE_POSIX_SOCKETS not defined
#	endif
#endif

// POSIX base
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>

// lib c
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cassert>

// lib c++
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>

// stl
#include <algorithm>
#include <functional>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <deque>
#include <set>


#include <sys/types.h>

#ifdef USE_POSIX_SOCKETS

	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <fcntl.h>
//	#include <sys/filio.h>
	#include <sys/ioctl.h>

#endif

#ifdef USE_SDL
	#include <SDL.h>
	#include <SDL_opengl.h>
	#include <SDL_thread.h>
	#include <SDL_mutex.h>
	#include <glob.h>
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <sys/stat.h>
#endif

// zlib
#include <zlib.h>

// opengl
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#if !defined(WIN32)
	#include <GL/glx.h>
#endif


// vorbis
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#if defined(WIN32)
//	#include <glprocs.h>
	#include <winsock.h>
	#include <dsound.h>
#endif

#endif // USE_PCH

#endif // _GAME_PCH_H_
