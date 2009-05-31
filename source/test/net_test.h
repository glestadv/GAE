// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _TEST_NET_H_
#define _TEST_NET_H_

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "game/network/client_interface.h"
#include "game/network/server_interface.h"

using namespace Game::Net;
using Shared::Platform::Socket;
using Shared::Platform::ClientSocket;
using Shared::Platform::ServerSocket;

namespace Test {

// =====================================================
//	class NetTest
// =====================================================

class NetTest : public CppUnit::TestFixture {
private:
	ServerInterface *s;
	ClientInterface *c1;
	ClientInterface *c2;

public:
	NetTest();
	~NetTest();

	static CppUnit::Test *suite();
	void setUp();
	void tearDown();
	
	void testConversation();
};

} // end namespace

#endif // _TEST_NET_H_
