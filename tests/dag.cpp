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

// Replacing a parent. See https://github.com/grunk-dev/parametric/issues/22
TEST(DAG, swapNode)
{
    NodeRef a(new DAGNode("a"));
    NodeRef a2(new DAGNode("a2"));
    NodeRef p(new DAGNode("p"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));
    NodeRef d(new DAGNode("d"));

    add_parent(a, p);
    add_parent(b, a);
    add_parent(c, d);
    add_parent(c, a); // a is not the first parent of c
    add_parent(a2, d);

    swap_node(a, a2);

    // old node keeps its parents, but has no children anymore
    EXPECT_EQ(1, a->num_parents());
    EXPECT_EQ(0, a->num_children());
    EXPECT_TRUE(p->precedes(*a));

    // new node keeps its parents and received the children
    EXPECT_EQ(1, a2->num_parents());
    EXPECT_EQ(2, a2->num_children());
    EXPECT_TRUE(a2->precedes(*b));
    EXPECT_TRUE(a2->precedes(*c));
    EXPECT_FALSE(a->precedes(*b));
    EXPECT_FALSE(a->precedes(*c));
    EXPECT_FALSE(p->precedes(*b));

    // the position in the parent list is kept
    ASSERT_EQ(2, c->num_parents());
    EXPECT_EQ(d, c->get_parents()[0]);
    EXPECT_EQ(a2, c->get_parents()[1]);
}

TEST(DAG, swapNodeDuplicateParents)
{
    NodeRef a(new DAGNode("a"));
    NodeRef a2(new DAGNode("a2"));
    NodeRef b(new DAGNode("b"));

    add_parent(b, a);
    add_parent(b, a);

    swap_node(a, a2);

    ASSERT_EQ(2, b->num_parents());
    EXPECT_EQ(a2, b->get_parents()[0]);
    EXPECT_EQ(a2, b->get_parents()[1]);
    EXPECT_EQ(1, a2->num_children());
    EXPECT_EQ(0, a->num_children());
}

TEST(DAG, swapNodeNewIsAlreadyParent)
{
    NodeRef a(new DAGNode("a"));
    NodeRef a2(new DAGNode("a2"));
    NodeRef b(new DAGNode("b"));

    add_parent(b, a);
    add_parent(b, a2);

    swap_node(a, a2);

    // the child list is unique
    EXPECT_EQ(1, a2->num_children());
    EXPECT_EQ(2, b->num_parents());
    EXPECT_EQ(0, a->num_children());
}

TEST(DAG, swapNodeCycle)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    NodeRef c(new DAGNode("c"));
    NodeRef a2(new DAGNode("a2"));

    add_parent(b, a);
    add_parent(c, b);
    add_parent(a2, c); // a2 is downstream of a

    EXPECT_THROW(swap_node(a, a2), std::runtime_error);
    EXPECT_THROW(swap_node(a, b), std::runtime_error);

    // nothing has changed
    EXPECT_EQ(1, a->num_children());
    EXPECT_EQ(a, b->get_parents()[0]);
    EXPECT_EQ(0, a2->num_children());
    EXPECT_EQ(1, a2->num_parents());
}

TEST(DAG, swapNodeInvalidArguments)
{
    NodeRef a(new DAGNode("a"));
    NodeRef b(new DAGNode("b"));
    add_parent(b, a);

    EXPECT_THROW(swap_node(nullptr, a), std::invalid_argument);
    EXPECT_THROW(swap_node(a, nullptr), std::invalid_argument);

    // swapping a node with itself is a no-op
    swap_node(a, a);
    EXPECT_EQ(1, a->num_children());
    EXPECT_EQ(a, b->get_parents()[0]);
}

TEST(DAG, swapNodeIgnoresDeadChildren)
{
    NodeRef a(new DAGNode("a"));
    NodeRef a2(new DAGNode("a2"));
    NodeRef alive(new DAGNode("alive"));
    {
        NodeRef dead(new DAGNode("dead"));
        add_parent(dead, a);
        add_parent(alive, a);
    }

    swap_node(a, a2);

    EXPECT_EQ(0, a->num_children());
    EXPECT_EQ(1, a2->num_children());
    EXPECT_EQ(a2, alive->get_parents()[0]);
}
