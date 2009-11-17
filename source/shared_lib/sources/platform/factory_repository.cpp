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

FactoryRepository(const string &graphicsFactoryName, const string &soundFactoryName)
		: graphicsFactoryName(graphicsFactoryName)
		, soundFactoryName(soundFactoryName)
		, graphicsFactory(NULL)
		, soundFactory(NULL) {

	if (graphicsFactoryName == "OpenGL") {
		graphicsFactory = new GraphicsFactoryGl;
	} else if (graphicsFactoryName == "OpenGL2") {
		graphicsFactory = new GraphicsFactoryGl2;
	} else {
		throw runtime_error("Unknown graphics factory: " + graphicsFactory);
	}

	if(0) {
#if USE_OPENAL
	} else if (soundFactoryName == "OpenAL") {
		soundFactory = new SoundFactoryOpenAL;
#endif

#if USE_DS8
	} else if (soundFactoryName == "DirectSound8") {
		soundFactory = new SoundFactoryDs8;
#endif

	} else {
		throw runtime_error("Unknown sound factory: " + soundFactoryName);
	}
}

FactoryRepository::~FactoryRepository() {
	if(graphicsFactory) {
		delete graphicsFactory;
	}

	if(soundFactory) {
		delete soundFactory;
	}
}

}} // end namespace
