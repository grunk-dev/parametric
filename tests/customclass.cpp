#include <gtest/gtest.h>

#include <parametric/core.hpp>
#include <cmath>


// an example, how we can also use multiple outputs
class CustomComputer : public parametric::ComputeNode
{
public:
    CustomComputer(const parametric::param<double>& op1, double op2)
        : m_op1(op1), m_op2(op2)
    {
        depends_on(m_op1);
        computes(m_result_pow, parametric::param<double>("result_pow"));
        computes(m_result_div, parametric::param<double>("result_div"));
    }

    parametric::param<double> pow() const
    {
        return m_result_pow;
    }

    parametric::param<double> div() const
    {
        return m_result_div;
    }

    void eval() const
    {
        if (!m_result_pow.expired())
            m_result_pow.set_value(::pow(m_op1, m_op2));
        if (!m_result_div.expired()) {
            m_result_div.set_value(m_op1 / m_op2);
        }
    }

private:
    // Inputs Args
    const parametric::param<double> m_op1;
    double m_op2;

    // Outputs
    mutable parametric::OutputParam<double> m_result_pow;
    mutable parametric::OutputParam<double> m_result_div;
};


TEST(CustomClass, multipleOuts)
{
    auto b = parametric::new_param(4.0, "b");

    auto node = parametric::new_node<CustomComputer>(b, 2.0);
    auto pow_result = node->pow();
    auto div_result = node->div();

    EXPECT_NEAR(16., pow_result, 1e-12);
    EXPECT_NEAR(2.0, div_result, 1e-12);

    b = 5.0;
    EXPECT_NEAR(25., pow_result, 1e-12);
    EXPECT_NEAR(2.5, div_result, 1e-12);
}

TEST(CustomClass, nestedScope)
{
    auto b = parametric::new_param(4.0);

    auto pow_result = parametric::new_param(0.);
    auto div_result = parametric::new_param(0.);

    {
        // We create the compute node in an inner scope
        // We want to test, wether the computation is still done
        // I.e. the node is not destroyed
        auto node = parametric::new_node<CustomComputer>(b, 2.0);
        pow_result = node->pow();
        div_result = node->div();
    }

    EXPECT_NEAR(16., pow_result, 1e-12);
    EXPECT_NEAR(2.0, div_result, 1e-12);

    b = 5.0;
    EXPECT_NEAR(25., pow_result, 1e-12);
    EXPECT_NEAR(2.5, div_result, 1e-12);
}

TEST(CustomClass, multipleComputer)
{
    auto b = parametric::new_param(4.0);

    // compute (b**2) / 2
    // This includes nesting of two compute nodes
    auto pow_result = parametric::new_node<CustomComputer>(b, 2.0)->pow();
    auto div_result = parametric::new_node<CustomComputer>(pow_result, 2.0)->div();

    EXPECT_NEAR(16., pow_result, 1e-12);
    EXPECT_NEAR(8.0, div_result, 1e-12);

    b = 5.0;
    EXPECT_NEAR(25.0, pow_result, 1e-12);
    EXPECT_NEAR(12.5, div_result, 1e-12);
}

TEST(CustomClass, lifeTime)
{
    auto pow_result = parametric::new_param<double>();

    {
        auto b = parametric::new_param(4.0);
        auto node = parametric::new_node<CustomComputer>(b, 2.0);
        pow_result = node->pow();
    }
    // no computation has be performed yet
    EXPECT_FALSE(pow_result.is_valid());

    // compute b**2
    // This triggers the computation, requiring
    // node and b to be still alive
    EXPECT_NEAR(16., pow_result, 1e-12);
    EXPECT_TRUE(pow_result.is_valid());

}
