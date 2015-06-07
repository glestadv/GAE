// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <cppunit/extensions/HelperMacros.h>
#include "pos_circular_iterator_test.h"

#include "leak_dumper.h"

using Shared::Math::Vec2i;
using Shared::Math::Rect2i;

using Glest::Util::PosCircularIteratorOrdered;
using Glest::Util::PosCircularIteratorFactory;


namespace Test {

#define ADD_TEST(Class, Method) suiteOfTests->addTest( \
	new CppUnit::TestCaller<Class>(#Method, &Class::Method));

PosCircularIteratorTest::PosCircularIteratorTest() {
}

PosCircularIteratorTest::~PosCircularIteratorTest() {
}

CppUnit::Test *PosCircularIteratorTest::suite() {
	CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("ReverseRectIteratorTest");
	ADD_TEST(PosCircularIteratorTest, testRangeOne);
	ADD_TEST(PosCircularIteratorTest, testBoundaryCases);
	ADD_TEST(PosCircularIteratorTest, testNoDuplicates);
	return suiteOfTests;
}

PosCircularIteratorFactory *factory;

void PosCircularIteratorTest::setUp() {
    factory = new PosCircularIteratorFactory(64);
}

void PosCircularIteratorTest::tearDown() {
    delete factory;
    factory = nullptr;
}

void PosCircularIteratorTest::testBoundaryCases() {
    Rect2i bounds(Vec2i(0,0), Vec2i(128,128));
    Vec2i p; // scrath var

    cout << "\nTesting PosCircularIterorOrdered - boundary cases...\n";
    cout << "Case: Top-left.\n";
    PosCircularIteratorOrdered pcio(bounds, Vec2i(0,0), factory->getInsideOutIterator(1, 64));
    int i = 0;
    while (pcio.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
    cout << "Case: Top-right.\n";
    pcio = PosCircularIteratorOrdered(bounds, Vec2i(128,0), factory->getInsideOutIterator(1, 64));
    i = 0;
    while (pcio.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
    cout << "Case: Bottom-right.\n";
    pcio = PosCircularIteratorOrdered(bounds, Vec2i(128,128), factory->getInsideOutIterator(1, 64));
    i = 0;
    while (pcio.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
    cout << "Case: Bottom-left.\n";
    pcio = PosCircularIteratorOrdered(bounds, Vec2i(0,128), factory->getInsideOutIterator(1, 64));
    i = 0;
    while (pcio.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
}

void PosCircularIteratorTest::testRangeOne() {
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0,0), Vec2i(128,128)), Vec2i(2,2), factory->getInsideOutIterator(0, 1));
    Vec2i p;
    cout << "\nTesting PosCircularIterorOrdered - range 1...\n";
    CPPUNIT_ASSERT(pcio.getNext(p));
    CPPUNIT_ASSERT(p == Vec2i(2, 2));
    cout << p;
    while (pcio.getNext(p)) {
        cout << ", " << p;
    }
    cout << "\n";
}

void PosCircularIteratorTest::testRangeTwo() {
}


void PosCircularIteratorTest::testNoDuplicates() {
	PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0,0), Vec2i(128,128)), Vec2i(31,64), factory->getInsideOutIterator(1, 64));

    cout << "\nTesting PosCircularIterorOrdered - no duplicate positions...\n";
    Vec2i p;
    std::set<Vec2i> seen;
    for (int i=0; i < 32; ++i) {
        if (pcio.getNext(p)) {
            seen.insert(p);
            cout << p << ", ";
            CPPUNIT_ASSERT(seen.find(p) == seen.end());
            if (i % 8 == 7) cout << "\n";
        }
    }
    cout << "\n\n";

}


} // end namespace
