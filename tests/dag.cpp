// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
// SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

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
// Destroying a node must not modify the parent list it iterates over (UB, flagged by ASan with
// -D_GLIBCXX_SANITIZE_VECTOR). See https://github.com/grunk-dev/parametric/issues/86
TEST(DAG, destroyNodeWithParents)
{
    NodeRef x(new DAGNode("x"));
    NodeRef y(new DAGNode("y"));
    {
        NodeRef z(new DAGNode("z"));
        NodeRef w(new DAGNode("w"));
        add_parent(z, x);
        add_parent(z, y);
        add_parent(w, z);
        add_parent(w, x);
        EXPECT_EQ(2, w->num_parents());
    }
    // the surviving parents are still intact and usable
    EXPECT_EQ(0, x->num_parents());
    EXPECT_EQ(0, y->num_parents());
    NodeRef c(new DAGNode("c"));
    add_parent(c, x);
    EXPECT_TRUE(x->precedes(*c));
}

TEST(DAG, destroyNodeWithDuplicateParents)
{
    NodeRef a(new DAGNode("a"));
    {
        NodeRef b(new DAGNode("b"));
        add_parent(b, a);
        add_parent(b, a);
        EXPECT_EQ(2, b->num_parents());
    }
    NodeRef c(new DAGNode("c"));
    add_parent(c, a);
    EXPECT_TRUE(a->precedes(*c));
}

// Dead consumers must not pile up in a long-lived parent. See https://github.com/grunk-dev/parametric/issues/85
TEST(DAG, deadChildrenArePruned)
{
    NodeRef x(new DAGNode("x"));
    for (int i = 0; i < 1000; ++i) {
        NodeRef c(new DAGNode("c"));
        add_parent(c, x);
    }
    NodeRef alive(new DAGNode("alive"));
    add_parent(alive, x);
    EXPECT_EQ(1, x->num_children());
    EXPECT_TRUE(x->precedes(*alive));
}
