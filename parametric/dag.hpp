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
 * @brief swap_node replaces old_node in old_node's DAG with new_node.
 * The old_node remains valid, but its children now use new_node instead.
 * The new_node's previous parent-child-relations are kept in place.
 * @param old_node
 * @param new_node
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

        auto predicate = [&child](auto const& c){
            if (!c.expired()) {
                return c.lock() == child;
            }
            return false;
        };
        if (std::find_if(std::begin(parent->childs), std::end(parent->childs), predicate ) == parent->childs.end()) {
            parent->childs.push_back(child);
        }

        child->parents.push_back(parent);
    }

    friend void swap_node(const NodeRef& old_node, const NodeRef& new_node)
    {
        auto const& children = old_node->get_children();
        for (auto const& c : children) {
            if (auto cp = c.lock(); cp) {
                cp->swap_parent(
                    *old_node,
                    new_node
                );
                old_node->remove_child(cp);
                new_node->add_child(cp);
            }
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

    /**
     * @brief swap_child swaps a child without adding this as parent to child_new
     */
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

    virtual ~DAGNode()
    {
        for (const auto& parent : parents) {
            if (parent) {
                remove_parent(*parent);
            }
        }
    }

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
     * @brief swap_parent swaps a parent without adding this as child to parent_new
     */
    void swap_parent(const DAGNode& parent_old, const NodeRef& parent_new)
    {
        if (
            auto search = std::find_if(
                parents.begin(), parents.end(),
                [&parent_old](NodeRef& parent) { return parent.get() == &parent_old; }
                );
            search != parents.end()
            )
        {
            *search = parent_new;
        }
    }

    /**
     * @brief add_child adds a new child, without modifying the parent list of the child
     * @param child_new
     */
    void add_child(const NodeRef& child_new) {
        childs.push_back(child_new);
    }

    void remove_child(const NodeRef& child) {
        if(
            auto search = std::find_if(
                childs.begin(), childs.end(),
                [&child](std::weak_ptr<DAGNode>& c){ return c.lock().get() == child.get(); }
                );
            search != childs.end()
            )
        {
            childs.erase(search);
        }
    }


    mutable std::vector<std::weak_ptr<DAGNode>> childs; /**< @brief Pointer to child nodes */
    std::vector<std::shared_ptr<DAGNode>> parents;      /**< @brief Pointer to parent nodes */

    /**
     * @brief Override this function to implement the
     * specific invalidation logic of the node
     */
    virtual void invalidateSelf() {}
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
