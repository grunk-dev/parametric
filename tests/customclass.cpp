#include <gtest/gtest.h>

#include <parametric/core.hpp>
#include <cmath>


// an example, how we can also use multiple outputs
class CustomComputer : public parametric::ComputeNode<CustomComputer, parametric::Results<double, double>, parametric::Arguments<double>>
{
public:
    CustomComputer(double op2)
        : m_op2(op2)
    {}

    void eval() const
    {
        if (auto r0 = res<0>(); r0)
            r0->set_value(::pow(arg<0>().value(), m_op2));
        if (auto r1 = res<1>(); r1)
            r1->set_value(arg<0>().value() / m_op2);
    }

private:
    double m_op2;
};

decltype(auto) custom_compute(parametric::param<double> op1, double op2){

    struct CustomResult {
        decltype(auto) pow() { return std::get<0>(parms); }
        decltype(auto) div() { return std::get<1>(parms); }
        std::tuple<parametric::param<double>, parametric::param<double>> parms;
    };

    auto ptr = std::make_shared<CustomComputer>(op2);
    return CustomResult{parametric::compute(ptr, op1)};
}


TEST(CustomClass, multipleOuts)
{
    auto b = parametric::new_param(4.0, "b");

    auto node = custom_compute(b, 2.0);
    auto pow_result = node.pow();
    auto div_result = node.div();

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
        auto node = custom_compute(b, 2.0);
        pow_result = node.pow();
        div_result = node.div();
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
    auto pow_result = custom_compute(b, 2.0).pow();
    auto div_result = custom_compute(pow_result, 2.0).div();

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
        auto node = custom_compute(b, 2.0);
        pow_result = node.pow();
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
    auto n = custom_compute(a, 2.0);
    auto b = n.pow();
    auto c = n.div();

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

