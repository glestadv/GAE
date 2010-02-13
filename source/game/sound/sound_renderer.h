// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SOUNDRENDERER_H_
#define _GLEST_GAME_SOUNDRENDERER_H_

#include "sound.h"
#include "sound_player.h"
#include "window.h"
#include "vec.h"
#include "sound_factory.h"
#include "config.h"

namespace Glest { namespace Game {

using Shared::Sound::StreamSound;
using Shared::Sound::StaticSound;
using Shared::Sound::SoundPlayer;
using Shared::Graphics::Vec3f;
using Shared::Sound::SoundFactory;

//class Config;
class GuiProgram;

// =====================================================
// 	class SoundRenderer
//
///	Wrapper to acces the shared library sound engine
// =====================================================

class SoundRenderer {
public:
	//static const string EMPTY

	static const int ambientFade = 6000;
	static const float audibleDist = 50.f;

private:
	const Config &config;
	shared_ptr<SoundPlayer> soundPlayer;

public:
	SoundRenderer(const Config &config, SoundFactory &soundFactory);
	~SoundRenderer();

	static SoundRenderer &getInstance();


	float getFxVolume() const			{return config.getSoundVolumeFx() / 100.f;}
	float getMusicVolume() const		{return config.getSoundVolumeMusic() / 100.f;}
	float getAmbientVolume() const		{return config.getSoundVolumeAmbient() / 100.f;}

	void update();
	SoundPlayer &getSoundPlayer() const	{return *soundPlayer;}

	//music
	string playMusic(const Sound &sound, const string &playerName = "");
	void stopMusic(const string &playerName = "");

	//fx
	string playFx(const Sound &sound, const Vec3f &soundPos, const Vec3f &camPos, const string &playerName = "");
	string playFx(const Sound *sound, const Vec3f &soundPos, const Vec3f &camPos, const string &playerName = "")	{return !sound ? "" : playFx(*sound, soundPos, camPos, playerName);}
	string playFx(const Sound &sound, const string &playerName = "");
	string playFx(const Sound *sound, const string &playerName = "")												{return !sound ? "" : playFx(*sound, playerName);}

	//ambient
	string playAmbient(const Sound &sound, const string &playerName = "");
	void stopAmbient(const string &playerName = "");

	bool isPlaying(const string &playerName);

	//misc
	void stopAllSounds();
};

}} // end namespace

#endif
