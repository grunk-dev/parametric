/**
 * @file core.hpp
 */
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

template <typename T>
class OutputParam;

/**
 * @brief This class encapsulates an arbitrary type to be used
 * as a parameter.
 *
 * A parameter is an object with a value
 * and a valid state. Whenever its value has been changed,
 * it propagates this invalidation to all dependent parameters,
 * requiring their recomputation.
 */
template <typename T>
class param
{
public:
    /**
     * @brief Creates a **valid** parameter from a value and an identifier.
     *
     * After construction, the parameter is valid (it contains a value)
     */
    param(const T& v, const std::string& id)
        : m_holder(std::make_shared<impl::param_holder<T>>(v, id))
    {}

    /**
     * @brief Creates a **invalid** parameter with an identifier.
     *
     * After construction, the parameter is invalid (it does not contain a value)
     */
    param(const std::string& id)
        : m_holder(std::make_shared<impl::param_holder<T>>(id))
    {}

    template <typename... Args>
    param(in_place_t, Args const&... args, std::string const& id)
        : m_holder(std::make_shared<impl::param_holder<T>>(in_place_t(), args..., id))
    {}

    /// @private
    const std::shared_ptr<impl::param_holder<T>> node_pointer() const
    {
        return m_holder;
    }

    /**
     * @brief Returns the current value of the parameter
     */
    const T& value() const
    {
        return m_holder->Value();
    }

    /**
     * @brief Write access to the value
     */
    T& change_value()
    {
        return m_holder->AccessValue();
    }

    /**
     * @brief Sets the value of the parameter.
     *
     * The value will only be changed only, if the new value is different
     * than the old one. The parameter will be **valid** after setting the value.
     *
     * Note: The check for difference is done using the != operator. If a custom class
     * does not provide this operator, it can be added on global scope e.g.
     *
     * \code{cpp}
     * bool operator(const MyClass& c1, const MyClass& c2)
     * {
     *     return ...
     * }
     * \endcode
     *
     * @param value The value to be set.
     */
    void set_value(const T& value)
    {
        return m_holder->SetValue(value);
    }

    /**
     * @brief Returns the parameter value
     */
    operator const T& () const
    {
        return value();
    }

    /**
     * @brief Sets the value of the parameter, see param::SetValue
     */
    param<T>& operator=(const T& other)
    {
        m_holder->SetValue(other);
        return *this;
    }

    /**
     * @brief Gets the identifier of the parameter
     */
    std::string id() const
    {
        return m_holder->id();
    }

    /**
     * @brief Sets the identifier of the parameter
     */
    void set_id(const std::string& id)
    {
        m_holder->set_id(id);
    }

    /**
     * @brief Indicates, whether the parameter contains a value (valid) or not (invalid)
     */
    bool is_valid() const
    {
        return m_holder->IsValid();
    }

private:
    friend OutputParam<T>;
    param(const std::shared_ptr<impl::param_holder<T>>& holder)
        : m_holder(holder)
    {}

    std::shared_ptr<impl::param_holder<T>> m_holder;
};

/**
 * @brief Convenience function to create a parameter of type T with value v
 * and identifier id
 */
template <class T>
param<T> new_param(const T& v, const std::string& id)
{
    return param<T>(v, id);
}

/**
 * @brief Convenience function to create a parameter of type T with value v
 */
template <class T>
param<T> new_param(const T& v)
{
    return parametric::new_param(v, TypeName<T>::Get());
}

/**
 * @brief Convenience function to create an empty parameter of
 * and identifier id
 */
template <class T>
param<T> new_param()
{
    return param<T>(TypeName<T>::Get());
}

/**
 * @brief Convenience function create a parameter of type T with value v.
 * The value will be constructed in place, thus avoiding unncessary copies
 * 
 * @tparam T the type held by the parameter
 * @tparam Args Constructor argument types of T
 * @param args the arguments forwarded to the constructor of T
 * @param id the identifier id
 * @return param<T> the created parameter
 */
template <typename T, typename... Args>
param<T> make_param(Args const&... args, std::string const& id){
    static_assert(std::is_constructible<T, Args...>::value, "make_param: T is not constructible from given arguments");
    return parametric::param<T>(in_place_t(), args..., id);
}

/// @private
/// Disable connecting two values
template <class C1, class C2>
void add_parent(const std::shared_ptr<parametric::impl::param_holder<C1>>&, const std::shared_ptr<parametric::impl::param_holder<C2>>)
{
    static_assert(AlwaysFalse<C1>::value, "Connecting two parametric values is not allowed");
}

