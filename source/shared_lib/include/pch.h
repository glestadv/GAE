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

#ifndef _SHARED_PCH_H_
#define _SHARED_PCH_H_

#ifdef USE_PCH

#if defined(WIN32) || defined(WIN64)

	// sanity checks
#	if defined (USE_POSIX_SOCKETS)
#		error USE_POSIX_SOCKETS is not compatible with WIN32 or WIN64
#	endif

#	if defined (USE_SDL)
#		error USE_SDL is not compatible with WIN32 or WIN64
#	endif

#	include <windows.h>
#else
#	include <unistd.h>
#	include <signal.h>
#endif

// some local headers of importance
#include "types.h"

// POSIX base
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>

// boost
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

// c++ standard library
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// will this fly on windoze?
#include <sys/types.h>

#ifdef USE_POSIX_SOCKETS
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <fcntl.h>
#	include <sys/ioctl.h>
#endif

#ifdef USE_SDL
#	include <SDL.h>
#	define GL_GLEXT_PROTOTYPES
#	include <SDL_opengl.h>
#	include <SDL_thread.h>
#	include <SDL_mutex.h>
#	include <execinfo.h>
#	include <glob.h>
#	include <AL/al.h>
#	include <AL/alc.h>
#	include <sys/stat.h>
#endif

// zlib
#include <zlib.h>

// opengl
#include <GL/gl.h>
#include <GL/glu.h>
#if !(defined(WIN32) || defined(WIN64))
	#include <GL/glx.h>
#endif

// vorbis
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// xerces-c
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesDefs.hpp>

#if defined(WIN32) || defined(WIN64)
#	include <glprocs.h>
#	include <winsock.h>
#	include <dsound.h>
#	include <io.h>
#endif

// Glest Shared Library headers
#include "graphics/buffer.h"
#include "graphics/camera.h"
#include "graphics/context.h"
#include "graphics/entity.h"
#include "graphics/font.h"
#include "graphics/font_manager.h"
#include "graphics/gl/context_gl.h"
#include "graphics/gl/font_gl.h"
#include "graphics/gl/graphics_factory_basic_gl.h"
#include "graphics/gl/graphics_factory_gl.h"
#include "graphics/gl/graphics_factory_gl2.h"
#include "graphics/gl/model_gl.h"
#include "graphics/gl/model_renderer_gl.h"
#include "graphics/gl/opengl.h"
#include "graphics/gl/particle_renderer_gl.h"
#include "graphics/gl/shader_gl.h"
#include "graphics/gl/text_renderer_gl.h"
#include "graphics/gl/texture_gl.h"
#include "graphics/gl2/graphics_factory_gl2.h"
#include "graphics/gl2/shader_gl.h"
#include "graphics/graphics_factory.h"
#include "graphics/graphics_interface.h"
#include "graphics/interpolation.h"
#include "graphics/math_util.h"
#include "graphics/matrix.h"
#include "graphics/model.h"
#include "graphics/model_header.h"
#include "graphics/model_manager.h"
#include "graphics/model_renderer.h"
#include "graphics/particle.h"
#include "graphics/particle_renderer.h"
#include "graphics/pixmap.h"
#include "graphics/quaternion.h"
#include "graphics/shader.h"
#include "graphics/shader_manager.h"
#include "graphics/text_renderer.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"
#include "graphics/vec.h"
#include "lang_features.h"
#include "physics/weather.h"
#include "platform/input.h"
#include "platform/net_util.h"
#include "platform/platform_exception.h"
#include "platform/platform_util.h"
#include "platform/simd.h"
#include "platform/socket.h"
#include "platform/thread.h"
#include "platform/timer.h"
#include "platform/types.h"
#include "platform/window.h"
#include "platform/window_gl.h"
#include "sound/sound.h"
#include "sound/sound_factory.h"
#include "sound/sound_file_loader.h"
#include "sound/sound_interface.h"
#include "sound/sound_player.h"
#include "util/checksum.h"
#include "util/conversion.h"
#include "util/exception_base.h"
#include "util/factory.h"
#include "util/leak_dumper.h"
#include "util/patterns.h"
#include "util/profiler.h"
#include "util/properties.h"
#include "util/random.h"
#include "util/simple_data_buffer.h"
#include "util/util.h"
#include "xml/xml_parser.h"

#if defined(WIN32) || defined(WIN64)
#	include "platform/win32/factory_repository.h"
#	include "platform/win32/gl_wrap.h"
#	include "platform/win32/platform_definitions.h"
#	include "platform/win32/platform_main.h"
#	include "platform/win32/platform_menu.h"
#	include "sound/ds8/sound_factory_ds8.h"
#	include "sound/ds8/sound_player_ds8.h"
#else
#	include "platform/sdl/factory_repository.h"
#	include "platform/sdl/gl_wrap.h"
#	include "platform/sdl/noimpl.h"
#	include "platform/sdl/platform_main.h"
#	include "platform/sdl/sdl_private.h"
#	include "sound/openal/sound_factory_openal.h"
#	include "sound/openal/sound_player_openal.h"
#endif

#endif // USE_PCH
#endif // _SHARED_PCH_H_


