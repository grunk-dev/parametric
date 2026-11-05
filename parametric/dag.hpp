// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
// SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

/**
 * @file dag.hpp
 *
 * @brief This file defines the building blocks to create DAGs
 */
#ifndef DAG_HPP
#define DAG_HPP

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>
#include <type_traits> // for std::false_type
#include <string>
#include <stdexcept>

namespace parametric
{

/// @private
template <typename T> struct AlwaysFalse : std::false_type {
};

class DAGNode;

/**
 * @brief A shared pointer to a DAG node
 */
typedef std::shared_ptr<parametric::DAGNode> NodeRef;


/**
     * @brief Adds the node "parent" as parent to the node "child"
     * @param child The node to which "parent" is added as a parent.
     * @param parent The parent node to be added.
     */
void add_parent(const NodeRef& child, const NodeRef& parent);

/**
 * @brief Replaces old_node by new_node as a parent of all children of old_node
 *
 * After the call, every child that depended on old_node depends on new_node instead, at the same
 * position in its parent list. This also holds if old_node appears multiple times in the parent list
 * of a child. The moved children and all their descendants are invalidated. old_node itself stays valid
 * and keeps its own parents, but has no children afterwards. The parents of new_node are not modified.
 *
 * The function has the strong exception guarantee: if it throws, the graph is unchanged.
 *
 * @param old_node The node to be replaced.
 * @param new_node The node that takes over the children of old_node.
 * @throws std::invalid_argument if one of the nodes is null or has positional children (e.g. compute nodes).
 * @throws std::runtime_error if the replacement would introduce a cycle.
 */
void swap_node(const NodeRef& old_node, const NodeRef& new_node);

/**
 * @brief This class provides a basis node of a directed acyclic graph (DAG)
 *
 * It provides all required methods to connect multiple nodes to a
 * larger graph.
 *
 * Design Considerations: In the current design, child nodes own their parent nodes.
 * This makes it possible to hold a pointer to a child node keeping the whole graph alive.
 */
class DAGNode
{
public:
    /**
     * @brief DAGNode Constructs the node with an identifier.
     * The id does not have to be unique.
     *
     * @param id The identifier of the node.
     */
    DAGNode(const std::string& id)
        : _id(id)
    {}

    /**
     * @brief This virtual function can be overwritten for serializing 
     * and deserializing nodes to std::string. 
     * 
     * @return std::string 
     */
    virtual std::string serialize() const
    {
        throw std::logic_error("DAGNode::serialize is not implemented by the derived class.");
    };

    /**
     * @brief Adds the node "parent" as parent to the node "child"
     *
     * The list of children is unique, but the list of parents may contain duplicates. The rationale
     * behind this is that a compute node could have the same parent for different arguments, e.g. b = a*a,
     * but a parent must only invalidate their children once.
     *
     * @param child The node to which "parent" is added as a parent.
     * @param parent The parent node to be added.
     */
    friend void add_parent(const NodeRef& child, const NodeRef& parent)
    {

        if (!child || !parent) {
            throw std::invalid_argument("Cannot attach node: null pointer passed.");
        }


        if (child == parent || child->precedes(*parent)) {
            throw std::runtime_error("Cannot attach node: cycles are not allowed.");
        }

        // Drop entries of consumers that have been destroyed in the meantime, so that a long-lived
        // node does not accumulate one dead weak_ptr per consumer ever attached to it. This is done
        // here and not in ~DAGNode, because a node may be destroyed while its parent is traversing
        // its child list. Nodes with positional children (outputs of compute nodes) must keep their slots.
        if (!parent->has_positional_children()) {
            parent->childs.erase(
                std::remove_if(parent->childs.begin(), parent->childs.end(),
                               [](auto const& c) { return c.expired(); }),
                parent->childs.end());
        }

        auto predicate = [&child](auto const& c){
            return c.lock() == child;
        };
        if (std::find_if(std::begin(parent->childs), std::end(parent->childs), predicate ) == parent->childs.end()) {
            parent->childs.push_back(child);
        }

        child->parents.push_back(parent);
    }

