// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_SOUNDRENDERER_H_
#define _GAME_SOUNDRENDERER_H_

#include "sound.h"
#include "sound_player.h"
#include "window.h"
#include "vec.h"

namespace Game {

using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;
using Shared::Sound::SoundPlayer;
using Shared::Graphics::Vec3f;
using Shared::Platform::Window;

// =====================================================
// 	class SoundRenderer
//
///	Wrapper to acces the shared library sound engine
// =====================================================

class SoundRenderer{
public:
	static const int ambientFade;
	static const float audibleDist;
private:
	SoundPlayer *soundPlayer;

	//volume
	float fxVolume;
	float musicVolume;
	float ambientVolume;

private:
	SoundRenderer();

public:
	//misc
	~SoundRenderer();
	static SoundRenderer &getInstance();
	void init(Window *window);
	void update();
	SoundPlayer *getSoundPlayer() const	{return soundPlayer;}

	//music
	void playMusic(StrSound *strSound);
	void stopMusic(StrSound *strSound);

	//fx
	void playFx(StaticSound *staticSound, Vec3f soundPos, Vec3f camPos);
	void playFx(StaticSound *staticSound);

	//ambient
	//void playAmbient(StaticSound *staticSound);
	void playAmbient(StrSound *strSound);
	void stopAmbient(StrSound *strSound);

	//misc
	void stopAllSounds();
	void loadConfig();
};

} // end namespace

#endif
