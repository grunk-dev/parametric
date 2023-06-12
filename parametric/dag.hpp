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

        // only attach if not already attached
        if (std::find(std::begin(child->parents), std::end(child->parents), parent) == child->parents.end()) {
            child->parents.push_back(parent);
            parent->childs.push_back(child);
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
        // remove myself from parent
        auto& p_childs  = parent.childs;
        auto it_to_this = std::find_if(p_childs.begin(), p_childs.end(),
                                       [this](std::weak_ptr<DAGNode>& child) {
                                           return child.lock().get() == this;
                                       });

        if (it_to_this != p_childs.end()) {
            p_childs.erase(it_to_this);
        }

        // remove parent from parent list
        auto parentIt = std::find_if(std::begin(parents), std::end(parents), [&parent](const NodeRef& p) {
            return p.get() == &parent;
        });

        if (parentIt != parents.end()) {
            parents.erase(parentIt);
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
    
    /**
     * @brief Override this function to deep-copy a node
     *
     * The optional input cloned_nodes is used to keep track of the already cloned nodes. This way
     * redundant clones can be avoided.
     * 
     */
    virtual std::shared_ptr<DAGNode> clone(
        std::shared_ptr<ClonedNodeMap> cloned_nodes = std::make_shared<ClonedNodeMap>()
    ) 
    {
        auto& cloned = (*cloned_nodes)[this];
        if (!cloned) {
            cloned = std::make_shared<DAGNode>(*this);
            cloned->childs.clear(); // we are only cloning in direction of ancestors
            for (auto& parent : cloned->parents) {
                parent = parent->clone(cloned_nodes);
                parent->childs.push_back(cloned);
            }
        }
        return cloned;
    }

private:
    std::string _id; /**< @private */

protected:
    mutable std::vector<std::weak_ptr<DAGNode>> childs; /**< @brief Pointer to child nodes */
    std::vector<std::shared_ptr<DAGNode>> parents;      /**< @brief Pointer to parent nodes */

    /**
     * @brief Override this function to implement the
     * specific invalidation logic of the node
     */
    virtual void invalidateSelf() {}
};

} // namespace parametric

#endif // DAG_HPP
