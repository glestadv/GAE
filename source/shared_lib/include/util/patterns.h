// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

/**
 * @file patterns.h
 * Contains utility classes for common design patterns and motifs.
 */

#ifndef _SHARED_UTIL_PATTERNS_H_
#define _SHARED_UTIL_PATTERNS_H_

#include "types.h"
#include "lang_features.h"

using Shared::Platform::int64;

namespace Shared { namespace Util {

// =====================================================
// class Cloneable
// =====================================================

/**
 * A cloneable object can be cloned without knowing the derived type.  The implementing class
 * should generally call its copy constructor.
 */
class Cloneable {
public:
	/**
	 * @return a copy of this object (analagous to a virtual copy constructor, which C++ does not
	 *		   directly support).
	 */
	virtual Cloneable *clone() const = 0;
};

// =====================================================
// class Uncopyable
// =====================================================

/**
 * <p>An Uncopyable has no default copy constructor of operator=.  A subclass may derive from
 * Uncopyable at any level of visibility, even private, and subclasses will not have a default copy
 * constructor or operator=.  If boost is ever integrated into GAE, I suggest this class be replaced
 * with a typedef:</p>
 * <pre>typedef boost::noncopyable Uncopyable;</pre>
 */
class Uncopyable {
protected:
	Uncopyable() {}
	~Uncopyable() {}

private:
	Uncopyable(const Uncopyable&) DELETE_FUNC;
	const Uncopyable &operator=(const Uncopyable&) DELETE_FUNC;
};

/**
 * Defines a type that has an update method intended to be called when needed to update the object.
 */
class Updateable {
public:
	virtual void update() = 0;
};

/**
 * Extends on Updateable adding the concept of the type being able to tell when it needs an update
 * again so that more precise CPU management can be achieved while keeping the type properly
 * updated.
 */
class Scheduleable {
private:
	/** Time last updated in microseconds. */
	int64 lastExecution;
	int64 nextExecution;

public:
	Scheduleable(int64 nextExecution, int64 lastExecution = 0)
			: lastExecution(lastExecution)
			, nextExecution(nextExecution) {
	}

	int64 getLastExecution() const	{return lastExecution;}
	int64 getNextExecution() const	{return nextExecution;}

protected:
	void setLastExecution(int64 v)	{lastExecution = v;}
	void setNextExecution(int64 v)	{nextExecution = v;}
};

}}//end namespace

using Shared::Util::Cloneable;
using Shared::Util::Uncopyable;
using Shared::Util::Updateable;
using Shared::Util::Scheduleable;

#endif // _SHARED_UTIL_PATTERNS_H_
