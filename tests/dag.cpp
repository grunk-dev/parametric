#include <gtest/gtest.h>

#include <parametric/dag.hpp>

using namespace parametric;

TEST(DAG, connect)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));

    // Add b as a parent to a
    addParent(a, b);

    EXPECT_TRUE(b->precedes(*a));
}

TEST(DAG, precedes)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));

    EXPECT_FALSE(b->precedes(*a));

    // Add b as a parent to a
    addParent(a, b);

    EXPECT_TRUE(b->precedes(*a));
    EXPECT_FALSE(a->precedes(*b));
    EXPECT_FALSE(a->precedes(*a));
    EXPECT_FALSE(b->precedes(*b));

    addParent(b, c);

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
    addParent(a, b);
    addParent(b, c);

    // this should not be allowed
    EXPECT_THROW(addParent(c, a), std::runtime_error);
}

TEST(DAG, unattach)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));

    // Add b as a parent to a
    addParent(c, b);
    addParent(a, b);

    EXPECT_TRUE(b->precedes(*a));
    EXPECT_TRUE(b->precedes(*c));

    a->removeParent(*b);
    EXPECT_FALSE(b->precedes(*a));
    EXPECT_TRUE(b->precedes(*c));
}
