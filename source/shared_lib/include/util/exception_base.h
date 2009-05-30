// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_EXCEPTIONBASE_H_
#define _SHARED_UTIL_EXCEPTIONBASE_H_

#include <exception>
#include <sstream>
#include <string>
#include <errno.h>
#include <map>

#include "util.h"

using std::runtime_error;
using std::stringstream;
using std::string;
using Shared::Util::newStrOrNull;
using Shared::Util::Cloneable;

#define __STRIZE2(x) #x
#define __STRIZE(x) __STRIZE2(x)
#define __FILE_SPOT __FILE__ "(" __STRIZE(__LINE__) "): "

namespace Shared { namespace Util {

// ===============================
//  class GlestException
// ===============================

/** Base exception type */
class GlestException : public runtime_error, public Cloneable {
public:
	typedef std::map<std::string, std::string> ValueMap;
	typedef std::map<std::string, Shared::Util::Printable *> PrintableMap;

private:
	string msg;
	string operation;
	shared_ptr<GlestException> rootCause;
	string fileName;
	int lineNumber;
	int err;
	string errDescr;
	string whatStr;

public:
	GlestException(const string &msg) throw();
	GlestException(
			const string &msg,
			const string &operation,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0,
			int err = 0,
			const string &errDescr = "") throw();
	virtual ~GlestException() throw();

	const string &getOperation() const throw()			{return operation;}
	const GlestException *getRootCause() const throw()	{return rootCause.get();}
	const string &getFileName() const throw()			{return fileName;}
	int getLineNumber() const throw()					{return lineNumber;}
	int getErr() const throw()							{return err;}
	const string &getErrDescr() const throw()			{return errDescr;}

	virtual const char* what() const throw();
	virtual string getType() const throw();
	virtual GlestException *clone() const throw();
};

}} // end namespace

#endif // _SHARED_UTIL_EXCEPTIONBASE_H_
