// ==============================================================
//	This file is part of the Glest Advanced Engine (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "collections.h"

namespace Shared { namespace Util {

// =====================================================
// class SelectionContainer
// =====================================================
#if 0
SelectionContainer::~SelectionContainer() {
}

void SelectionContainer::manage(Selection *item) {
	if(managedItems.count(item)) {
		throw range_error("Item already exists in SelectionContainer!");
	}
	managedItems[item] = shared_ptr<Selection>(item);
}
#endif

#if 0
#include "sound.h"

#include <fstream>
#include <stdexcept>
//#include <boost/shared_ptr.hpp>
//using boost::shared_ptr;
#include "util.h"

#include "leak_dumper.h"


// =====================================================
// class SerialList
// =====================================================
SerialList(bool loop);


// =====================================================
// class RandomSoundPlayList
// =====================================================

const Selector<T> *RandomList::getSound() const {
	if(!selections.size()) {
		return NULL;
	}

	int64 now = Chrono::getCurMicros();
	Selections possibilities;


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
#endif

}} // end namespace
