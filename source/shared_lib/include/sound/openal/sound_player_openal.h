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
private:
	ALuint source;
	const Sound *sound;
	float volume;

public:
	SoundSource();
	virtual ~SoundSource();


	template<class T> const T *getSound() const	{return reinterpret_cast<const T *>(sound);}
	float getVolume() const						{return volume;}

	bool playing();
	void stop();

protected:
	void setSound(const Sound *v)				{sound = v;}
	void setVolume(float v)						{volume = v;}
	ALuint &getSource()							{return source;}
	ALenum getFormat(const Sound &sound);
};

class StaticSoundSource : public SoundSource {
private:
	bool bufferAllocated;
	ALuint buffer;

public:
	StaticSoundSource();
	virtual ~StaticSoundSource();

	const StaticSound *getSound() const		{return SoundSource::getSound<StaticSound>();}

	void play(const StaticSound &sound, float attenuation);
};

class StreamSoundSource : public SoundSource {
private:
	static const size_t STREAMBUFFERSIZE = 1024 * 500;
	static const size_t STREAMFRAGMENTS = 5;
	static const size_t STREAMFRAGMENTSIZE = STREAMBUFFERSIZE / STREAMFRAGMENTS;

	//const StreamSound *sound;
	shared_ptr<SoundFileLoader> soundFileLoader;
	ALuint buffers[STREAMFRAGMENTS];
	ALenum format;

	enum FadeState { NoFading, FadingOn, FadingOff };
	FadeState fadeState;
	Chrono chrono; // delay-fade chrono
	int64 fade;

public:
	StreamSoundSource();
	virtual ~StreamSoundSource();

	const StreamSound *getSound() const		{return SoundSource::getSound<StreamSound>();}

	void play(const StreamSound &sound, float attenuation, int64 fade);
	void update();
	void stop(int64 fade = 0);

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
	typedef std::vector<StaticSoundSource> StaticSoundSources;
	typedef std::vector<StreamSoundSource> StreamSoundSources;

	ALCdevice* device;
	ALCcontext* context;
	StaticSoundSources staticSources;
	StreamSoundSources streamSources;
	SoundPlayerParams params;

public:
	SoundPlayerOpenAL();
	virtual ~SoundPlayerOpenAL();
	void init(const SoundPlayerParams &params);
	void end();
	void play(const StaticSound &staticSound, float attenuation);
	void play(const StreamSound &streamSound, float attenuation, int64 fadeOn=0);
	void stop(const StreamSound &streamSound, int64 fadeOff=0);
	void stopAllSounds();
	void updateStreams();	//updates str buffers if needed

	static void checkAlError(const char* message);

private:
	void printOpenALInfo();
	StaticSoundSource* findStaticSoundSource();
	StreamSoundSource* findStreamSoundSource();
	void checkAlcError(const char* message);
};

}}} // end namespace

#endif

