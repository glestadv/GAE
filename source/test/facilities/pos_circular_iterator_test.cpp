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
	ADD_TEST(PosCircularIteratorTest, testRangeZeroToOne);
	ADD_TEST(PosCircularIteratorTest, testRangeZeroToTwo);
	//ADD_TEST(PosCircularIteratorTest, testBoundaryCases);
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
    //pcio.~PosCircularIteratorOrdered();
    pcio.init(bounds, Vec2i(128,0), factory->getInsideOutIterator(1, 64));
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
    pcio.init(bounds, Vec2i(128,128), factory->getInsideOutIterator(1, 64));
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
    pcio.init(bounds, Vec2i(0,128), factory->getInsideOutIterator(1, 64));
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

void printGridLines(int width) {
    cout << "+";
    for (int i=0; i < width; ++i) {
        cout << "-+";
    }
    cout << "\n";
}

void printMap(int width, int height, std::set<Vec2i> &seen) {
    for (int y=0; y < height; ++y) {
        printGridLines(width);
        cout << "|";
        for (int x=0; x < width; ++x) {
            if (seen.find(Vec2i(x, y)) != seen.end()) {
                cout << "#";
            } else {
                cout << " ";
            }
            cout << "|";
        }
        cout << "\n";
    }
    printGridLines(width);
}

void printList(const std::vector<Vec2i> &_list) {
    for (std::vector<Vec2i>::const_iterator it = _list.cbegin(); it != _list.cend(); ++it) {
        cout << *it;
        if (it + 1 != _list.end()) cout << ", ";
    }
    cout << endl;
}

void PosCircularIteratorTest::testRangeZeroToOne() {
    const int width = 16;
    const int height = 16;
    const int cx = width / 2;
    const int cy = height / 2;
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0, 0), Vec2i(width, height)), Vec2i(cx, cy), factory->getInsideOutIterator(0, 1));
    Vec2i p;

    cout << "\nTesting PosCircularIterorOrdered - range 1...\n";
    CPPUNIT_ASSERT(pcio.getNext(p));
    CPPUNIT_ASSERT(p == Vec2i(cx, cy));
    std::set<Vec2i> seen;
    std::vector<Vec2i> _list;
    do {
        seen.insert(p);
        _list.push_back(p);
    } while (pcio.getNext(p));

    printMap(width, height, seen);
    printList(_list);

    Vec2i knownPositions[3 * 3] {
        Vec2i(cx - 1, cy - 1), Vec2i(cx - 1, cy + 0), Vec2i(cx - 1, cy + 1),
        Vec2i(cx + 0, cy - 1), Vec2i(cx + 0, cy + 0), Vec2i(cx + 0, cy + 1),
        Vec2i(cx + 1, cy - 1), Vec2i(cx + 1, cy + 0), Vec2i(cx + 1, cy + 1)
    };

    for (int i=0; i < 9; ++i) {
        CPPUNIT_ASSERT(seen.find(knownPositions[i]) != seen.end());
    }
}

void PosCircularIteratorTest::testRangeZeroToTwo() {
    const int width = 16;
    const int height = 16;
    const int cx = width / 2;
    const int cy = height / 2;
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0, 0), Vec2i(width, height)), Vec2i(cx, cy), factory->getInsideOutIterator(0, 2));
    Vec2i p;

    cout << "\nTesting PosCircularIterorOrdered - range 2...\n";
    CPPUNIT_ASSERT(pcio.getNext(p));
    CPPUNIT_ASSERT(p == Vec2i(cx, cy));
    std::set<Vec2i> seen;
    std::vector<Vec2i> _list;
    do {
        seen.insert(p);
        _list.push_back(p);
    } while (pcio.getNext(p));

    printMap(width, height, seen);
    printList(_list);

    Vec2i knownPositions[5 * 5] {
        Vec2i(cx - 2, cy - 2), Vec2i(cx - 2, cy - 1), Vec2i(cx - 2, cy + 0), Vec2i(cx - 2, cy + 1), Vec2i(cx - 2, cy + 2),
        Vec2i(cx - 1, cy - 2), Vec2i(cx - 1, cy - 1), Vec2i(cx - 1, cy + 0), Vec2i(cx - 1, cy + 1), Vec2i(cx - 1, cy + 2),
        Vec2i(cx + 0, cy - 2), Vec2i(cx + 0, cy - 1), Vec2i(cx + 0, cy + 0), Vec2i(cx + 0, cy + 1), Vec2i(cx + 0, cy + 2),
        Vec2i(cx + 1, cy - 2), Vec2i(cx + 1, cy - 1), Vec2i(cx + 1, cy + 0), Vec2i(cx + 1, cy + 1), Vec2i(cx + 1, cy + 2),
        Vec2i(cx + 2, cy - 2), Vec2i(cx + 2, cy - 1), Vec2i(cx + 2, cy + 0), Vec2i(cx + 2, cy + 1), Vec2i(cx + 2, cy + 2)
    };
    for (int i=0; i < 25; ++i) {
        CPPUNIT_ASSERT(seen.find(knownPositions[i]) != seen.end());
    }
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
