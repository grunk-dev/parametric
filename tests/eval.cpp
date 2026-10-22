// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
// SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

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
        return xy.X() + xy.Y();
    };

    auto xy = parametric::new_param(XY(5, 3));
    auto sum = parametric::eval(xy_sum, xy);

    EXPECT_EQ(8, sum);

    xy.change_value().SetX(10);
    EXPECT_EQ(13, sum);

    // create invalid param, accessing the value is therefore not possible
    parametric::param<XY> xy2("xy2");
    EXPECT_THROW(xy2.change_value().SetX(10), std::runtime_error);
}

TEST(Eval, void_function)
{
    double x = 0.;
    auto i = parametric::new_param(42.);
    auto o = parametric::eval(
        [&x](double o){ x=o; }, // this is a void lambda
        i
    );
    EXPECT_EQ(x, 0.);
    o->eval();
    EXPECT_EQ(x, 42.);
}

namespace {
    struct Foo {
        Foo(double v) : bar(v) {}
        double bar;
    };
}

TEST(Eval, member_access) {
    auto i = parametric::new_param(Foo(42.));
    auto o = parametric::eval(&Foo::bar, i);
    static_assert(std::is_same_v<decltype(o), parametric::param<double const&>>);
    EXPECT_EQ(o, 42.);
}

// Replacing a parameter. See https://github.com/grunk-dev/parametric/issues/22
TEST(Eval, swapParam)
{
    auto a  = parametric::new_param(3.0);
    auto b  = parametric::new_param(7.0);
    auto a2 = parametric::new_param(100.0);

    auto c = parametric::eval(add, a, b);
    auto d = parametric::eval(add, a, c); // a has two consumers
    EXPECT_NEAR(10.0, c, 1e-12);
    EXPECT_NEAR(13.0, d, 1e-12);

    parametric::swap_param(a, a2);

    // results are recomputed using the new parameter
    EXPECT_NEAR(107.0, c, 1e-12);
    EXPECT_NEAR(207.0, d, 1e-12);

    // the new parameter drives the results ...
    a2 = 1.0;
    EXPECT_NEAR(8.0, c, 1e-12);
    EXPECT_NEAR(9.0, d, 1e-12);

    // ... the old one does not anymore
    a = 1000.0;
    EXPECT_NEAR(8.0, c, 1e-12);
    EXPECT_NEAR(9.0, d, 1e-12);
    EXPECT_NEAR(1000.0, a, 1e-12);
}

TEST(Eval, swapParamInvalidatesResult)
{
    auto a  = parametric::new_param(1.0);
    auto b  = parametric::new_param(2.0);
    auto a2 = parametric::new_param(10.0);

    auto c = parametric::eval(add, a, b);
    EXPECT_NEAR(3.0, c, 1e-12);
    EXPECT_TRUE(c.is_valid());

    parametric::swap_param(a, a2);
    EXPECT_FALSE(c.is_valid());
    EXPECT_NEAR(12.0, c, 1e-12);
}

TEST(Eval, swapParamDuplicateArgument)
{
    auto a  = parametric::new_param(3.0);
    auto a2 = parametric::new_param(5.0);

    auto c = parametric::eval(add, a, a); // c = a + a
    EXPECT_NEAR(6.0, c, 1e-12);

    parametric::swap_param(a, a2);
    EXPECT_NEAR(10.0, c, 1e-12);
}

TEST(Eval, swapParamOldParamDestroyed)
{
    auto a2 = parametric::new_param(5.0);
    auto b  = parametric::new_param(1.0);
    auto c  = [&]() {
        auto a = parametric::new_param(3.0);
        auto c = parametric::eval(add, a, b);
        parametric::swap_param(a, a2);
        return c;
    }();

    EXPECT_NEAR(6.0, c, 1e-12);
    a2 = 6.0;
    EXPECT_NEAR(7.0, c, 1e-12);
}

TEST(Eval, swapParamCycle)
{
    auto a = parametric::new_param(1.0);
    auto b = parametric::new_param(2.0);
    auto c = parametric::eval(add, a, b);

    // c depends on a, so it cannot replace a
    EXPECT_THROW(parametric::swap_param(a, c), std::runtime_error);

    // the graph still works
    a = 5.0;
    EXPECT_NEAR(7.0, c, 1e-12);
}

TEST(Eval, swapParamResultOfComputation)
{
    auto a = parametric::new_param(1.0);
    auto b = parametric::new_param(2.0);
    auto s = parametric::eval(add, a, b);   // 3
    auto t = parametric::new_param(10.0);
    auto u = parametric::eval(add, s, t);   // 13

    // replace a computed parameter by an independent one
    auto s2 = parametric::new_param(100.0);
    parametric::swap_param(s, s2);
    EXPECT_NEAR(110.0, u, 1e-12);

    // s is still a valid, working parameter
    a = 4.0;
    EXPECT_NEAR(6.0, s, 1e-12);
    EXPECT_NEAR(110.0, u, 1e-12);
}

namespace {
class SumDiff : public parametric::ComputeNode<
    SumDiff,
    parametric::Results<double, double>,
    parametric::Arguments<double, double>>
{
public:
    void eval() const override {
        if (auto s = res<0>(); s) {
            s->set_value(arg<0>().value() + arg<1>().value());
        }
        if (auto d = res<1>(); d) {
            d->set_value(arg<0>().value() - arg<1>().value());
        }
    }
};
}

TEST(Eval, swapParamMultipleOutputs)
{
    auto a  = parametric::new_param(5.0);
    auto b  = parametric::new_param(3.0);
    auto a2 = parametric::new_param(10.0);

    auto [sum, diff] = parametric::compute<SumDiff>(a, b);
    EXPECT_NEAR(8.0, sum, 1e-12);
    EXPECT_NEAR(2.0, diff, 1e-12);

    parametric::swap_param(a, a2);
    EXPECT_NEAR(13.0, sum, 1e-12);
    EXPECT_NEAR(7.0, diff, 1e-12);
}

TEST(Eval, swapParamClone)
{
    auto a  = parametric::new_param(1.0);
    auto b  = parametric::new_param(2.0);
    auto a2 = parametric::new_param(10.0);
    auto c  = parametric::eval(add, a, b);

    parametric::swap_param(a, a2);

    auto c_clone = c.clone();
    EXPECT_NEAR(12.0, c_clone, 1e-12);
    EXPECT_NEAR(12.0, c, 1e-12);
}
