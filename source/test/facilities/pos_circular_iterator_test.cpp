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

#include <iomanip>

using Shared::Math::Vec2i;
using Shared::Math::Rect2i;

using Glest::Util::PosCircularIteratorOrdered;
using Glest::Util::PosCircularIteratorFactory;
using Shared::Math::fixed;

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
	ADD_TEST(PosCircularIteratorTest, testRangeOneToTwo);
	ADD_TEST(PosCircularIteratorTest, testOrdering);
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
    //pcio.~PosCircularIteratorOrdered();
	PosCircularIteratorOrdered pcio2(bounds, Vec2i(128,0), factory->getInsideOutIterator(1, 64));
    i = 0;
    while (pcio2.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
    cout << "Case: Bottom-right.\n";
    PosCircularIteratorOrdered pcio3(bounds, Vec2i(128,128), factory->getInsideOutIterator(1, 64));
    i = 0;
    while (pcio3.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
    cout << "Case: Bottom-left.\n";
    PosCircularIteratorOrdered pcio4(bounds, Vec2i(0,128), factory->getInsideOutIterator(1, 64));
    i = 0;
    while (pcio4.getNext(p)) {
        //cout << p << ", ";
        if (i % 16 == 15) cout << ".";
        CPPUNIT_ASSERT(bounds.isInside(p));
        //if (i % 8 == 7) cout << "\n";
        ++i;
    }
    cout << "\n";
}

void printGridLines(int map_width, int grid_width = 1) {
    cout << "+";
    for (int i=0; i < map_width; ++i) {
        for (int j=0; j < grid_width; ++j) {
            cout << "-";
        }
        cout << "+";
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

void printMap(int width, int height, std::map<Vec2i, int> &things) {
    for (int y=0; y < height; ++y) {
        printGridLines(width, 2);
        cout << "|";
        for (int x=0; x < width; ++x) {
            std::map<Vec2i, int>::iterator it = things.find(Vec2i(x, y));
            if (it != things.end()) {
                cout << std::setw(2) << std::setfill('0') << it->second;
            } else {
                cout << "  ";
            }
            cout << "|";
        }
        cout << "\n";
    }
    printGridLines(width, 2);
}

void printList(const std::vector<Vec2i> &_list) {
    for (std::vector<Vec2i>::const_iterator it = _list.cbegin(); it != _list.cend(); ++it) {
        cout << *it;
        if (it + 1 != _list.end()) cout << ", ";
    }
    cout << endl;
}

void PosCircularIteratorTest::testRangeZeroToOne() {
    const int width = 9;
    const int height = 9;
    const int cx = width / 2;
    const int cy = height / 2;
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0, 0), Vec2i(width, height)), Vec2i(cx, cy), factory->getInsideOutIterator(0, 1));
    Vec2i p;

    cout << "\nTesting PosCircularIterorOrdered - range [0,1]...\n";
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

    Vec2i knownPositions[3 * 3] = {
        Vec2i(cx - 1, cy - 1), Vec2i(cx - 1, cy + 0), Vec2i(cx - 1, cy + 1),
        Vec2i(cx + 0, cy - 1), Vec2i(cx + 0, cy + 0), Vec2i(cx + 0, cy + 1),
        Vec2i(cx + 1, cy - 1), Vec2i(cx + 1, cy + 0), Vec2i(cx + 1, cy + 1)
    };

    for (int i=0; i < 9; ++i) {
        CPPUNIT_ASSERT(seen.find(knownPositions[i]) != seen.end());
    }
}