    friend void swap_node(const NodeRef& old_node, const NodeRef& new_node)
    {
        if (!old_node || !new_node) {
            throw std::invalid_argument("Cannot swap node: null pointer passed.");
        }

        if (old_node->has_positional_children() || new_node->has_positional_children()) {
            throw std::invalid_argument("Cannot swap node: nodes with positional children (e.g. compute nodes) are not supported.");
        }

        if (old_node == new_node) {
            return;
        }

        // Take a snapshot of the live children, because the child list of old_node is modified below.
        std::vector<NodeRef> moved;
        for (auto const& c : old_node->childs) {
            if (NodeRef cp = c.lock()) {
                moved.push_back(std::move(cp));
            }
        }

        // Validate everything before modifying anything.
        for (auto const& c : moved) {
            if (c == new_node || c->precedes(*new_node)) {
                throw std::runtime_error("Cannot swap node: cycles are not allowed.");
            }
        }

        for (auto const& c : moved) {
            c->replace_parent(*old_node, new_node);

            auto predicate = [&c](auto const& x) { return x.lock() == c; };
            if (std::find_if(new_node->childs.begin(), new_node->childs.end(), predicate) == new_node->childs.end()) {
                new_node->childs.push_back(c);
            }
        }
        old_node->childs.clear();

        for (auto const& c : moved) {
            c->invalidateSelf();
            c->invalidate();
        }
    }

    /**
     * @brief Returns the identifier of the node
     * @return The id as a string
     */
    std::string id() const
    {
        return _id;
    }

    /**
     * @brief Sets the identifier of the node
     * @param id The identifier string.
     */
    void set_id(const std::string& id)
    {
        _id = id;
    }


    /**
     * @brief Checks, whether "node" precides the current node in the DAG
     * @param node The node to be checked
     * @return True, if node precedes this node.
     */
    bool precedes(const DAGNode& node) const
    {
        class HasChildVisitor
        {
        public:
            HasChildVisitor(const DAGNode& c)
                : _c(c)
                , hasChild(false)
            {}

            void visit(const DAGNode& n, size_t depth)
            {
                if (&n == &_c && depth > 0) {
                    hasChild = true;
                }
            }

        private:
            const DAGNode& _c;

        public:
            bool hasChild;
        };

        HasChildVisitor v(node);
        accept(v);

        return v.hasChild;
    }

    /**
     * @brief Defines the graph travsersal direction
     */
    enum class Direction
    {
        up,  /**< Traverse upwards, i.e. in direction of parents */
        down /**< Traverse downwards, i.e. in direction of childs */
    };

    /**
     * @brief This provides an interface for the visitor pattern to
     * walk through the whole DAG down from this node
     *
     * @param v The Visitor that is applied on each node during traversal
     * @param depth The current depth of traversal
     * @param dir The direction of traversal (either up or down)
     */
    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0, Direction dir=Direction::down) const
    {
        v.visit(*this, depth);
        if (dir == Direction::down) {
            for (std::weak_ptr<DAGNode>& child : childs) {
                if (NodeRef c = child.lock()) {
                    auto const& cc = *c; // const ref to child
                    cc.accept(v, depth + 1, dir);
                }
            }
        }
        else {
            for (const NodeRef& parent : parents) {
                auto const& p = *parent; // const ref to parent
                p.accept(v, depth + 1, dir);
            }
        }
    }

