// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_NET_PROTOCOLEXCEPTION_H_
#define _GAME_NET_PROTOCOLEXCEPTION_H_

#include "exception_base.h"
#include "remote_interface.h"
#include "network_message.h"

namespace Game { namespace Net {

// =====================================================
//	class ProtocolException
// =====================================================

class ProtocolException : public GlestException {
private:
	const RemoteInterface &sender;
	shared_ptr<NetworkMessage> netMsg;

public:
	ProtocolException(
			const RemoteInterface &sender,
			NetworkMessage *netMsg,
			const string &msg,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0) throw();
	virtual ~ProtocolException() throw ();
	virtual ProtocolException *clone() const throw ();

	virtual string getType() const throw();

	/** Convenience method to throw a Protocol exception as a cold function call. */
	static __noreturn __cold void coldThrow(
			const RemoteInterface &sender,
			NetworkMessage *netMsg,
			const string &msg,
			const GlestException *rootCause = NULL,
			const string &fileName = "",
			long lineNumber = 0);

private:
	static string buildFullMsg(const RemoteInterface &sender, NetworkMessage *netMsg,
			const string &msg);
};

}} // end namespace

#endif // _GAME_NET_PROTOCOLEXCEPTION_H_
