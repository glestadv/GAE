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

/**
 * @file
 *
 * A generic library of for playlist-style containers of objects.
 */

//#include <string>
#include <stdexcept>
#include <cassert>
#include <vector>
#include <map>
//#include <dequeue>

#include "random.h"
#include "util.h"
#include "temporal_data_collection.h"

//#include <boost/tuple/tuple.hpp>
using std::vector;
using std::map;
using std::range_error;
using Shared::Platform::Chrono;
//using namespace Shared::Platform;
//using boost::tuple;

namespace Shared { namespace Util {

//class Object;
//class SingleSelection;
//class ManagedSingleSelection;
//class SerialSelection;
//class RandomSelection
//class RandomSelectionWithNoRepeat;

/**
 * A simple utility class that manages the lifecycle of objects, destroying them all when the
 * LifecycleManager is destroyed.
 */
template<class T> class LifecycleManager {
private:
	typedef map<T *, shared_ptr<T> > ManagedItems;
	ManagedItems managedItems;			/**< Items who's lifecycle is managed by this container. */

public:
	virtual ~LifecycleManager() {}

	/** Call to have the lifecycle of item managed by this object. */
	T *add(T *item) {
		if(managedItems.count(item)) {
			throw range_error("Item already exists in SelectionContainer!");
		}
		managedItems[item] = shared_ptr<T>(item);
		return item;
	}

	/**
	 * Returns a pointer to the shared_ptr object for the supplied item if it exists, NULL
	 * otherwise.
	 */
	const shared_ptr<T> *getSharedPointer(T *item) const {
		typename ManagedItems::const_iterator i = managedItems.find(item);
		return i == managedItems.end() ? NULL : i->second;
	}
};

/**
 * Abstract base class for any object that can provide a pointer to a selection, which is an
 * instance of type T.  The term "selection" is used because it can refer to both a collection of
 * items and a single "selected" item.
 */
template<class T> class Selection {
public:
	virtual ~Selection() {}
	virtual const T *getNext() const = 0;
	virtual const T *getLast() const = 0;
};

/**
 * A single selection item, always returning the same object.  The object passed to the constructor
 * is expected to remain valid throughout the lifetime of the SingleSelection object.
 */
template<class T> class SingleSelection : public Selection<T> {
private:
	const T &single;

public:
	SingleSelection(const T &single) : single(single) {}
	const T *getNext() const {return &single;}
	const T *getLast() const {return &single;}
};

/**
 * Contains a single item, wrapped in a shared_ptr.  Like SingleSelection, it always returns the
 * same object.
 */
template<class T> class ManagedSingleSelection : public Selection<T> {
private:
	shared_ptr<T> single;

public:
	ManagedSingleSelection(shared_ptr<T> &single) : single(single) {}
	ManagedSingleSelection(T * single) : single(single) {}
	const T *getNext() const {return single.get();}
	const T *getLast() const {return single.get();}
};


#if 0
template<class T> class SelectionContainer : public Selection<T> {
public:
	typedef map<Selection<T> *, shared_ptr<Selection<T> > > ManagedItems;

private:
	ManagedItems managedItems;			/**< Items who's lifecycle is managed by this container. */

protected:
	void manage(Selection<T> *item)	{
		if(managedItems.count(item)) {
			throw range_error("Item already exists in SelectionContainer!");
		}
		managedItems[item] = shared_ptr<Selection<T> >(item);
	}
};
#endif

/** Class that provides selections serially, returning the next successive selection in the list. */
template<class T> class SerialSelection : public Selection<T> {
public:
	typedef pair<const Selection<T> *, size_t> Item;	/**< A Selection and count. */
	typedef vector<Item> Selections;
	typedef vector<shared_ptr<Selection<T> > > ManagedItems;

	static const size_t INFINITE = static_cast<size_t>(-1);

private:
	Selections selections;								/**< Collection of possible selections. */
	LifecycleManager<Selection<T> > managedItems;		/**< Manages the lifecycle of objects when requested. */
	bool loop;											/**< Rather or not to rewind when the last selection is reached. */
	mutable size_t repitition;							/**< How many times the current item has been repeated. */
	mutable typename Selections::iterator next;	/**< The next item. */
	mutable const T *last;

public:
	SerialSelection(bool loop)
			: selections()
			, managedItems()
			, loop(loop)
			, repitition(0)
			, next(selections.end())
			, last(NULL) {
	}

	/**
	 * Adds the selection to the end of the selections vector.  The selection object is expected to
	 * remain valid for the lifespan of the SerialSelection object.
	 */
	void add(const Selection<T> &selection, size_t reps = 1) {
		// vector::push_back may cause a reallocation, which invalidates iterators
		size_t nextIndex = next - selections.begin();
		selections.push_back(typename Selections::value_type(&selection, reps));
		next = selections.begin() + nextIndex;
	}

	/** Add a selection who's lifecycle will be managed by this SerialSelection object. */
	void addManaged(Selection<T> *selection, size_t reps = 1) {
		add(*selection, reps);
		managedItems.add(selection);
	}

	/*
	void addSingle(const T &single, size_t reps = 1) {
		add(SingleSelection<T>(shared_ptr<T>(single)), reps);
	}

	void addManagedSingle(T *single, size_t reps = 1) {
		add(ManagedSingleSelection<T>(shared_ptr<T>(single)), reps);
	}
*/
	/** Returns true if this object is ready to make a selection. */
	bool isReady() const {
		return !selections.empty() && (next != selections.end() || loop);
	}

	/** Rewinds to the first repitition of the first element. */
	void reset() {
		next = selections.begin();
		repitition = 0;
	}

	/** Return the next selection in selection or NULL if there are no selections or if the end has been reached and looping is not specified. */
	const T *getNext() const {
		if(selections.empty()) {
			return NULL;
		}

		if(next == selections.end()) {
			if(!loop) {
				return NULL;
			} else {
				next == selections.begin();
			}
		}

		const T *ret = next->first->getNext();
		if(ret) {
			last = ret;
		}
		if(next->second != INFINITE && ++repitition == next->second) {
			++next;
			repitition = 0;
		}
		return ret;
	}

	const T *getLast() const	{return last;}
};

/**
 * A RandomSelection "grab bag" where each request for the next element returns a random item from
 * the internal list.
 *
 * Theory: Each item has a "chance" that is represented by a floating point value.  The chance
 * values do not need to add up to any particular number (100% or such).  The total of all chance
 * values are summed as items are added to the collection and stored in the totalRandomChance
 * data member.  When a call to getNext() is made, a random number is chosen between zero and
 * totalRandomChance and the resulting item is selected from the std::map and returned.
 */
template<class T> class RandomSelection : public Selection<T> {
protected:
	typedef map<float, pair<const Selection<T> *, float> > Selections;

	Selections selections;
	LifecycleManager<Selection<T> > managedItems;		/**< Manages the lifecycle of objects when requested. */
	float totalRandomRange;
	mutable Random random;
	mutable const T *last;

public:
	RandomSelection(int seed)
			: selections()
			, managedItems()
			, totalRandomRange(0.f)
			, random(seed) {
	}

	void add(const Selection<T> &selection, float chance = 1.f) {
		assert(chance > 0.f);
		selections[totalRandomRange] = typename Selections::mapped_type(&selection, chance);
		totalRandomRange += chance;
	}

	/** Add a selection who's lifecycle will be managed by this SerialSelection object. */
	void addManaged(Selection<T> *selection, float chance = 1.f) {
		add(*selection, chance);
		managedItems.add(selection);
	}

	virtual bool isReady() const {
		return !selections.empty();
	}

	virtual const T *getNext() const {
		float value = random.randRange(0.f, totalRandomRange);
		typename Selections::const_iterator i = selections.lower_bound(value);
		assert(i != selections.end()); // this should never happen here unless something is goofed

		const T *ret = i->second.first->getNext();
		if(ret) {
			last = ret;
		}
		return ret;
	}

	const T *getLast() const	{return last;}
};

#if 0
/**
 * A RandomSelection that prevents repititions based upon the supplied rule set.
 *
 * Theory: This specialization of RandomSelection adds the dimension of storing a history of items
 * that were previously selected.  The history is stored in a TemporalDataCollection-derived object
 * where the "data" is used to store the chance of the item being chosen and a max age & count
 * are managed by the TemporalDataCollection.
 *
 * When getNext() is called, a random number chosen and then adjusted using the History.
 * History::clean() is first called to remove out-dated items and then
 * History::getHistoryAdjustedValue() is called to adjust the value account for items that should
 * not be selected.  This could probably be optimized, but should serve its purpose for now,
 * operating efficiently on very large numbers of objects.
 */
template<class T> class RandomSelectionWithNoRepeat : public RandomSelection<T> {
public:
	class HistoryItem : public TemporalDataItem<float> {
	private:
		float key;
//		size_t index;

	public:
		HistoryItem(float chance, float key/*, size_t index*/)
				: TemporalDataItem<float>(chance)
				, key(key)
				/*, index(index) */{
		}

		float getKey() const	{return key;}
//		size_t getIndex() const	{return index;}
	};

	class History : public TemporalDataCollection<float, HistoryItem> {
	private:
		typedef map<float, HistoryItem> HistoryMap;
		HistoryMap historyMap;

	public:
		History(int64 maxAge, size_t maxCount)
				: TemporalDataCollection<float, HistoryItem>(maxAge, maxCount)
				, historyMap() {
		}

		float getAdjustedValue(float value) const {
			this->clean();
			foreach(const typename HistoryMap::value_type &item, historyMap) {
				if(value >= item.first) {
					// "data" in this case is the chance
					value += item.second.getData();
				} else {
					break;
				}
			}
			return value;
		}

	private:
		virtual void postAdd(const HistoryItem &item)	{historyMap[item.getKey()] = item;}
		virtual void prePop(const HistoryItem &item)	{historyMap.erase(item.getKey());}
	};

	static const size_t UNLIMITED_NO_REPEAT_COUNT = History::NO_SIZE_LIMIT;

private:
	mutable History history;

public:
	RandomSelectionWithNoRepeat(int seed, int64 noRepeatInterval, size_t noRepeatCount = UNLIMITED_NO_REPEAT_COUNT)
			: RandomSelection<T>(seed)
			, history(noRepeatInterval, noRepeatCount) {
	}

	bool isReady() const {
		assert(history.size() <= selections.size());
		history.clean();
		return selections.size() - history.size();
	}

	const T *getNext() const {
		if(!isReady()) {
			return NULL;
		}

		float value = history.getAdjustedValue(random.randRange(0.f, totalRandomRange));
		typename Selections::const_iterator i = selections.lower_bound(value);
		assert(i != selections.end()); // this should never happen here unless something is goofed
		history.add(HistoryItem(i->second->second, i->first));

		const T *ret = i->second->first->getNext();
		if(ret) {
			last = ret;
		}
		return ret;
	}
};
#endif
}} // end namespace

#endif

