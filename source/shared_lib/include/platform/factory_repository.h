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

#ifndef _SHARED_PLATFORM_FACTORYREPOSITORY_H_
#define _SHARED_PLATFORM_FACTORYREPOSITORY_H_

#include <string>

#include "gae_features.h"
#include "graphics_factory.h"
#include "sound_factory.h"

#include "graphics_factory_gl.h"
//#include "graphics_factory_gl2.h"

#if USE_OPENAL
#	include "sound_factory_openal.h"
#elif USE_DS8
#	include "sound_factory_ds8.h"
#else
#	error No sound library specified
#endif

#include "patterns.h"

using std::string;

using Shared::Graphics::GraphicsFactory;
using Shared::Sound::SoundFactory;
using Shared::Graphics::Gl::GraphicsFactoryGl;
using Shared::Graphics::Gl::ContextGl;
#if USE_OPENAL
using Shared::Sound::OpenAL::SoundFactoryOpenAL;
#elif USE_DS8
using Shared::Sound::Ds8::SoundFactoryDs8;
#endif

namespace Shared { namespace Platform {

// =====================================================
//	class FactoryRepository
// =====================================================

/**
 * Provides factories for graphics & sound.  So it's almost a factory factory, isn't that fucking
 * brilliant!?
 */
class FactoryRepository : Uncopyable {
private:
	ContextGl &context;
	string graphicsFactoryName;
	string soundFactoryName;
	GraphicsFactory *graphicsFactory;
	SoundFactory *soundFactory;

public:
	FactoryRepository(ContextGl &context, const string &graphicsFactoryName, const string &soundFactoryName);
	~FactoryRepository();

	GraphicsFactory &getGraphicsFactory()	{return *graphicsFactory;}
	SoundFactory &getSoundFactory()			{return *soundFactory;}
};

}}//end namespace

#endif
