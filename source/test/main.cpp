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
#include <cppunit/ui/text/TestRunner.h>

#include <iostream>

#include "reverse_rect_iter_test.h"
#include "node_pool_test.h"
#include "influence_map_test.h"

#include "leak_dumper.h"

namespace Test {
} // end namespace

using namespace Test;

int main(int argc, char **argv) {
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(Test::ReverseRectIteratorTest::suite());
	runner.addTest(Test::NodePoolTest::suite());
	runner.addTest(Test::InfluenceMapTest::suite());

	runner.run();

	char line[256];
	std::cout << "[Enter] to exit.";
	std::cin.getline(line,256);
	return 0;
}