void PosCircularIteratorTest::testRangeZeroToTwo() {
    const int width = 9;
    const int height = 9;
    const int cx = width / 2;
    const int cy = height / 2;
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0, 0), Vec2i(width, height)), Vec2i(cx, cy), factory->getInsideOutIterator(0, 2));
    Vec2i p;

    cout << "\nTesting PosCircularIterorOrdered - range [0,2]...\n";
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

    Vec2i knownPositions[5 * 5] = {
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


void PosCircularIteratorTest::testRangeOneToTwo() {
    const int width = 9;
    const int height = 9;
    const int cx = width / 2;
    const int cy = height / 2;
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0, 0), Vec2i(width, height)), Vec2i(cx, cy), factory->getInsideOutIterator(1, 2));
    Vec2i p;

    cout << "\nTesting PosCircularIterorOrdered - range [1,2]...\n";
    CPPUNIT_ASSERT(pcio.getNext(p));
    //CPPUNIT_ASSERT(p == Vec2i(cx, cy));
    std::set<Vec2i> seen;
    std::vector<Vec2i> _list;
    do {
        seen.insert(p);
        _list.push_back(p);
    } while (pcio.getNext(p));

    printMap(width, height, seen);
    printList(_list);

    Vec2i knownPositions[5 * 5 - 1] = {
        Vec2i(cx - 2, cy - 2), Vec2i(cx - 2, cy - 1), Vec2i(cx - 2, cy + 0), Vec2i(cx - 2, cy + 1), Vec2i(cx - 2, cy + 2),
        Vec2i(cx - 1, cy - 2), Vec2i(cx - 1, cy - 1), Vec2i(cx - 1, cy + 0), Vec2i(cx - 1, cy + 1), Vec2i(cx - 1, cy + 2),
        Vec2i(cx + 0, cy - 2), Vec2i(cx + 0, cy - 1),                        Vec2i(cx + 0, cy + 1), Vec2i(cx + 0, cy + 2),
        Vec2i(cx + 1, cy - 2), Vec2i(cx + 1, cy - 1), Vec2i(cx + 1, cy + 0), Vec2i(cx + 1, cy + 1), Vec2i(cx + 1, cy + 2),
        Vec2i(cx + 2, cy - 2), Vec2i(cx + 2, cy - 1), Vec2i(cx + 2, cy + 0), Vec2i(cx + 2, cy + 1), Vec2i(cx + 2, cy + 2)
    };
    for (int i=0; i < 24; ++i) {
        CPPUNIT_ASSERT(seen.find(knownPositions[i]) != seen.end());
    }
}

void PosCircularIteratorTest::testOrdering() {
   const int width = 9;
    const int height = 9;
    const int cx = width / 2;
    const int cy = height / 2;
    PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0, 0), Vec2i(width, height)), Vec2i(cx, cy), factory->getInsideOutIterator(1, 2));
    Vec2i p;
    fixed d;

    cout << "\nTesting PosCircularIterorOrdered - distance ordering...\n";
    CPPUNIT_ASSERT(pcio.getNext(p, d));
    std::map<Vec2i, fixed> distMap;
    std::map<Vec2i, int> orderMap;
    std::vector<Vec2i> _list;
    do {
        distMap.insert(std::make_pair(p, d));
        orderMap.insert(std::make_pair(p, _list.size()));
        _list.push_back(p);
    } while (pcio.getNext(p));

    printMap(width, height, orderMap);
    printList(_list);

    d = 0;
    for (std::vector<Vec2i>::iterator it = _list.begin(); it != _list.end(); ++it) {
        CPPUNIT_ASSERT(distMap[*it] >= d);
        d = distMap[*it];
    }
}

void PosCircularIteratorTest::testNoDuplicates() {
    cout << "\nTesting PosCircularIterorOrdered - no duplicate positions...\n";
	PosCircularIteratorOrdered pcio(Rect2i(Vec2i(0,0), Vec2i(128,128)), Vec2i(31,64), factory->getInsideOutIterator(1, 64));
    Vec2i p;
    std::set<Vec2i> seen;
    std::vector<Vec2i> _list;
    while (pcio.getNext(p)) {
        if (seen.find(p) != seen.end()) {
            cout << "Testing range [1,64] : Duplicate pos: " << p << endl;
        }
        CPPUNIT_ASSERT(seen.find(p) == seen.end());
        seen.insert(p);
        _list.push_back(p);
    }
    cout << "Testing range [1,64] : Ok." << endl;

    PosCircularIteratorOrdered pcio2(Rect2i(Vec2i(0,0), Vec2i(128,128)), Vec2i(31,64), factory->getInsideOutIterator(0, 16));
    seen.clear();
    _list.clear();
    while (pcio2.getNext(p)) {
        if (seen.find(p) != seen.end()) {
            cout << "Testing range [0,16] : Duplicate pos: " << p << endl;
        }
        CPPUNIT_ASSERT(seen.find(p) == seen.end());
        seen.insert(p);
        _list.push_back(p);
    }
    cout << "Testing range [0,16] : Ok." << endl;

}


} // end namespace
