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
#include "logger.h"

#include "leak_dumper.h"

#include <algorithm>
#include <vector>

using namespace Shared::Graphics;
using namespace Shared::Sound;

namespace Glest { namespace Sound {
using Global::Config;
using Util::Logger;

const int SoundRenderer::ambientFade= 6000;
const float SoundRenderer::audibleDist= 50.f;

// =====================================================
// 	class SoundRenderer
// =====================================================

SoundRenderer::SoundRenderer() 
    : musicStream(0) {
	loadConfig();
}

void SoundRenderer::init(Window *window) {
	g_logger.logProgramEvent("Initialising sound renderer");

	SoundInterface &si= SoundInterface::getInstance();
	FactoryRepository &fr= FactoryRepository::getInstance();
	Config &config= Config::getInstance();

	g_logger.logProgramEvent("\tRequesting sound factory of type '" + config.getSoundFactory() + "'");
	si.setFactory(fr.getSoundFactory(config.getSoundFactory()));
	soundPlayer= si.newSoundPlayer();

	SoundPlayerParams soundPlayerParams;
	soundPlayerParams.staticBufferCount= config.getSoundStaticBuffers();
	soundPlayerParams.strBufferCount= config.getSoundStreamingBuffers();
	g_logger.logProgramEvent("\tInitialising SoundPlayer");
	soundPlayer->init(&soundPlayerParams);
}

SoundRenderer::~SoundRenderer(){
	delete soundPlayer;
}

SoundRenderer &SoundRenderer::getInstance(){
	static SoundRenderer soundRenderer;
	return soundRenderer;
}

void SoundRenderer::update(){
	soundPlayer->updateStreams();
}

// ======================= Music ============================
void SoundRenderer::playStream(StrSound *strSound, bool loop, int64 fadeOn, RequestNextStream cbFunc){
	strSound->restart();
	soundPlayer->play(strSound, loop, fadeOn, cbFunc);
}

void SoundRenderer::stopStream(StrSound *strSound, int64 fadeOff){
	soundPlayer->stop(strSound, fadeOff);
}

// ======================= Music ============================

void SoundRenderer::playMusic(StrSound *strSound, bool loop, RequestNextStream cbFunc){
	if (!strSound) {
        return;
	}

    if (musicStream) {
	    soundPlayer->stop(musicStream);
    }
	musicStream = strSound;

	strSound->setVolume(musicVolume);
    playStream(strSound, loop, 0, cbFunc);
}

void SoundRenderer::stopMusic(StrSound *strSound){
    if (strSound) {
	    soundPlayer->stop(strSound);
    }
	musicStream = 0;
}

void SoundRenderer::setMusicVolume(float v) {
	musicVolume = v;
	if (musicStream) {
		musicStream->setVolume(v);
	}
}

StrSound *NextMusicTrackCallback() {
	StrSound *nextTrack = g_soundRenderer.getNextMusicTrack();
	g_soundRenderer.setStream(nextTrack);
    return nextTrack;
}

void SoundRenderer::startMusicPlaylist() {
    StrSound *nextTrack = getNextMusicTrack();
    playMusic(nextTrack, false, NextMusicTrackCallback);
}

StrSound *SoundRenderer::getNextMusicTrack() {
    if (musicPlaylistQueue.empty()) {
        return 0;
    }

    stMusic &curMusic = musicPlaylistQueue.back();
    MusicPlaylistType *curPlaylist = curMusic.curPlaylist;
	
    if (curPlaylist == 0) {
		// determine first playlist
		if (curMusic.musicList.empty()) {
			return 0;
		} else if (curMusic.musicList.size() == 1) {
            curMusic.curPlaylist = const_cast<MusicPlaylistType*>(curMusic.musicList[0]);
        } else {
            curMusic.curPlaylist = const_cast<MusicPlaylistType*>(curMusic.getRandomPlaylist());
        }
    } else if (curPlaylist->isRandom()) {
		// do we need to consider the playlist as done playing?
		if (!curPlaylist->isLooping() 
				&& (curPlaylist->getActivationType() == MusicPlaylistType::eActivationType_ReplaceCurrent)) {
			if (curMusic.musicList.empty()) {
				return 0;
			} else if (curMusic.musicList.size() == 1) {
				musicPlaylistQueue.pop_back();
				return getNextMusicTrack();
			} else {
				curMusic.removePlaylist(curPlaylist);
				return getNextMusicTrack();
			}
		}

		curMusic.curPlaylist = const_cast<MusicPlaylistType*>(curMusic.getRandomPlaylist());
	}

	return curMusic.curPlaylist->getNextTrack();
}

void SoundRenderer::addPlaylist(const MusicPlaylistType *playlist) {
	 // verify that its not already in the current playlist group
	if (!musicPlaylistQueue.empty() && musicPlaylistQueue.back().containsPlaylist(playlist)) {
        return;
    }

    if (musicPlaylistQueue.empty() 
			|| (playlist->getActivationType() == MusicPlaylistType::eActivationType_ReplaceCurrent)) {
        stMusic newMusic;
        newMusic.musicList.push_back(playlist);
        musicPlaylistQueue.push_back(newMusic);
    } else if (playlist->getActivationType() == MusicPlaylistType::eActivationType_AddToCurrent) {
        musicPlaylistQueue.back().musicList.push_back(playlist);
    }
    
	if (playlist->getActivationMode() == MusicPlaylistType::eActivationMode_Interrupt) {
        startMusicPlaylist();
	}
}

const MusicPlaylistType *SoundRenderer::stMusic::getRandomPlaylist() const {
	int seed = int(Chrono::getCurTicks());
	Random random(seed);
	int curPlaylistIdx = random.randRange(0, musicList.size() - 1);
	return musicList[curPlaylistIdx];
}

void SoundRenderer::stMusic::removePlaylist(const MusicPlaylistType *playlist) {
	vectorMusicPlaylist::iterator it = std::find(musicList.begin(), musicList.end(), playlist);
	musicList.erase(it);
}

bool SoundRenderer::stMusic::containsPlaylist(const MusicPlaylistType *playlist) const {
	vectorMusicPlaylist::const_iterator it = std::find(musicList.begin(), musicList.end(), playlist);
	return it != musicList.end();
}

void SoundRenderer::removePlaylist(const MusicPlaylistType *playlist) {
	MusicPlaylistQueue::iterator queue = musicPlaylistQueue.begin();
	for (; queue != musicPlaylistQueue.end(); ++queue) {
		stMusic& curMusic = *queue;
		curMusic.removePlaylist(playlist);

		if (curMusic.musicList.empty()) {
			musicPlaylistQueue.erase(queue);
			return;
		}

		if (curMusic.curPlaylist == playlist) {
			curMusic.curPlaylist = 0;
		}
	}
}


// ======================= Fx ============================

void SoundRenderer::playFx(StaticSound *staticSound, Vec3f soundPos, Vec3f camPos){
	if(staticSound!=NULL){
		float d= soundPos.dist(camPos);

		if(d<audibleDist){
			float vol= (1.f-d/audibleDist)*fxVolume;
			float correctedVol= log10(log10(vol*9+1)*9+1);
			staticSound->setVolume(correctedVol);
			soundPlayer->play(staticSound);
		}
	}
}

void SoundRenderer::playFx(StaticSound *staticSound){
	if(staticSound!=NULL){
		staticSound->setVolume(fxVolume);
		soundPlayer->play(staticSound);
	}
}

void SoundRenderer::setFxVolume(float v) {
	fxVolume = v;
}

// ======================= Ambient ============================

void SoundRenderer::playAmbient(StrSound *strSound, bool loop){
	strSound->setVolume(ambientVolume);
	playStream(strSound, loop, ambientFade);
	ambientStreams.insert(strSound);
}

void SoundRenderer::stopAmbient(StrSound *strSound){
	stopStream(strSound, ambientFade);
	ambientStreams.erase(strSound);
}

void SoundRenderer::setAmbientVolume(float v) {
	ambientVolume = v;
	foreach (std::set<StrSound*>, it, ambientStreams) {
		(*it)->setVolume(v);
	}
}

// ======================= Misc ============================

void SoundRenderer::stopAllSounds(){
	soundPlayer->stopAllSounds();
	//this might be needed here, seems to cause lots of memory leaks though??? - hailstone 25Feb2012
	//musicPlaylistQueue.clear();
}

void SoundRenderer::loadConfig(){
	Config &config= Config::getInstance();

	fxVolume= config.getSoundVolumeFx()/100.f;
	musicVolume= config.getSoundVolumeMusic()/100.f;
	ambientVolume= config.getSoundVolumeAmbient()/100.f;
}

}}//end namespace
