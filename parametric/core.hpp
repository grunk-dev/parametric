#ifndef PARAMETRIC_CORE_HPP
#define PARAMETRIC_CORE_HPP

#include <parametric/typename.hpp>
#include <parametric/tupletools.hpp>
#include <parametric/dag.hpp>
#include <parametric/impl/core_impl.hpp>

#include <memory>
#include <vector>
#include <exception>
#include <cassert>

namespace parametric
{

/**
 * @param This class encapsulates an arbitrary type to be used
 * as a parameter.
 *
 * A parameter is an object with a value
 * and a valid state. Whenever its value has been changed,
 * it propagates this invalidation to all dependent parameters,
 * requiring their recomputation.
 *
 * Output interface parameters store a reference to an output parameter.
 * If an output parameter is not used in the code or no reference exist to it,
 * the interface output will be expired. Before setting an output value,
 * it has to be checked with ::Expired() for validity.
 */
template <typename T>
class param
{
public:
    param(const T& v, const std::string& id)
        : m_holder(std::make_shared<impl::param_holder<T>>(v, id))
    {}

    param(const std::string& id)
        : m_holder(std::make_shared<impl::param_holder<T>>(id))
    {}

    param(const std::shared_ptr<impl::param_holder<T>>& holder)
        : m_holder(holder)
    {}

    const std::shared_ptr<impl::param_holder<T>> node_pointer() const
    {
        return m_holder;
    }

    const T& Value() const
    {
        return m_holder->Value();
    }

    /**
     * @brief Sets the value of the parameter.
     *
     * The value will only be changed only, if the new value is different
     * than the old one.
     *
     * Note: The check for difference is done using the != operator. If a custom class
     * does not provide this operator, it can be added on global scope e.g.
     *
     * bool operator(const MyClass& c1, const MyClass& c2)
     * {
     *     return ...
     * }
     *
     * @param value The value to be set.
     */
    void SetValue(const T& value)
    {
        return m_holder->SetValue(value);
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

    void SetId(const std::string& id)
    {
        m_holder->SetId(id);
    }

    bool IsValid() const
    {
        return m_holder->IsValid();
    }

private:
    std::shared_ptr<impl::param_holder<T>> m_holder;
};

template <class T>
param<T> new_param(const T& v)
{
    return param<T>(v, TypeName<T>::Get());
}

template <class T>
param<T> new_param(const T& v, const std::string& name)
{
    return param<T>(v, name);
}


template <class T>
param<T> new_param()
{
    return param<T>(TypeName<T>::Get());
}

// Disable connecting two values
template <class C1, class C2>
void addParent(const std::shared_ptr<parametric::impl::param_holder<C1>>&, const std::shared_ptr<parametric::impl::param_holder<C2>>)
{
    static_assert(AlwaysFalse<C1>::value, "Connecting two parametric values is not allowed");
}

/**
 * @brief The interface parameter is used to define
 * the in- and outputs in ComputeNodes.
 *
 * Interface paramters thus define the parametric interface
 * of a compute node.
 */
template <class T>
class InterfaceParam
{
public:
    InterfaceParam()
    {}

    explicit InterfaceParam(const parametric::param<T>& p)
        : p_holder(p.node_pointer())
    {}

    parametric::param<T> Param() const
    {
        return  parametric::param<T>(p_holder.lock());
    }

    operator parametric::param<T>() const
    {
        return Param();
    }

    InterfaceParam<T>& operator=(const parametric::param<T>& p)
    {
        p_holder = p.node_pointer();
        return *this;
    }

    InterfaceParam<T>& operator=(const InterfaceParam<T>& p) = delete;

    void SetValue(const T& v)
    {
        if (!Expired()) {
            Param().SetValue(v);
        }
    }

    const T& Value() const
    {
        assert(!Expired());
        return Param().Value();
    }

    operator const T& () const
    {
        return Value();
    }

