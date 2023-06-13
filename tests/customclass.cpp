#include <gtest/gtest.h>

#include <parametric/core.hpp>
#include <cmath>


// an example, how we can also use multiple outputs
class CustomComputer : public parametric::ComputeNode<CustomComputer>
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

TEST(CustomClass, clone)
{
    // original parametric tree
    auto a = parametric::new_param(4.0);
    auto n = parametric::new_node<CustomComputer>(a, 2.0);
    auto b = n->pow();
    auto c = n->div();

    // cloned parametric tree
    auto cloned_nodes = parametric::DAGNode::new_cloned_node_map();
    auto x = a.clone(cloned_nodes);
    auto y = b.clone(cloned_nodes);
    auto z = c.clone(cloned_nodes);

    // check parent-child relations
    EXPECT_TRUE(a.node_pointer()->precedes(*b.node_pointer()));
    EXPECT_TRUE(a.node_pointer()->precedes(*c.node_pointer()));
    EXPECT_TRUE(x.node_pointer()->precedes(*y.node_pointer()));
    EXPECT_TRUE(x.node_pointer()->precedes(*z.node_pointer()));
    EXPECT_FALSE(a.node_pointer()->precedes(*y.node_pointer()));
    EXPECT_FALSE(a.node_pointer()->precedes(*z.node_pointer()));
    EXPECT_FALSE(x.node_pointer()->precedes(*b.node_pointer()));
    EXPECT_FALSE(x.node_pointer()->precedes(*c.node_pointer()));

    // y,z should be copies of a,b and behave the same
    EXPECT_NEAR(b.value(), 16., 1e-12);
    EXPECT_NEAR(c.value(), 2., 1e-12);
    EXPECT_NEAR(y.value(), 16., 1e-12);
    // EXPECT_NEAR(z.value(), 2., 1e-12);

    // // changing a should change b and c, but not y and z
    // x.change_value() = 3.;

    // EXPECT_TRUE(b.is_valid());
    // EXPECT_NEAR(b.value(), 16., 1e-12);
    // EXPECT_TRUE(c.is_valid());
    // EXPECT_NEAR(c.value(), 2., 1e-12);

    // EXPECT_FALSE(y.is_valid());
    // EXPECT_NEAR(y.value(), 9., 1e-12);
    // EXPECT_FALSE(z.is_valid());
    // EXPECT_NEAR(z.value(), 1.5, 1e-12);

}

