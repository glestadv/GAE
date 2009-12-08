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

#ifndef _SHARED_UTIL_COLLECTIONS_H_
#define _SHARED_UTIL_COLLECTIONS_H_

//#include <string>
#include <vector>
#include <map>
//#include <dequeue>

#include "random.h"

//#include <boost/tuple/tuple.hpp>
using std::vector;
using std::map;
//using namespace Shared::Platform;
//using boost::tuple;

namespace Shared { namespace Util {

//class Object;

/**
 * Abstract base class for any object that can provide a pointer to a selection, which is an
 * instance of type T.
 */
template<class T> class Selector {
public:
	virtual ~Selector() {}
	virtual const T *getSelection() const = 0;
};

/** Class that provides selections serially, returning the next successive selection in the list. */
template<class T> class SerialList : public Selector<T> {
public:
	typedef vector<Selector<T>*> Selections;

private:
	Selections selections;		/**< Collection of possible selections. */
	bool loop;					/**< Rather or not to rewind when the last selection is reached. */
	mutable typename Selections::const_iterator last;	/**< The last item selected. */

public:
	SerialList(bool loop)
			: selections()
			, loop(loop)
			, last(selections.end()) {
	}

	/**
	 * Add a new selector to the end of the selections vector.  If last was pointing to the end of
	 * the vector, then it should now point at this selector.
	 */
	void add(const Selector<T> *selector) {
		selections.push_back(selector);
		if(last == selections.end()) {
			--last;
		}
	}

	/** Returns true if this object is ready to make a selection. */
	bool isReady() const {
		return selections.size() && (last != selections.end() || loop);
	}

	/** Resets the state so that last will point to the first element in selections, if there is one. */
	void reset() {
		last = selections.size() ? selections.begin() : selections.end();
	}

	/** Return the next selector in selection or NULL if there are no selections or if the end has been reached and looping is not specified. */
	const T *getSelection() const {
		if(!isReady()) {
			return NULL;
		}

		if(last == selections.end()) {
			if(!loop) {
				return NULL;
			} else {
				last == selections.begin();
			}
		}
		return last++->getSelection();
	}
};

template<class T> class RandomList : public Selector<T> {
public:
	typedef map<float, const Selector<T> *> Selections;

private:
	Selections selections;
	float totalRandomRange;
	mutable Random random;

public:
	RandomList(int seed)
			: selections()
			, totalRandomRange(0.f)
			, random(seed) {
	}

	bool isReady() const {
		return selections.size();
	}

	void add(const Selector<T> *selector, float chance)	{
		assert(chance > 0.f);
		selections[totalRandomRange] = selector;
		totalRandomRange += chance;
	}

	const T *getSelection() const {
		float value = random.randRange(0.f, totalRandomRange);
		typename Selections::const_iterator i = selections.lower_bound(value);
		assert(i != selections.end()); // this should never happen here unless something is goofed
		return i->second->getSelection();
	}
};

}} // end namespace

#endif