    bool Expired() const
    {
        return p_holder.expired();
    }

private:
    std::weak_ptr<impl::param_holder<T>> p_holder;
};

/**
 * @brief This class is the base class to define arbitrary compute nodes.
 *
 * A valid compute node has to
 *  - Have A (private) parametric::InterfaceParam object for each input and output parameter
 *  - Override the ```eval() const``` method to perform the computation
 *  - Must register the input and output parameters using ::DefineInput and ::DefineOutput
 *
 *  Example:
 *  \code{.cpp}
 *      class DivComputer : public parametric::ComputeNode
 *      {
 *      public:
 *          DivComputer(const parametric::param<double>& op1, const parametric::param<double>& op2) : v1(op1), v2(op2) {
 *              DefineInput(v1);
 *              DefineInput(v2);
 *              DefineOutput(_result, parametric::param<double>("result"));
 *           }
 *
 *          parametric::param<double> result() const {return _result;}
 *
 *          void eval() const {
 *              if (!_result.Expired())
 *                  _result.SetValue(v1.Value() / v2.Value());
 *          }
 *
 *      private:
 *          const parametric::InterfaceParam<double> v1, v2;
 *          mutable parametric::InterfaceParam<double> _result;
 *   \endcode
 */
class ComputeNode : public DAGNode
{
public:
    virtual ~ComputeNode() {}

    template <typename T>
    void DefineInput(const InterfaceParam<T>& p)
    {
        inputs.push_back(p.Param().node_pointer());
    }

    template <typename T>
    void DefineOutput(parametric::InterfaceParam<T>& intf_param, const parametric::param<T>& initial)
    {
        intf_param = initial;
        outputs.push_back(initial.node_pointer());
    }

    static void Connect(const std::shared_ptr<ComputeNode>& c)
    {
        assert(c);
        for (const auto& input : c->inputs) {
            addParent(c, input);
        }
        for (const auto& output : c->outputs) {
            addParent(output, c);
        }
    }

    static void ReleaseNodes(const std::shared_ptr<ComputeNode>& c)
    {
        if (!c) return;

        c->inputs.clear();
        c->outputs.clear();
    }

protected:
    ComputeNode()
        : DAGNode("")
    {
    }

    std::vector<std::shared_ptr<DAGNode> > inputs;
    std::vector<std::shared_ptr<DAGNode> > outputs;
};

/**
 * This class handles to correct memory management of compute nodes
 *
 * It ensures the correct connection if inputs and outputs
 * To create a compute node, use ```parametric::new_node<MyComputeNode>(arg1, arg2)```
 */
template <class T>
class compute_node_ptr
{
public:


    template<typename ... Args>
    compute_node_ptr(T* t)
        : wrapped(t)
    {
        T::Connect(wrapped);
    }

    std::shared_ptr<T> operator->() {
        return wrapped;
    }

    ~compute_node_ptr()
    {
        T::ReleaseNodes(wrapped);
    }

private:
    std::shared_ptr<T> wrapped;
};

/**
 * @brief Similar to std::shared_ptr, this function
 * should be used to correctly create a compute node
 *
 * Example:
 *   ```parametric::new_node<MyComputeNode>(arg1, arg2)```
 */
template <typename T, typename ... Args>
compute_node_ptr<T> new_node(Args&& ... args)
{
    return compute_node_ptr<T>(new T(std::forward<Args>(args) ... ));
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
constexpr parametric::param<typename std::result_of<Fn(Args...)>::type>
eval(Fn wrapped_function, const parametric::param<Args>& ... parameterArgs)
{

    using rtype = typename std::result_of<Fn(Args...)>::type;

    class ComputeWrapperNode : public parametric::ComputeNode
    {
    public:
        ComputeWrapperNode(Fn&& ff, const parametric::param<Args>&... t)
            : _wrapped_function(ff), _parameters(t...)
            , _resultNode(parametric::new_param<rtype>())
        {
            // connect inputs
            static_foreach(_parameters, [this](const auto & parm) {
                DefineInput(parm);
            });
            DefineOutput(_resultNode, parametric::param<rtype>("result"));
        }

        void eval() const
        {
            if(!_resultNode.Expired()) {
                _resultNode.SetValue(
                    apply(std::forward<const Fn>(_wrapped_function),
                      _parameters,
                      [](const auto& parm){return parm.Value();}
                    )
                 );
            }
        }

        parametric::param<rtype> result() const
        {
            return _resultNode;
        }

    private:
        Fn _wrapped_function;
        std::tuple<parametric::InterfaceParam<Args>...> _parameters;
        mutable parametric::InterfaceParam<rtype> _resultNode;
    };

    auto computeNode = new_node<ComputeWrapperNode>(std::forward<Fn>(wrapped_function),
                                                    std::forward<const parametric::param<Args>&>(parameterArgs)...);
    return computeNode->result();
}

} // namespace parametric


#endif // PARAMETRIC_CORE_HPP
