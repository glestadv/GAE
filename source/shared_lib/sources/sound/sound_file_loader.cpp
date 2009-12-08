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

#include "pch.h"
#include "sound_file_loader.h"

#include <stdexcept>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "sound.h"
#include "util.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace std;

namespace Shared { namespace Sound {

shared_ptr<SoundFileLoader> SoundFileLoader::open(const Sound &sound, bool initSoundObject) {
	const string &path = sound.getPath();
	string ext = Shared::Util::toLower(Shared::Util::ext(path));
	if(ext == "wav") {
		return shared_ptr<SoundFileLoader>(new WavSoundFileLoader(path, initSoundObject));
	} else if(ext == "ogg") {
		return shared_ptr<SoundFileLoader>(new OggSoundFileLoader(path, initSoundObject));
	} else {
		throw range_error("No support for sound file extension " + ext);
	}
}

// =====================================================
// class WavSoundFileLoader
// =====================================================
// FIXME: Non-portable code! This code will not work on PPC or ARM!!
void WavSoundFileLoader::WavSoundFileLoader(const Sound &sound, bool initSoundObject)
		: SoundFileLoader()
		, dataOffset(0)
		, dataSize(0)
		, bytesPerSecond(0)
		, f(sound.getPath().c_str(), ios_base::in | ios_base::binary) {

	const string &path = sound.getPath();
    char chunkId[] = {'-', '-', '-', '-', '\0'};
    uint32 size32 = 0;
    uint16 size16 = 0;
    int count;

    if (!f.is_open()) {
        throw runtime_error("Error opening wav file: " + path);
    }

    //RIFF chunk - Id
    f.read(chunkId, 4);

    if (strcmp(chunkId, "RIFF") != 0) {
        throw runtime_error("Not a valid wav file (first four bytes are not RIFF):" + path);
    }

    //RIFF chunk - Size
    f.read((char*) &size32, 4);

    //RIFF chunk - Data (WAVE string)
    f.read(chunkId, 4);

    if (strcmp(chunkId, "WAVE") != 0) {
        throw runtime_error("Not a valid wav file (wave data don't start by WAVE): " + path);
    }

    // === HEADER ===

    //first sub-chunk (header) - Id
    f.read(chunkId, 4);

    if (strcmp(chunkId, "fmt ") != 0) {
        throw runtime_error("Not a valid wav file (first sub-chunk Id is not fmt): " + path);
    }

    //first sub-chunk (header) - Size
    f.read((char*) &size32, 4);

    //first sub-chunk (header) - Data (encoding type) - Ignore
    f.read((char*) &size16, 2);

    //first sub-chunk (header) - Data (nChannels)
    f.read((char*) &size16, 2);
	if(initSoundObject) {
    	const_cast<Sound&>(sound).setChannels(size16);
	} else {
		assert(sound.getChannels() == size16);
	}

    //first sub-chunk (header) - Data (nsamplesPerSecond)
    f.read((char*) &size32, 4);
	if(initSoundObject) {
    	const_cast<Sound&>(sound).setSamplesPerSecond(size32);
	} else {
		assert(sound.getSamplesPerSecond() == size32);
	}

    //first sub-chunk (header) - Data (nAvgBytesPerSec)  - Ignore
    f.read((char*) &size32, 4);

    //first sub-chunk (header) - Data (blockAlign) - Ignore
    f.read((char*) &size16, 2);

    //first sub-chunk (header) - Data (nsamplesPerSecond)
    f.read((char*) &size16, 2);
	if(initSoundObject) {
    	const_cast<Sound&>(sound).setBitsPerSample(size16);
	} else {
		assert(sound.getBitsPerSample() == size16);
	}

    if (sound.getBitsPerSample() != 8 && sound.getBitsPerSample() != 16) {
        throw range_error("Bits per sample must be 8 or 16: " + path);
    }
    bytesPerSecond = sound.getBitsPerSample() / 8 * sound.getSamplesPerSecond() * sound.getChannels();

    count = 0;
    do {
        count++;

        // === DATA ===
        //second sub-chunk (samples) - Id
        f.read(chunkId, 4);
        if (strncmp(chunkId, "data", 4) != 0) {
            continue;
        }

        //second sub-chunk (samples) - Size
        f.read((char*) &size32, 4);
        dataSize = size32;
		if(initSoundObject) {
        	const_cast<Sound&>(sound).setSize(dataSize);
		} else {
			assert(sound.getSize() == dataSize);
		}
    } while (strncmp(chunkId, "data", 4) != 0 && count < maxDataRetryCount);

    if (f.bad() || count == maxDataRetryCount) {
        throw runtime_error("Error reading samples: " + path);
    }

    dataOffset = f.tellg();
}

uint32 WavSoundFileLoader::read(int8 *samples, uint32 size) {
    f.read(reinterpret_cast<char*> (samples), size);
    return f.gcount();
}

void WavSoundFileLoader::close() {
    f.close();
}

void WavSoundFileLoader::restart() {
    f.seekg(dataOffset, ios_base::beg);
}

// =======================================
//        Ogg Sound File Loader
// =======================================

OggSoundFileLoader::OggSoundFileLoader(const Sound &sound, bool initSoundObject)
		: SoundFileLoader()
		, vf(NULL)
		, f(NULL) {

	const string &path = sound.getPath();
	if (!(f = fopen(path.c_str(), "rb"))) {
		throw runtime_error("Can't open ogg file: " + path);
	}

	vf = new OggVorbis_File();
	if (ov_open(f, vf, NULL, 0)) {
		fclose(f);
		f = NULL;
		throw runtime_error("ov_open failed on ogg file: " + path);
	}
	// ogg assumes the file pointer.
	f = NULL;

	vorbis_info *vi = ov_info(vf, -1);

	if (initSoundObject) {
		const_cast<Sound&>(sound).setChannels(vi->channels);
		const_cast<Sound&>(sound).setSamplesPerSecond(vi->rate);
		const_cast<Sound&>(sound).setBitsPerSample(16);
		const_cast<Sound&>(sound).setSize(static_cast<uint32>(ov_pcm_total(vf, -1)) * 2);
	} else {
		assert(sound.getChannels() == vi->channels);
		assert(sound.getSamplesPerSecond() == vi->rate);
		assert(sound.getBitsPerSample() == 16);
		assert(sound.getSize() == static_cast<uint32>(ov_pcm_total(vf, -1)) * 2);
	}
}

uint32 OggSoundFileLoader::read(int8 *samples, uint32 size) {
    int section;
    int totalBytesRead = 0;

    while (size > 0) {
        int bytesRead = ov_read(vf, reinterpret_cast<char*> (samples), size, 0, 2, 1, &section);
        if (bytesRead == 0) {
            break;
        }
        size -= bytesRead;
        samples += bytesRead;
        totalBytesRead += bytesRead;
    }
    return totalBytesRead;
}

void OggSoundFileLoader::close() {
    if (vf) {
        ov_clear(vf);
        delete vf;
        vf = NULL;
    }
}

void OggSoundFileLoader::restart() {
    ov_raw_seek(vf, 0);
}

}}//end namespace
