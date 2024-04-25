#include <gtest/gtest.h>

#include <parametric/core.hpp>
#include <parametric/operators.hpp>

#include <chrono>
#include <thread>

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

TEST(Eval, multithreading) {

    auto add = [](int l, int r){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return l+r;
    };

    auto l00 = parametric::new_param(1);
    auto l01 = parametric::new_param(2);
    auto l02 = parametric::new_param(3);
    auto l03 = parametric::new_param(4);

    auto l10 = parametric::eval(add, l00, l01);
    auto l11 = parametric::eval(add, l01, l02);
    auto l12 = parametric::eval(add, l02, l03);

    auto l20 = parametric::eval(add, l10, l11);
    auto l21 = parametric::eval(add, l11, l12);

    auto l30 = parametric::eval(add, l20, l21);

    using namespace std::chrono_literals;
    const auto start = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(20, l30.value());

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;
    
    // parallel evaluation should first one add, then two adds in parallel, then three adds in parallel.
    // Since add takes approx. 100 ms, I expect the total execution time to be below 350
    if (std::thread::hardware_concurrency() > 2) {
        EXPECT_LE(elapsed.count(), 350);
    }
}