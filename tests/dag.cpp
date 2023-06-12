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

TEST(DAG, clone)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));

    add_parent(c, a);
    add_parent(c, b);

    {
        NodeRef z = c->clone();
        // z should not depend on a and b, but should have two new parents
        EXPECT_FALSE(a->precedes(*z));
        EXPECT_FALSE(b->precedes(*z));
        EXPECT_EQ(z->num_parents(), 2);

        // a and b should have only one child, namely c
        EXPECT_EQ(a->num_children(), 1);
        EXPECT_EQ(b->num_children(), 1);
        EXPECT_EQ(c->num_parents(), 2);
        EXPECT_TRUE(a->precedes(*c));
        EXPECT_TRUE(b->precedes(*c));
    }

    {
        auto cloned_nodes = std::make_shared<DAGNode::ClonedNodeMap>();
        NodeRef x = a->clone(cloned_nodes);
        NodeRef y = b->clone(cloned_nodes);
        NodeRef z = c->clone(cloned_nodes);

        // check the map
        EXPECT_EQ(cloned_nodes->size(), 3);
        EXPECT_EQ((*cloned_nodes)[a.get()], x);
        EXPECT_EQ((*cloned_nodes)[b.get()], y);
        EXPECT_EQ((*cloned_nodes)[c.get()], z);

        // parent-child relations of x,y and z should be the same as a,b and c
        EXPECT_EQ(x->num_parents(), 0);
        EXPECT_EQ(x->num_children(), 1);
        EXPECT_EQ(y->num_parents(), 0);
        EXPECT_EQ(y->num_children(), 1);
        EXPECT_EQ(z->num_parents(), 2);
        EXPECT_EQ(z->num_children(), 0);
        EXPECT_TRUE(x->precedes(*z));
        EXPECT_TRUE(y->precedes(*z));
    }
}
