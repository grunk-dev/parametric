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

TEST(Eval, writeAccess)
{
    struct XY {
        XY(int x, int y) : m_x(x), m_y(y) {}

        int X() const {return m_x;}
        int Y() const {return m_y;}
        void SetX(int x) {m_x = x;}

    private:
        int m_x, m_y;
    };

    auto xy_sum = [](const XY& xy) {
        std::cout << "sum" << std::endl;
        return xy.X() + xy.Y();
    };

    auto xy = parametric::new_param(XY(5, 3));
    auto sum = parametric::eval(xy_sum, xy);

    EXPECT_EQ(8, sum);

    xy.Value().SetX(10);
    EXPECT_EQ(13, sum);

    // create invalid param, accessing the value is therefore not possible
    parametric::param<XY> xy2("xy2");
    EXPECT_THROW(xy2.Value().SetX(10), std::runtime_error);
}

