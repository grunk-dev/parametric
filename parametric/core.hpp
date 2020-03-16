#ifndef PARAMETRIC_CORE_HPP
#define PARAMETRIC_CORE_HPP

#include <parametric/typename.hpp>
#include <parametric/tupletools.hpp>
#include <parametric/optional.hpp>

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

class may_not_attach : public std::exception
{
};

class DAGNode
{
public:
    DAGNode(const std::string& id)
        : _id(id)
    {
    }

    friend void attach(const NodeRef& child, const NodeRef& parent)
    {

        if (!child || !parent) {
            throw std::invalid_argument("Cannot attach node: null pointer passed.");
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


    virtual std::string id() const
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

        private:
            const DAGNode& _c;

        public:
            bool hasChild;
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
        [](std::weak_ptr<DAGNode>& child) {
            return !child.lock();
        });

        if (it_to_this != p_childs.end()) {
            p_childs.erase(it_to_this);
        }
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
                unattachFrom(*parent);
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




template <class ResultType>
class param_holder : public DAGNode
{
public:
    param_holder(const ResultType& v, const std::string& id)
        : DAGNode(id)
        , value(v)
    {
    }
    param_holder(const std::string& id)
        : DAGNode(id)
    {
    }

    virtual const ResultType& Value() const
    {
        if (!value.is_initialized()) {
            eval();
            // this assertion should only fail, if the node was not connected to its compute node
            // and therefore compute fails
            assert(value.is_initialized());
        }

        return value.value();
    }
    virtual void SetValue(const ResultType& v)
    {
        // TODO: not all classes provide the != operators. This should be abstracted
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
    void eval() const override
    {
        // TODO: Here we need a mutex to avoid multiple threads computing
        // The same value
        assert(parents.size() <= 1);

        if (parents.size() > 0) {
            NodeRef p = parents[0];
            p->eval();
        }
    }

    optional<ResultType> value;
};


template <typename T>
class param
{
public:
    param(const T& v, const std::string& id)
        : m_holder(std::make_shared<param_holder<T>>(v, id))
    {}

    param(const std::string& id)
        : m_holder(std::make_shared<param_holder<T>>(id))
    {}

    const std::shared_ptr<param_holder<T>> node_pointer() const
    {
        return m_holder;
    }

    const T& Value() const
    {
        return m_holder->Value();
    }

    void SetValue(const T& other)
    {
        return m_holder->SetValue(other);
    }

    operator const T& () const
    {
        return Value();
    }

    param<T>& operator=(const T& other)
    {
        m_holder->SetValue(other);
        return *this;
    }

private:
    std::shared_ptr<param_holder<T>> m_holder;
};

template <class T>
param<T> new_param(const T& v)
{
    return param<T>(v, TypeName<T>::Get());
}

template <class T>
param<T> new_param()
{
    return param<T>(TypeName<T>::Get());
}

// Disable connecting two values
template <class C1, class C2>
void attach(std::shared_ptr<param_holder<C1>>&, std::shared_ptr<param_holder<C2>>)
{
    static_assert(AlwaysFalse<C1>::value, "Connecting two parametric values is not allowed");
}

class ComputeNode : public DAGNode
{
public:

    template <typename T>
    static void AddInput(const std::shared_ptr<ComputeNode>& cn, const param<T>& p){
        attach(cn, p.node_pointer());
    }

    template <typename T>
    static void AddOutput(const std::shared_ptr<ComputeNode>& cn, const param<T>& p){
        attach(p.node_pointer(), cn);
    }

    virtual ~ComputeNode() {}

protected:
    ComputeNode()
        : DAGNode("")
    {
    }

};

/**
 * @brief This function creates a parametric version from a "normal" function
 *
 * The first argument is the function to be wrapped
 * It accepts a variable list of input arguments, each must be a parameter
 *
 * Here's an example:
 *
 * double mult(double a, double b)
 * {
 *   return a * b;
 * }
 *
 * parametric::param<double> a = parametric::new_param(2.0);
 * parametric::param<double> b = parametric::new_param(10.0);
 *
 *
 * auto result = eval_parametric(mult, a, b);
 */
template<typename Fn, typename... Args>
constexpr parametric::param<typename std::result_of<Fn(Args...)>::type>
eval(Fn wrapped_function, const parametric::param<Args>& ... parameterArgs)
{

    using rtype = typename std::result_of<Fn(Args...)>::type;

    class ComputeWrapperNode : public parametric::ComputeNode
    {
    public:
        ComputeWrapperNode(Fn&& ff, const std::tuple<parametric::param<Args>...>& t)
            : _wrapped_function(ff), _parameters(t)
            ,  _resultNode(parametric::new_param<rtype>())
        {}

        void eval() const
        {
            _resultNode.SetValue(
                apply(std::forward<const Fn>(_wrapped_function),
                  _parameters,
                  [](const auto& parm){return parm.Value();}
                )
             );
        }

        parametric::param<rtype> result() const
        {
            return _resultNode;
        }

    private:
        Fn _wrapped_function;
        std::tuple<parametric::param<Args>...> _parameters;
        mutable parametric::param<rtype> _resultNode;
    };


    std::tuple<parametric::param<Args>...> _parameters(parameterArgs...);

    auto computeNode = std::make_shared<ComputeWrapperNode>(std::forward<Fn>(wrapped_function), _parameters);
    // connect inputs
    static_foreach(_parameters, [&computeNode](const auto & parm) {
        ComputeNode::AddInput(computeNode, parm);
    });
    ComputeNode::AddOutput(computeNode, computeNode->result());

    return computeNode->result();
}

} // namespace parametric


#endif // PARAMETRIC_CORE_HPP
