// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_SOUND_SOUND_H_
#define _SHARED_SOUND_SOUND_H_

#include "sound_file_loader.h"
#include "collections.h"

#include <string>
#include <vector>
//#include <dequeue>

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Shared { namespace Sound {

class Sound;

// =====================================================
// class Sound
// =====================================================

class Sound /*: public Selector<Sound> */{
private:
	string path;
	uint32 channels;
	uint32 samplesPerSecond;
	uint32 bitsPerSample;
	uint32 size;

public:
	Sound(const string &path);
	virtual ~Sound() {};

//	const Sound *getSelection() const	{return this;}
	const string &getPath() const		{return path;}
	uint32 getChannels() const			{return channels;}
	uint32 getSamplesPerSecond() const	{return samplesPerSecond;}
	uint32 getBitsPerSample() const		{return bitsPerSample;}
	uint32 getSize() const				{return size;}

	void setChannels(uint32 v)			{channels = v;}
	void setSamplesPerSecond(uint32 v)	{samplesPerSecond = v;}
	void setBitsPerSample(uint32 v)		{bitsPerSample = v;}
	void setSize(uint32 v)				{size = v;}

	bool operator==(const Sound &other) const	{return path == other.path;}

protected:
	shared_ptr<SoundFileLoader> init() {return SoundFileLoader::open(*this, true);}
};

// =====================================================
// class StaticSound
// =====================================================

class StaticSound: public Sound {
private:
	vector<int8> samples;

public:
	StaticSound(const string &path);
	virtual ~StaticSound();

	const int8 *getSamples() const		{return &samples.front();}
};

// =====================================================
// class StreamSound
// =====================================================

class StreamSound: public Sound {
private:
	string path;

public:
	StreamSound(const string &path);
	virtual ~StreamSound();
//bool operator==(const Sound &other) const{return Sound::operator==(other);}
//	shared_ptr<SoundFileLoader> getSoundFileLoader() const;
	//void open(const string &path);
	//uint32 read(int8 *samples, uint32 size);
	//void close();
	//void restart();
};

// =====================================================
// class StreamSound
// =====================================================

typedef vector<const Sound *> Sounds;


}}//end namespace

#endif
