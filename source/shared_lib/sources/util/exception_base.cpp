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


#include "pch.h"
#include "exception_base.h"
#include "leak_dumper.h"

using std::endl;

namespace Shared { namespace Util {

// ===============================
//  class GlestException
// ===============================

const char* GlestException::what() const throw() {
	if(whatStr.empty()) {
		stringstream str;
		str << msg << endl
						<< "  type       : " << getType();
		if(!operation.empty()) {
			str << endl << "  operation  : " << operation;
		}
		if(!errDescr.empty()) {
			str << endl << "  error code : " << err
				<< endl << "  descripton : " << errDescr;
		}
		if(rootCause) {
			str << endl << "  root cause : " << rootCause->what();
		}
		if(!fileName.empty()) {
			str << endl << "  location   : " << fileName << ":" << lineNumber;
		}
		const_cast<GlestException*>(this)->whatStr = str.str();
	}
	return whatStr.c_str();
}

GlestException::GlestException(const string &msg) throw ()
		: runtime_error("")
		, msg(msg)
		, operation()
		, rootCause()
		, fileName()
		, lineNumber(0)
		, err(0)
		, errDescr()
		, whatStr() {
}

GlestException::GlestException(
		const string &msg,
		const string &operation,
		const GlestException *rootCause,
		const string &fileName,
		long lineNumber,
		int err,
		const string &errDescr) throw()
		: runtime_error("")
		, msg(msg)
		, operation(operation)
		, rootCause(rootCause ? rootCause->clone() : NULL)
		, fileName(fileName)
		, lineNumber(lineNumber)
		, err(err)
		, errDescr(errDescr)
		, whatStr() {
}

GlestException::~GlestException() throw() {
}

string GlestException::getType() const throw() {
	return "GlestException";
}

GlestException *GlestException::clone() const throw () {
	return new GlestException(*this);
}

}} // end namespace
