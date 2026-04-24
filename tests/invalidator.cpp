// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
// SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>


#include <parametric/core.hpp>
#include <parametric/adaptors.hpp>


struct MyParms
{
    parametric::param<float> a{"a", 1.};
    parametric::param<float> b{"b", 2.};
};

/**
 * This class uses some caching mechanism to
 * only compute once the result.
 *
 * Then the parameters parms change, the invalidation
 * had to be called manually.
 *
 * Using the invalidator, this now can be performned automatically
 */
class LegacyClass
{
public:
    LegacyClass(const MyParms& parms)
        : parms(parms)
    {
    }

    /** This adds the automatic invalidation functionality */
    void setup_invalidator()
    {
        invalidator.set_invalidation_function([this]() {
            invalidate_result();
        });
        invalidator.on(parms.a);
        invalidator.on(parms.b);
    }

    void invalidate_result()
    {
        my_result.reset();
    }

    float get_result() const
    {
        if (!my_result) {
            my_result = parms.a.value() + parms.b.value();
        }

        return my_result.value();
    }

private:
    mutable std::optional<float> my_result;

public:
    const MyParms& parms;
    parametric::adaptors::Invalidator invalidator;

};

class InvaldatorTest : public testing::Test
{
public:
    MyParms parms;
    LegacyClass cls{parms};
};


TEST_F(InvaldatorTest, checkFunction)
{
    cls.setup_invalidator();

    EXPECT_EQ(cls.get_result(), 3.);

    parms.a.set_value(2);
    EXPECT_EQ(cls.get_result(), 4.);

    parms.a.set_value(3);
    EXPECT_EQ(cls.get_result(), 5.);
}

TEST_F(InvaldatorTest, invalidatorNotSet)
{
    EXPECT_EQ(cls.get_result(), 3.);

    // changing the value does not
    // trigger recomputation
    parms.a.set_value(2);
    EXPECT_EQ(cls.get_result(), 3.);
}

#ifndef NDEBUG
TEST_F(InvaldatorTest, wrongSetup)
{
    // this line must triggers an assert in debug mode
    EXPECT_DEATH(cls.invalidator.on(parms.a), "The invalidation function must be set before connecting parameters");
}
#endif

TEST_F(InvaldatorTest, checkInvalidations)
{
    int counter = 0;

    auto incCounter = [&counter]() {
        counter++;
    };

    cls.invalidator.set_invalidation_function(incCounter);

    EXPECT_EQ(0, counter);
    parms.a.set_value(10);

    // must be still zero, still a is not yet connected
    EXPECT_EQ(0, counter);

    cls.invalidator.on(parms.a);
    EXPECT_EQ(0, counter);

    parms.a.set_value(11);
    EXPECT_EQ(1, counter);

    parms.b.set_value(12);
    EXPECT_EQ(1, counter);

    cls.invalidator.on(parms.b);
    EXPECT_EQ(1, counter);

    parms.b.set_value(13);
    EXPECT_EQ(2, counter);
}

