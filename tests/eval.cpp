#include <gtest/gtest.h>

#include <parametric/core.hpp>
#include <parametric/operators.hpp>

double add(double v1, double v2) {
    return v1 + v2;
}


TEST(Eval, nonParametric)
{

    auto v1 = parametric::new_param(3.0);
    auto v2 = parametric::new_param(7.0);

    auto v3 = add(v1, v2);
    EXPECT_NEAR(10.0, v3, 1e-12);

    v2 = -3;
    EXPECT_NEAR(10.0, v3, 1e-12);
}

TEST(Eval, parametric)
{

    auto v1 = parametric::new_param(3.0);
    auto v2 = parametric::new_param(7.0);

    auto v3 = parametric::eval(add, v1, v2);
    EXPECT_NEAR(10.0, v3, 1e-12);

    v2 = -3;
    EXPECT_NEAR(0.0, v3, 1e-12);
}

// This test will fail, if parametric/operators.hpp is NOT included
TEST(Eval, operators)
{
    auto v1 = parametric::new_param(3.0);
    auto v2 = parametric::new_param(7.0);

    auto v3 = v1 + v2;
    EXPECT_NEAR(10.0, v3, 1e-12);

    v2 = -3;
    EXPECT_NEAR(0.0, v3, 1e-12);

    v3 = v1 * v2;

    EXPECT_NEAR(-9.0, v3, 1e-12);

    v1 = -10.0;
    EXPECT_NEAR(30.0, v3, 1e-12);
}