/**
 * @brief The output parameter is used to define
 * theoutputs in ComputeNodes.
 *
 * Technically output parameters store a reference the resulting output parametric::param.
 * If an output parameter is not used in the code or no reference exist to it,
 * the output will be expired. Before setting an output value,
 * it has to be checked with OutputParam::Expired() for validity.
 */
template <class T>
class OutputParam
{
public:
    /**
     * @brief Creates output parameter placeholder
     */
    OutputParam()
    {}

    /**
     * @brief Creates output parameter referencing parameter p
     */
    explicit OutputParam(const parametric::param<T>& p)
        : p_holder(p.node_pointer())
    {}

    /**
     * @brief Returns the resulting parameter from the interface
     */
    const parametric::param<T> param() const
    {
        return  parametric::param<T>(p_holder.lock());
    }

    /**
     * @brief Returns the resulting parameter from the output
     */
    parametric::param<T> param()
    {
        return  parametric::param<T>(p_holder.lock());
    }


    /**
     * @brief Conversion to a parameter
     */
    operator parametric::param<T>() const
    {
        return param();
    }

    /**
     * @brief Assigns the parameter p to the output
     */
    OutputParam<T>& operator=(const parametric::param<T>& p)
    {
        p_holder = p.node_pointer();
        return *this;
    }

    /// @private
    OutputParam<T>& operator=(const OutputParam<T>& p) = delete;

    /**
     * @brief Sets the value of the parameter, equal to param::SetValue
     */
    void set_value(const T& v)
    {
        if (!expired()) {
            p_holder.lock()->SetValue(v);
        }
    }

    /**
     * @brief Returns the value of the parameter, equal to param::Value
     */
    const T& value() const
    {
        assert(!expired());
        return p_holder.lock()->Value();
    }

    /**
     * @brief Returns the value of the parameter, equal to param::Value
     */
    operator const T& () const
    {
        return value();
    }

    /**
     * @brief Expired returns, whether the reference to the parameter node is still valid
     */
    bool expired() const
    {
        return p_holder.expired();
    }

    /**
     * @brief Clears the value of the output parameter
     *
     * This is required in the initialization of output parameters
     */
    void invalidate()
    {
        p_holder.lock()->Clear();
    }

private:
    std::weak_ptr<impl::param_holder<T>> p_holder;
};

/**
 * @brief This class is the base class to define arbitrary compute nodes.
 *
 * A compute node has to
 *  - inherit from parametric::ComputeNode
 *  - have a (private) parametric::param object for each input and output parameter
 *  - register the input and output parameters using ComputeNode::depends_on and ComputeNode::computes
 *  - override the ComputeNode::eval method to perform the computation
 *  - be constructed with parametric::new_node  (Args &&... args)
 *
 *  Example:
 *  \code{cpp}
 *  // define computing node
 *  class DivComputer : public parametric::ComputeNode
 *  {
 *  public:
 *    DivComputer(const parametric::param<double>& op1, const parametric::param<double>& op2) : v1(op1), v2(op2) {
 *      depends_on(v1);
 *      depends_on(v2);
 *      computes(theresult, parametric::param<double>("result"));
 *    }
 *
 *    parametric::param<double> result() const {return theresult;}
 *
 *    void eval() const {
 *        if (theresult.expired() == false)
 *          theresult.set_value(v1.value() / v2.value());
 *    }
 *
 *  private:
 *    const parametric::param<double> v1, v2;
 *    mutable parametric::OutputParam<double> theresult;
 *  };
 *  \endcode
 *
 * The DivComputer compute node is then used as follows
 *
 * \code{cpp}
 *  auto p1 = parametric::new_param(2.0, "p1");
 *  auto p2 = parametric::new_param(5.0, "p2");
 *  auto computer = parametric::new_node<DivComputer>(p1, p2);
 *  auto result = computer->result();
 * \endcode
 */
class ComputeNode : public DAGNode
{
public:
    virtual ~ComputeNode() {}

    /**
     * @brief depends_on must be called in the derived classes constructor
     * to register input parameters.
     */
    template <typename T>
    void depends_on(const parametric::param<T>& p)
    {
        inputs.push_back(p.node_pointer());
    }

    /**
     * @brief computes must be called in the derived classes constructor
     * to register output parameters.
     */
    template <typename T>
    void computes(parametric::OutputParam<T>& intf_param, const parametric::param<T>& initial)
    {
        intf_param = initial;
        intf_param.invalidate();
        outputs.push_back(initial.node_pointer());
    }

    /// @private
    static void connect(const std::shared_ptr<ComputeNode>& c)
    {
        assert(c);
        for (const auto& input : c->inputs) {
            add_parent(c, input);
        }
        for (const auto& output : c->outputs) {
            add_parent(output, c);
        }
    }

