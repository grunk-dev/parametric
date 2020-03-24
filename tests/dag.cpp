#include <gtest/gtest.h>

#include <parametric/dag.hpp>

using namespace parametric;

TEST(DAG, connect)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));

    // Add b as a parent to a
    add_parent(a, b);

    EXPECT_TRUE(b->precedes(*a));
}

TEST(DAG, precedes)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));

    EXPECT_FALSE(b->precedes(*a));

    // Add b as a parent to a
    add_parent(a, b);

    EXPECT_TRUE(b->precedes(*a));
    EXPECT_FALSE(a->precedes(*b));
    EXPECT_FALSE(a->precedes(*a));
    EXPECT_FALSE(b->precedes(*b));

    add_parent(b, c);

    EXPECT_TRUE(c->precedes(*a));
    EXPECT_TRUE(c->precedes(*b));
    EXPECT_FALSE(a->precedes(*c));
}

TEST(DAG, connectCircular)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));

    // Add b as a parent to a
    add_parent(a, b);
    add_parent(b, c);

    // this should not be allowed
    EXPECT_THROW(add_parent(c, a), std::runtime_error);
}

TEST(DAG, unattach)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));

    // Add b as a parent to a
    add_parent(c, b);
    add_parent(a, b);

    EXPECT_TRUE(b->precedes(*a));
    EXPECT_TRUE(b->precedes(*c));

    a->remove_parent(*b);
    EXPECT_FALSE(b->precedes(*a));
    EXPECT_TRUE(b->precedes(*c));
}
