// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_SOUND_SOUNDPLAYEROPENAL_H_
#define _SHARED_SOUND_SOUNDPLAYEROPENAL_H_

#include "sound_player.h"
#include "platform_util.h"

#include <vector>
#include <SDL.h>
#include <AL/alc.h>
#include <AL/al.h>

#include "timer.h"

using std::vector;
using Shared::Platform::Chrono;

namespace Shared { namespace Sound { namespace OpenAL {

class SoundSource {
protected:
	friend class SoundPlayerOpenAL;
	ALuint source;

public:
	SoundSource();
	virtual ~SoundSource();

	bool playing();
	void stop();

protected:
	ALenum getFormat(const Sound &sound);
};

class StaticSoundSource : public SoundSource {
protected:
	friend class SoundPlayerOpenAL;
	bool bufferAllocated;
	ALuint buffer;

public:
	StaticSoundSource();
	virtual ~StaticSoundSource();

	void play(const StaticSound &sound, float attenuation);
};

class StreamSoundSource : public SoundSource {
private:
	friend class SoundPlayerOpenAL;
	static const size_t STREAMBUFFERSIZE = 1024 * 500;
	static const size_t STREAMFRAGMENTS = 5;
	static const size_t STREAMFRAGMENTSIZE = STREAMBUFFERSIZE / STREAMFRAGMENTS;

	StrSound* sound;
	ALuint buffers[STREAMFRAGMENTS];
	ALenum format;

	enum FadeState { NoFading, FadingOn, FadingOff };
	FadeState fadeState;
	Chrono chrono; // delay-fade chrono
	int64 fade;

public:
	StreamSoundSource();
	virtual ~StreamSoundSource();

	void play(const StrSound &sound, float attenuation, int64 fade);
	void update();
	void stop();
	void stop(int64 fade);

private:
	bool fillBufferAndQueue(ALuint buffer);
};

// ==============================================================
//	class SoundPlayerSDL
//
///	SoundPlayer implementation using SDL_mixer
// ==============================================================

class SoundPlayerOpenAL : public SoundPlayer {
private:
	friend class SoundSource;
	friend class StaticSoundSource;
	friend class StreamSoundSource;

	typedef std::vector<StaticSoundSource*> StaticSoundSources;
	typedef std::vector<StreamSoundSource*> StreamSoundSources;

	ALCdevice* device;
	ALCcontext* context;
	StaticSoundSources staticSources;
	StreamSoundSources streamSources;
	SoundPlayerParams params;

public:
	SoundPlayerOpenAL();
	virtual ~SoundPlayerOpenAL();
	virtual void init(const SoundPlayerParams &params);
	virtual void end();
	virtual void play(const StaticSound &staticSound, float attenuation);
	virtual void play(const StrSound &strSound, float attenuation, int64 fadeOn=0);
	virtual void stop(const StrSound &strSound, int64 fadeOff=0);
	virtual void stopAllSounds();
	virtual void updateStreams();	//updates str buffers if needed

private:
	void printOpenALInfo();
	StaticSoundSource* findStaticSoundSource();
	StreamSoundSource* findStreamSoundSource();
	void checkAlcError(const char* message);
	static void checkAlError(const char* message);
};

}}} // end namespace

#endif