    /**
     * @brief This provides an interface for the visitor pattern to
     * walk through the whole DAG down from this node
     *
     * @param v The Visitor that is applied on each node during traversal
     * @param depth The current depth of traversal
     * @param dir The direction of traversal (either up or down)
     */
    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0, Direction dir=Direction::down)
    {
        v.visit(*this, depth);
        if (dir == Direction::down) {
            for (std::weak_ptr<DAGNode>& child : childs) {
                if (NodeRef c = child.lock()) {
                    c->accept(v, depth + 1, dir);
                }
            }
        }
        else {
            for (const NodeRef& parent : parents) {
                parent->accept(v, depth + 1, dir);
            }
        }
    }

    /**
     * @brief Removes the node "parent" from the current node.
     * @param parent The node to be removed
     */
    void remove_parent(const DAGNode& parent)
    {
        // remove myself from parent. child list has unique entries
        auto& p_childs  = parent.childs;
        auto it_to_this = std::find_if(p_childs.begin(), p_childs.end(),
                                       [this](std::weak_ptr<DAGNode>& child) {
                                           return child.lock().get() == this;
                                       });

        if (it_to_this != p_childs.end()) {
            p_childs.erase(it_to_this);
        }

        // remove parent from parent list. parent lists may have duplicate entries
        auto parentIt = std::find_if(std::begin(parents), std::end(parents), [&parent](const NodeRef& p) {
            return p.get() == &parent;
        });
        while ( parentIt != parents.end() ) {
            parents.erase(parentIt);
            parentIt = std::find_if(std::begin(parents), std::end(parents), [&parent](const NodeRef& p) {
                return p.get() == &parent;
            });
        }
    }

    void swap_child(const DAGNode& child_old, const NodeRef& child_new)
    {
        if(
            auto search = std::find_if(
                childs.begin(), childs.end(), 
                [&child_old](std::weak_ptr<DAGNode>& child){ return child.lock().get() == &child_old; }
            ); 
            search != childs.end()
        ) 
        {
            *search = child_new;
        }
    }

    /**
     * @brief returns the number of parents
     */
    size_t num_parents() const {
        return parents.size();
    }

    /**
     * @brief returns the number of children
     */

    size_t num_children() const {
        return childs.size();
    }

    /**
     * @brief Invalidates this node and all other childs
     */
    void invalidate()
    {
        class InvalVisitor
        {
        public:
            void visit(DAGNode& n, size_t depth)
            {
                if (depth == 0) {
                    return;
                }

                n.invalidateSelf();
            }
        };

        InvalVisitor v;
        accept(v);
    }

    /**
     * @brief This method is internally used to evaluate the state of the node
     */
    virtual void eval() const {}

    /**
     * @brief Returns whether the node is in a valid state
     */
    virtual bool IsValid() const { return false; };

    /**
     * @brief Destroys the node.
     *
     * The parent list is released together with the node. Entries in the parents' child lists
     * are deliberately left in place: they are weak pointers that simply expire, and compute nodes
     * rely on them as positional output slots (see ComputeNode::res).
     */
    virtual ~DAGNode() = default;

    using ClonedNodeMap = std::unordered_map<DAGNode const*, std::shared_ptr<DAGNode>>; /**< @private */
    static std::shared_ptr<ClonedNodeMap> new_cloned_node_map() {
        return std::make_shared<ClonedNodeMap>();
    }

    /**
     * @brief This function deep-copies a node together with all of its ancestors, while maintaining
     * parent-child-relations
     *
     * The optional input cloned_nodes is used to keep track of the already cloned nodes. This way
     * redundant clones can be avoided.
     * 
     */
    virtual std::shared_ptr<DAGNode> clone(
        std::shared_ptr<ClonedNodeMap> cloned_nodes = new_cloned_node_map()
    ) const
    {
        throw std::runtime_error("DAGNode: No implementation for virtual method \"clone\" found.\n");
    }

    std::vector<std::weak_ptr<DAGNode>> const& get_children() const {
        return childs;
    }

    std::vector<std::shared_ptr<DAGNode>> const& get_parents() const {
        return parents;
    }

private:
    std::string _id; /**< @private */

protected:
    /**
     * @brief Replaces all occurrences of parent_old in the parent list by parent_new, keeping their
     * positions. The child lists of the parents are not modified.
     */
    void replace_parent(const DAGNode& parent_old, const NodeRef& parent_new)
    {
        for (auto& p : parents) {
            if (p.get() == &parent_old) {
                p = parent_new;
            }
        }
    }

    mutable std::vector<std::weak_ptr<DAGNode>> childs; /**< @brief Pointer to child nodes */
    std::vector<std::shared_ptr<DAGNode>> parents;      /**< @brief Pointer to parent nodes */

    /**
     * @brief Override this function to implement the
     * specific invalidation logic of the node
     */
    virtual void invalidateSelf() {}

    /**
     * @brief Override this function to return true if the position of a child in the child list is
     * meaningful, e.g. for the outputs of a compute node. Dead entries of such nodes are never pruned.
     */
    virtual bool has_positional_children() const { return false; }
};

template <typename Derived>
struct ClonableDAGNode : public DAGNode
{
    ClonableDAGNode(std::string const& id) : DAGNode(id) {}

    std::shared_ptr<DAGNode> clone(
        std::shared_ptr<DAGNode::ClonedNodeMap> cloned_nodes = DAGNode::new_cloned_node_map()
    ) const override 
    {
        auto& cloned = (*cloned_nodes)[this];
        if (!cloned) {
            auto c = std::make_shared<Derived>(static_cast<Derived const&>(*this));
            for (size_t i = 0; i < parents.size(); ++i) {
                c->parents[i] = parents[i]->clone(cloned_nodes);
                c->parents[i]->swap_child(*this, c);
            }
            cloned = c;
        }
        return cloned;
    }
};

} // namespace parametric

#endif // DAG_HPP
