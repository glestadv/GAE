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
#include "sound.h"

#include <fstream>
#include <stdexcept>
//#include <boost/shared_ptr.hpp>
//using boost::shared_ptr;
#include "util.h"

#include "leak_dumper.h"

namespace Shared { namespace Sound {
#if 0
// =====================================================
// class SerialSoundPlayList
// =====================================================

const Sound *SerialSoundPlayList::getSound() const {
	const Playables &playables = getPlayables();
	if(!playables.size()) {
		return NULL;
	}

	++lastItem;
	if(lastItem == playables.end()) {
		lastItem = playables.begin();
	}
	return lastItem->getSound();
}

// =====================================================
// class RandomSoundPlayList
// =====================================================

const Sound *RandomSoundPlayList::getSound() const {
	const Playables &playables = getPlayables();
	if(!playables.size()) {
		return NULL;
	}

	int64 now = Chrono::getCurMicros();
	Playables possibilities;


	if(noRepeatSize) {
		int64 staleTime = now - noRepeatInterval;

		// cull history
		while(history.size()) {
			if(history.front().second < staleTime) {
				history.pop_front();
			}
		}

		// If non-repeatable history contains all items then no sound this time.
		if(history.size() == playables.size()) {
			return NULL;
		}

		// calculate the value to subtract from the total
		foreach(History::value &h, history) {
			historyValue += h.first->second;
		}
	}

	float nextValue = 0.f;
	foreach(Playables::const_iterator &p, playables) {
		bool inHistory = false;
		foreach(History::value &h, history) {
			if(h.first->first == p) {
				inHistory = true;
				break;
			}
		}

		if(!inHistory) {
			possibilities.push_back(Playables::value(p.first, nextValue));
			nextValue += h.second;
		}
		nextValue

	float random

	++lastItem;
	if(lastItem == playables.end()) {
		lastItem = playables.begin();
	}
	return lastItem->getSound();
}

class SoundPlayList : public Playable {
public:
	typedef dequeue<pair<Playables::const_iterator, int64> > History;

private:
	size_t noRepeatSize;			/**< The number of history items to not repeat */
	int64 noRepeatInterval;			/**< The max interval to use the history before resetting it. */
	mutable History history;
	mutable Random random;

public:
	SoundContainer(int64 seed, size_t noRepeatSize, int64 noRepeatInterval);

	const Sound *getSound() const;
};
#endif
// =====================================================
// class Sound
// =====================================================

Sound::Sound(const string &path)
		: path(path)
		, channels(0)
		, samplesPerSecond(0)
		, bitsPerSample(0)
		, size(0) {
}

// =====================================================
// class StaticSound
// =====================================================

StaticSound::StaticSound(const string &path) : Sound(path) {
	shared_ptr<SoundFileLoader> sfl = init();
	samples.resize(getSize());
	sfl->read(&samples.front(), getSize());
}

StaticSound::~StaticSound() {
}

// =====================================================
// class StreamSound
// =====================================================

StreamSound::StreamSound(const string &path) : Sound(path) {
	init();
}

StreamSound::~StreamSound() {
}


/*uint32 StreamSound::read(int8 *samples, uint32 size){
 return soundFileLoader->read(samples, size);
}

void StreamSound::close(){
 if(soundFileLoader!=NULL){
  soundFileLoader->close();
  delete soundFileLoader;
  soundFileLoader= NULL;
 }
}

void StreamSound::restart(){
 soundFileLoader->restart();
}
*/
}} // end namespace
