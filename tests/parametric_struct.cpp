#include <gtest/gtest.h>

#include <parametric/core.hpp>


struct MyParms
{
    parametric::param<float> a{1., "a"};
    parametric::param<float> b{10, "b"};
    float c{100};
};

// overriding the default new_param allows to propagate changes
// to one parameter of the structure to the structure itself
template<>
parametric::param<MyParms> parametric::new_param(const MyParms& parms)
{
    return parametric::new_parametric_struct(parms,
                                             parms.a,
                                             parms.b
                                             );
}

float f(const MyParms& p)
{
    return p.a.value() + p.b.value() + p.c;
}

TEST(Struct, function)
{
    auto parms = MyParms{};

    // wrap into parametric structure
    auto pparms = parametric::new_param(parms);

    auto result = parametric::eval(f, pparms);
    EXPECT_FALSE(result.is_valid());
    EXPECT_EQ(111, result.value());
    EXPECT_TRUE(result.is_valid());

    // this make pparms invalid
    pparms.change_value().c = 1000;
    EXPECT_FALSE(result.is_valid());

    EXPECT_EQ(1011, result.value());
    EXPECT_TRUE(result.is_valid());

    // changing b automatically should make the structure
    // invalid as well. the invalidation is propagated from
    // members to the owning structure
    parms.b.set_value(500);
    EXPECT_FALSE(result.is_valid());

    EXPECT_EQ(1501, result.value());
    EXPECT_TRUE(result.is_valid());
}