    /// @private
    static void release_nodes(const std::shared_ptr<ComputeNode>& c)
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

    std::vector<std::shared_ptr<DAGNode> > inputs;  /**< @private */
    std::vector<std::shared_ptr<DAGNode> > outputs; /**< @private */
};

/**
 * @brief This class handles to correct memory management of compute nodes
 *
 * It ensures the correct connection if inputs and outputs
 * To create a compute node, use ```parametric::new_node<MyComputeNode>(arg1, arg2)```
 */
template <class T>
class compute_node_ptr
{
public:

    /**
     * Wraps a raw pointer to a compute node
     *
     * @param t Pointer to a compute node created with ``new``
     */
    template<typename ... Args>
    compute_node_ptr(T* t)
        : wrapped(t)
    {
        T::connect(wrapped);
    }

    /**
     * @brief The operator allows to access the wrapped objects members
     */
    std::shared_ptr<T> operator->() {
        return wrapped;
    }

    ~compute_node_ptr()
    {
        if (wrapped.use_count() == 1) {
            T::release_nodes(wrapped);
        }
    }

private:
    std::shared_ptr<T> wrapped;
};

/**
 * @brief Similar to std::shared_ptr, this function
 * should be used to correctly create a compute node
 *
 * Example:
 * \code{cpp}
 * parametric::new_node<MyComputeNode>(arg1, arg2)
 * \endcode
 */
template <typename T, typename ... Args>
parametric::compute_node_ptr<T> new_node(Args&& ... args)
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
 * \code{cpp}
 * double mult(double a, double b)
 * {
 *   return a * b;
 * }
 *
 * parametric::param<double> a = parametric::new_param(2.0);
 * parametric::param<double> b = parametric::new_param(10.0);
 *
 * auto result = parametric::eval(mult, a, b);
 * \endcode
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
                depends_on(parm);
            });
            computes(_resultNode, parametric::param<rtype>("result"));
        }

        void eval() const
        {
            if(!_resultNode.expired()) {
                _resultNode.set_value(
                    apply(std::forward<const Fn>(_wrapped_function),
                      _parameters,
                      [](const auto& parm){return parm.value();}
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
        std::tuple<parametric::param<Args>...> _parameters;
        mutable parametric::OutputParam<rtype> _resultNode;
    };

    auto computeNode = new_node<ComputeWrapperNode>(std::forward<Fn>(wrapped_function),
                                                    parameterArgs...);
    return computeNode->result();
}

/**
 * @brief This function creates a parametric version from a structure
 * or an object that consists of parameters
 *
 * It propagates mutation of the member parameters to the object itself.
 *
 * The first argument is the structure/object to be wrapped,
 * the following arguments are all parameters, which the object depends on
 *
 * Here's an example:
 *
 * \code{cpp}
 * struct AStruct
 * {
 *   parametric::param<int> a{10, "a"};
 *   parametric::param<int> b{100, "a"};
 * };
 *
 * auto structSum = [](const AStruct& s) -> int {
 *   return s.a.value() + s.b.value();
 * };
 *
 * AStruct s;
 * auto parametricS = parametric::new_parametric_struct(s, s.a, s.b);
 * auto sum = parametric::eval(structSum, parametricS);
 * \endcode
 *
 * The best practice is to create a helper function for each structure
 * that contains parameters by creating a template specialization
 * of parametric::new_param as follows:
 *
 * \code{cpp}
 * template<>
 * parametric::param<AStruct> parametric::new_param(const AStruct& s)
 * {
 *   return parametric::new_parametric_struct(s, s.a, s.b);
 * }
 * \endcode
 */
template<class T, typename... Args>
parametric::param<T>
new_parametric_struct(const T& the_struct, const parametric::param<Args>& ... parametric_members)
{
    class ParametricStructBuilder : public  parametric::ComputeNode
    {
    public:
        ParametricStructBuilder(const T& parms, const parametric::param<Args>&... t ) {
            // connect inputs
            std::tuple<const parametric::param<Args>&...> _parameters(t...);

            static_foreach(_parameters, [this](const auto & parm) {
                this->depends_on(parm);
            });
            computes(out, parametric::param<T>(parms, TypeName<T>::Get()));
        }

        parametric::param<T> result()
        {
            return out.param();
        }

    private:
        mutable parametric::OutputParam<T> out;
    };

    auto computeNode = parametric::new_node<ParametricStructBuilder>(the_struct, parametric_members...);
    return computeNode->result();
}

} // namespace parametric


#endif // PARAMETRIC_CORE_HPP
