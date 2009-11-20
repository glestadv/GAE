// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//					   2005 Matthias Braun <matze@braunis.de>
//					   2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "factory_repository.h"

#include "leak_dumper.h"

namespace Shared { namespace Platform {

// =====================================================
// class FactoryRepository
// =====================================================

GraphicsFactory *FactoryRepository::getGraphicsFactory(const string &name) {
	if(name == "OpenGL"){
		return &graphicsFactoryGl;
	}
	else if(name == "OpenGL2"){
		return &graphicsFactoryGl2;
	}

	throw runtime_error("Unknown graphics factory: " + name);
}

SoundFactory *FactoryRepository::getSoundFactory(const string &name) {
#if USE_OPENAL
	if (name == "OpenAL") {
		return &soundFactoryOpenAL;
	}
#endif

#if USE_DS8
	if (name == "DirectSound8") {
		return &soundFactoryDs8;
	}
#endif

	throw runtime_error("Unknown sound factory: " + name);
}

}} // end namespace
