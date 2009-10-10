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

#include "pch.h"
#include "protocol_exception.h"
#include "leak_dumper.h"

namespace Game { namespace Net {

// ===============================
//  class ProtocolException
// ===============================

ProtocolException::ProtocolException(
		const RemoteInterface &sender,
		NetworkMessage *netMsg,
		const string &msg,
		const GlestException *rootCause,
		const string &fileName,
		long lineNumber) throw()
		: GlestException(buildFullMsg(sender, netMsg, msg).c_str(), NULL, rootCause, fileName,
						 lineNumber, 0, NULL)
		, sender(sender)
		, netMsg(netMsg) {
}

ProtocolException::~ProtocolException() throw() {
}

ProtocolException *ProtocolException::clone() const throw () {
	return new ProtocolException(*this);
}

string ProtocolException::getType() const throw() {
	return string("ProtocolException");
}

string ProtocolException::buildFullMsg(const RemoteInterface &sender,
		NetworkMessage *netMsg, const string &msg) {
	stringstream str;
	str << msg << endl;
	ObjectPrinter op(str);
	if(netMsg) {
		op.print("netMsg", (const Printable &)*netMsg);
	} else {
		str << "no NetworkMessage specified";
	}
	op.print("sender", (const Printable &)sender);

	return str.str();
}

__noreturn __cold void ProtocolException::coldThrow(
		const RemoteInterface &sender,
		NetworkMessage *netMsg,
		const string &msg,
		const GlestException *rootCause,
		const string &fileName,
		long lineNumber) {
	throw ProtocolException(sender, netMsg, msg, rootCause, fileName, lineNumber);
}

}} // end namespace
