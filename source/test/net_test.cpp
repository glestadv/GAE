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

#include "pch.h"
#include <cppunit/extensions/HelperMacros.h>
#include "net_test.h"

#include "leak_dumper.h"

namespace Test {

NetTest::NetTest() : s(NULL), c1(NULL), c2(NULL) {}

NetTest::~NetTest() {
	if(c2) {
		delete c2;
	}
	if(c1) {
		delete c1;
	}
	if(s) {
		delete s;
	}
}

CppUnit::Test *NetTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("NetTest");
	suiteOfTests->addTest(new CppUnit::TestCaller<NetTest>(
			"testConversation",
			&NetTest::testConversation));
	return suiteOfTests;
}

void NetTest::setUp() {
}

void NetTest::tearDown() {
}

void NetTest::testConversation() {
	// Create server
	s = new ServerInterface(65432);
	CPPUNIT_ASSERT(s->getState() == STATE_LISTENING);

	// Create client 1
	c1 = new ClientInterface(65431);
	CPPUNIT_ASSERT(c1->getState() == STATE_LISTENING);

	// connect to server
	c1->connectToServer(IpAddress("192.168.1.4"), 65432);
	
	bool loop = true;
	while(loop) sleep(1000);
	
//	c2 = new ClientInterface(65430);
//	CPPUNIT_ASSERT(c2->getState() == STATE_LISTENING);
}

} // end namespace
