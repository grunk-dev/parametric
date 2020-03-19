#ifndef DAG_HPP
#define DAG_HPP

#include <algorithm>
#include <memory>
#include <vector>
#include <type_traits> // for std::false_type

#include <cassert>

namespace parametric
{

template <typename T> struct AlwaysFalse : std::false_type {
};

class DAGNode;

typedef std::shared_ptr<DAGNode> NodeRef;

class may_not_attach : public std::exception
{};

class DAGNode
{
public:
    DAGNode(const std::string& id)
        : _id(id)
    {}

    friend void addParent(const NodeRef& child, const NodeRef& parent)
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


    virtual std::string id() const
    {
        return _id;
    }

    void SetId(const std::string& id)
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
     * @brief This provides an interface for the visitor pattern to
     * walk through the whole DAG down from this node
     */
    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0,bool down=true) const
    {
        v.visit(*this, depth);
        if (down) {
            for (std::weak_ptr<DAGNode>& child : childs) {
                if (NodeRef c = child.lock()) {
                    c->accept(v, depth + 1, down);
                }
            }
        }
        else {
            for (NodeRef parent : parents) {
                parent->accept(v, depth + 1, down);
            }
        }
    }

    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0, bool down=true)
    {
        v.visit(*this, depth);
        if (down) {
            for (std::weak_ptr<DAGNode>& child : childs) {
                if (NodeRef c = child.lock()) {
                    c->accept(v, depth + 1, down);
                }
            }
        }
        else {
            for (NodeRef parent : parents) {
                parent->accept(v, depth + 1, down);
            }
        }
    }

    void removeParent(const DAGNode& parent)
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

    // Invalidates this node and all other childs
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

    virtual void eval() const {}

    virtual ~DAGNode()
    {
        for (auto parent : parents) {
            if (parent) {
                removeParent(*parent);
            }
        }
    }


protected:
    std::string _id;

    mutable std::vector<std::weak_ptr<DAGNode>> childs;
    std::vector<std::shared_ptr<DAGNode>> parents;

protected:
    virtual void invalidateSelf() {}
};

} // namespace parametric

#endif // DAG_HPP
