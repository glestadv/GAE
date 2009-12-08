// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_SOUND_SOUNDFILELOADER_H_
#define _SHARED_SOUND_SOUNDFILELOADER_H_

#include <string>
#include <fstream>

#include "types.h"

struct OggVorbis_File;

using std::string;
using std::ifstream;

namespace Shared { namespace Sound {

using Platform::uint32;
using Platform::int8;

class Sound;

// =====================================================
// class SoundFileLoader
//
/// Interface that all SoundFileLoaders will implement
// =====================================================

class SoundFileLoader {
	friend class Sound;
public:
	virtual ~SoundFileLoader() {}

	/**
	 * Opens the file specified in the sound parameter.  The Sound object must have already been
	 * initialized prior to calling this function.
	 * @return A SoundFileLoader-derived object that can read the specified sound file
	 * @throw range_error if the extension is not either .wav or .ogg
	 */
	static shared_ptr<SoundFileLoader> open(const Sound &sound)	{return open(sound, false);}

	virtual uint32 read(int8 *samples, uint32 size) = 0;
	virtual void close() = 0;
	virtual void restart() = 0;
private:
	/** Version of open to be called by Sound for initialization */
	static shared_ptr<SoundFileLoader> open(const Sound &sound, bool initSoundObject);
};

// =====================================================
// class WavSoundFileLoader
//
/// Wave file loader
// =====================================================

class WavSoundFileLoader: public SoundFileLoader {
private:
	static const int maxDataRetryCount = 10;

private:
	uint32 dataOffset;
	uint32 dataSize;
	uint32 bytesPerSecond;
	ifstream f;

public:
	WavSoundFileLoader(const Sound &sound, bool initSoundObject);
	~WavSoundFileLoader() {close();}
	uint32 read(int8 *samples, uint32 size);
	void close();
	void restart();
};

// =====================================================
// class OggSoundFileLoader
//
/// OGG sound file loader, uses ogg-vorbis library
// =====================================================

class OggSoundFileLoader: public SoundFileLoader {
private:
	OggVorbis_File *vf;
	FILE *f;

public:
	OggSoundFileLoader(const Sound &sound, bool initSoundObject);
	~OggSoundFileLoader() {close();}
	uint32 read(int8 *samples, uint32 size);
	void close();
	void restart();
};

}
}//end namespace

#endif
