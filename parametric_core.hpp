#ifndef PARAMETRIC_CORE_HPP
#define PARAMETRIC_CORE_HPP

#include <memory>
#include <vector>
#include <algorithm>
#include <exception>
#include <cassert>

#include <type_traits> // for std::false_type

namespace TupleTools_private
{

//! helper template function for the actual implementation of a compile-time for loop
template<typename GenericLambda, std::size_t ... Is>
constexpr void static_for_impl(GenericLambda&& f, std::index_sequence<Is...>)
{
    // unpack into std::initializer list for "looping" in correct order without recursion
    (void)std::initializer_list<char> {((void)f(std::integral_constant<unsigned, Is>()), '0')...};
}

template <class Tuple, class F, typename ElemFun, size_t... Is>
constexpr auto apply_impl(F f, Tuple t, ElemFun ef,
                          std::index_sequence<Is...>)
{
    return f(ef(std::get<Is>(t))...);
}


}
template<unsigned N, typename GenericLambda>
constexpr void static_for(GenericLambda&& f)
{
    TupleTools_private::static_for_impl(std::forward<GenericLambda>(f), std::make_index_sequence<N>());
}

template<typename... _Elements, typename GenericLambda>
constexpr void static_foreach(std::tuple<_Elements...>& aTuple, GenericLambda&& f)
{
    constexpr auto N = std::tuple_size<std::tuple<_Elements...>>::value;
    static_for<N>([&aTuple, &f](auto i) {
        f(std::get<i>(aTuple));
    });
}

template <class F, class Tuple, typename ElemFun>
constexpr auto apply(F f, Tuple t, ElemFun ef)
{
    return TupleTools_private::apply_impl(
               f, t, ef, std::make_index_sequence < std::tuple_size<Tuple> {} > {});
}

namespace parametric
{

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

    operator T const& () const
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

    virtual ~DAGNode()
    {
        for (auto parent : parents) {
            if (parent) {
                unattachFrom(*parent);
            }
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

    virtual ~ComputeNode() {}

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
class param_holder : public ValueType
{
public:
    param_holder(const ResultType& v)
        : ValueType(1)
        , value(v)
    {
    }
    param_holder()
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
class param
{
public:
    param(const T& v)
        : m_holder(std::make_shared<param_holder<T>>(v))
    {}

    param()
        : m_holder(std::make_shared<param_holder<T>>())
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
    return param<T>(v);
}

template <class T>
param<T> new_param()
{
    return param<T>();
}

// Disable connecting two values
template <class C1, class C2>
void attach(std::shared_ptr<param_holder<C1>>&, std::shared_ptr<param_holder<C2>>)
{
    static_assert(AlwaysFalse<C1>::value, "Connecting two parametric values is not allowed");
}

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
parametric::param<typename std::result_of<Fn(Args...)>::type>
eval(Fn wrapped_function, const parametric::param<Args>& ... parameterArgs)
{

    using rtype = typename std::result_of<Fn(Args...)>::type;

    class ComputeWrapperNode : public parametric::ComputeNode
    {
    public:
        ComputeWrapperNode(Fn ff, parametric::param<Args>... args)
            : _wrapped_function(ff), _parameters(args...)
            ,  _resultNode(parametric::new_param<rtype>())
        {
            // define input and output arguments
            define_output(_resultNode.node_pointer());
            static_foreach(_parameters, [this](const auto & parm) {
                define_input(parm.node_pointer());
            });

        }

        void compute()
        {
            _resultNode.SetValue(
                apply(
                    std::forward<Fn>(_wrapped_function),
                    _parameters,
            [](const auto & parm) {
                return parm.Value();
            }
                ));
        }

        parametric::param<rtype> result() const
        {
            return _resultNode;
        }

    private:
        Fn _wrapped_function;
        std::tuple<parametric::param<Args>...> _parameters;
        parametric::param<rtype> _resultNode;
    };


    std::shared_ptr<ComputeWrapperNode> computeNode(new ComputeWrapperNode(wrapped_function, parameterArgs...));
    ComputeWrapperNode::connect_ins_outs(computeNode);


    return computeNode->result();
}


} // namespace parametric


#endif // PARAMETRIC_CORE_HPP
