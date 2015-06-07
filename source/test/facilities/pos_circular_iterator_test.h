#ifndef POSCIRCULARITERATORTEST_H
#define POSCIRCULARITERATORTEST_H


#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include "pos_iterator.h"

namespace Test {


class PosCircularIteratorTest : public CppUnit::TestFixture {
public:
    PosCircularIteratorTest();
    virtual ~PosCircularIteratorTest();


	static CppUnit::Test *suite();
	void setUp();
	void tearDown();

    void testRangeZeroToOne();
    void testRangeZeroToTwo();
    void testRangeOneToTwo();

    void testOrdering();

	void testBoundaryCases();
    void testNoDuplicates();
};

}

#endif // POSCIRCULARITERATORTEST_H


