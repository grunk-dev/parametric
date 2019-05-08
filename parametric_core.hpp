#ifndef PARAMETRIC_CORE_HPP
#define PARAMETRIC_CORE_HPP

#include <memory>
#include <vector>
#include <algorithm>
#include <exception>
#include <cassert>

#include <type_traits> // for std::false_type

namespace parametric
{

template <typename T> struct AlwaysFalse : std::false_type {
};

using namespace std;

class DAGNode;

typedef std::shared_ptr<DAGNode> NodeRef;
typedef std::shared_ptr<const DAGNode> ConstNodeRef;

class may_not_attach : public std::exception
{
};

class DAGNode
{
public:
    DAGNode(int id)
        : _id(id)
    {
    }

    friend void attach(NodeRef child, NodeRef parent)
    {

        if (!child || !parent) {
            throw std::invalid_argument("Cannot attach node: null pointer passed.");
        }

        if (!child->attachAllowed()) {
            throw may_not_attach();
        }

        if (child == parent || child->precedes(*parent)) {
            throw std::invalid_argument("Cannot attach node: cycles are not allowed.");
        }

        // only attach if not already attached
        if (std::find(std::begin(child->parents), std::end(child->parents), parent) == child->parents.end()) {
            child->parents.push_back(parent);
            parent->childs.push_back(child);
        }
    }

    virtual bool attachAllowed() const
    {
        return true;
    }

    virtual int id() const
    {
        return _id;
    }

    bool precedes(const DAGNode& child) const
    {
        class HasChildVisitor
        {
        public:
            HasChildVisitor(const DAGNode& c)
                : _c(c)
                , hasChild(false)
            {
            }

            void visit(const DAGNode& n, size_t depth)
            {
                if (&n == &_c && depth > 0) {
                    hasChild = true;
                }
            }

            bool hasChild;

        private:
            const DAGNode& _c;
        };

        HasChildVisitor v(child);
        accept(v);

        return v.hasChild;
    }

    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0) const
    {
        v.visit(*this, depth);
        for (std::weak_ptr<DAGNode>& child : childs) {
            if (NodeRef c = child.lock()) {
                c->accept(v, depth + 1);
            }
        }
    }

    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0)
    {
        v.visit(*this, depth);
        for (std::weak_ptr<DAGNode>& child : childs) {
            if (NodeRef c = child.lock()) {
                c->accept(v, depth + 1);
            }
        }
    }

    void unattachFrom(const DAGNode& parent)
    {
        auto& p_childs  = parent.childs;
        auto it_to_this = std::find_if(p_childs.begin(), p_childs.end(),
                                       [this](std::weak_ptr<DAGNode>& child) { return !child.lock(); });

        if (it_to_this != p_childs.end()) {
            p_childs.erase(it_to_this);
        }
    }

    ~DAGNode()
    {
        for (auto parent : parents) {
            if (parent)
                unattachFrom(*parent);
        }
    }

protected:
    int _id;

    mutable std::vector<std::weak_ptr<DAGNode>> childs;
    std::vector<std::shared_ptr<DAGNode>> parents;
};

class InvalidatibleNode : public DAGNode
{
public:
    InvalidatibleNode(int id)
        : DAGNode(id)
    {
    }

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

                if (auto* par = dynamic_cast<InvalidatibleNode*>(&n)) {
                    par->invalidateSelf();
                }
            }
        };

        InvalVisitor v;
        accept(v);
    }

protected:
    virtual void invalidateSelf() = 0;
};

class ValueType : public InvalidatibleNode
{
public:
    ValueType(int id)
        : InvalidatibleNode(id)
    {
    }
};

class ComputeNode : public InvalidatibleNode
{
public:
    virtual void compute() = 0;

    static void connect_ins_outs(std::shared_ptr<ComputeNode> computeNode)
    {
        for (auto in : computeNode->_inputs) {
            attach(computeNode, in);
        }
        for (auto out : computeNode->_outputs) {
            attach(out, computeNode);
        }
    }

    void define_input(const std::shared_ptr<ValueType>& input_param)
    {
        _inputs.push_back(input_param);
    }
    void define_output(const std::shared_ptr<ValueType>& output_param)
    {
        _outputs.push_back(output_param);
    }

protected:
    ComputeNode()
        : InvalidatibleNode(0)
    {
    }

private:
    std::vector<std::shared_ptr<ValueType>> _inputs, _outputs;

    void invalidateSelf()
    {
    }
};

template <class ResultType>
class param : public ValueType
{
public:
    param(const ResultType& v)
        : ValueType(1)
        , value(v)
    {
    }
    param()
        : ValueType(0)
    {
    }

    virtual const ResultType& Value() const
    {
        if (!value.is_initialized()) {
            computeValue();
            // this assertion should only fail, if the node was not connected to its compute node
            // and therefore compute fails
            assert(value.is_initialized());
        }

        return value.value();
    }
    virtual void SetValue(const ResultType& v)
    {
        if (!value.is_initialized() || v != value.value()) {
            value = v;
            invalidate();
        }
    }

    bool attachAllowed()
    {
        return parents.size() == 0;
    }

protected:
    void invalidateSelf()
    {
        value.reset();
    }

private:
    template <typename T> class optional
    {
    public:
        optional()
            : data(nullptr)
        {
        }

        optional(const T& t)
            : data(new T(t))
        {
        }
        const T& value() const
        {
            if (!data) {
                throw std::runtime_error("Value not initialized");
            }
            return *data;
        }

        operator T const&() const
        {
            return value();
        }

        bool is_initialized() const
        {
            return data != nullptr;
        }

        void reset()
        {
            data.reset();
        }

    private:
        std::unique_ptr<T> data;
    };

    void computeValue() const
    {
        // TODO: Here we need a mutex to avoid multiple threads computing
        // The same value
        assert(parents.size() <= 1);

        if (parents.size() > 0) {
            NodeRef p = parents[0];
            if (auto* computeNode = dynamic_cast<ComputeNode*>(p.get())) {
                computeNode->compute();
            }
        }
    }

    optional<ResultType> value;
};

template <typename T>
using param_ptr = std::shared_ptr<param<T>>;

template <class T>
param_ptr<T> new_param(const T& v)
{
    return std::make_shared<param<T>>(v);
}

template <class T>
param_ptr<T> new_param()
{
    return std::make_shared<param<T>>();
}

// Disable connecting two values
template <class C1, class C2>
void attach(param_ptr<C1>&, param_ptr<C2>&)
{
    static_assert(AlwaysFalse<C1>::value, "Connecting two parametric values is not allowed");
}

} // namespace parametric

#endif // PARAMETRIC_CORE_HPP
