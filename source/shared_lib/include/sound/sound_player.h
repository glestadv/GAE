// ==============================================================
// This file is part of Glest Shared Library (www.glest.org)
//
// Copyright (C) 2001-2008 Martiño Figueroa
//
// You can redistribute this code and/or modify it under
// the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_SOUND_SOUNDPLAYER_H_
#define _SHARED_SOUND_SOUNDPLAYER_H_

#include "sound.h"
#include "types.h"
#include "collections.h"
#include "entity.h"

using Shared::Platform::uint32;

namespace Shared { namespace Sound {

class Source {
public:
	enum PositionType {
		NONE,	/**< The sound source has no position (i.e., is the same as the listener). */
		FIXED,	/**< The sound source has a fixed position. */
		ENTITY	/**< The sound source is an Entity, who's position can change. */
	};

private:
	const Sound *sound;
	float volume;
	PositionType positionType;
	Vec3f pos;
	const Entity *entity;

public:
	Source()
			: sound(NULL)
			, volume(0.f)
			, positionType(NONE)
			, pos()
			, entity(NULL) {
	}
	virtual ~Source() {}

	template<class T> const T *getSound() const	{return reinterpret_cast<const T *>(sound);}
	float getVolume() const						{return volume;}
	PositionType getPositionType() const		{return positionType;}
	const Vec3f &getPost() const				{return pos;}
	const Entity *getEntity() const				{return entity;}

	virtual bool playing() = 0;
	virtual void stop() = 0;

protected:
	void play(const Sound *sound, float volume, PositionType positionType) {
		this->sound = sound;
		this->volume = volume;
		this->positionType = positionType;
	}
	void setVolume(float v)				{volume = v;}
	void setPos(const Vec3f &v)			{pos = v;}
	void setEntity(const Entity *v)		{entity = v; pos = entity->getCurrVector();}
};

// =====================================================
// class SoundPlayerParams
// =====================================================

class SoundPlayerParams {
public:
    uint32 strBufferSize;
    uint32 strBufferCount;
    uint32 staticBufferCount;

    SoundPlayerParams();
};

// =====================================================
// class SoundPlayer
// =====================================================

/** Interface that every SoundPlayer will implement */
class SoundPlayer {
public:
    virtual ~SoundPlayer() {};
    virtual void init(const SoundPlayerParams &params) = 0;
    virtual void end() = 0;
    virtual void play(const StaticSound &staticSound, float attenuation) = 0;
    virtual void play(const StreamSound &streamSound, float attenuation, int64 fadeOn = 0) = 0; //delay and fade in miliseconds
    virtual void stop(const StreamSound &streamSound, int64 fadeOff = 0) = 0;
    virtual void stopAllSounds() = 0;
    virtual void updateStreams() = 0;
};

}} // end namespace

#endif
