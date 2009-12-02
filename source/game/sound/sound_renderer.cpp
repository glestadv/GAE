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

#include "pch.h"
#include "sound_renderer.h"

#include "core_data.h"
#include "config.h"
#include "sound_interface.h"
#include "factory_repository.h"
#include "gui_program.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Sound;

namespace Glest { namespace Game {

// =====================================================
//  class SoundRenderer
// =====================================================

SoundRenderer::SoundRenderer(const Config &config, SoundFactory &soundFactory)
		: config(config)
//		, fxVolume(config.getSoundVolumeFx() / 100.f)
//		, musicVolume(config.getSoundVolumeMusic() / 100.f)
//		, ambientVolume(config.getSoundVolumeAmbient() / 100.f)
		, soundPlayer(soundFactory.newSoundPlayer()) {
	SoundPlayerParams soundPlayerParams;
	soundPlayerParams.staticBufferCount = config.getSoundStaticBuffers();
	soundPlayerParams.strBufferCount = config.getSoundStreamingBuffers();
	soundPlayer->init(soundPlayerParams);
}

SoundRenderer &SoundRenderer::getInstance() {
	return GuiProgram::getInstance().getSoundRenderer();
}

void SoundRenderer::update() {
	soundPlayer->updateStreams();
}

// ======================= Music ============================

void SoundRenderer::playMusic(const StrSound &strSound) {
	//strSound->setVolume(musicVolume);
	//strSound->restart();
#warning	soundPlayer->restart();
	soundPlayer->play(strSound, getMusicVolume());
}

void SoundRenderer::stopMusic(const StrSound &strSound) {
	soundPlayer->stop(strSound);
}

// ======================= Fx ============================

void SoundRenderer::playFx(const StaticSound &staticSound, const Vec3f &soundPos, const Vec3f &camPos) {
	//if (staticSound != NULL) {
		float d = soundPos.dist(camPos);

		if (d < audibleDist) {
			float vol = (1.f - d / audibleDist) * getFxVolume();
			float correctedVol = log10(log10(vol * 9 + 1) * 9 + 1);
			//staticSound->setVolume(correctedVol);
			soundPlayer->play(staticSound, correctedVol);
		}
	//}
}

void SoundRenderer::playFx(const StaticSound &staticSound) {
	//if (staticSound != NULL) {
	//	staticSound->setVolume(fxVolume);
		soundPlayer->play(staticSound, getFxVolume());
	//}
}

// ======================= Ambient ============================

void SoundRenderer::playAmbient(const StrSound &strSound) {
	//strSound->setVolume(ambientVolume);
	soundPlayer->play(strSound, getAmbientVolume(), ambientFade);
}

void SoundRenderer::stopAmbient(const StrSound &strSound) {
	soundPlayer->stop(strSound, ambientFade);
}

// ======================= Misc ============================

void SoundRenderer::stopAllSounds() {
	soundPlayer->stopAllSounds();
}

}} // end namespace
