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
#include <type_traits>
#include <utility>
#include <vector>
#include <exception>
#include <cassert>

namespace parametric
{

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

    using value_type = T;


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
    param(std::in_place_t, Args const&... args, std::string const& id)
        : m_holder(std::make_shared<impl::param_holder<T>>(std::in_place_t(), args..., id))
    {}

    /// @private
    const std::shared_ptr<impl::param_holder<T>> node_pointer() const
    {
        return m_holder;
    }

    const NodeRef compute_node() const {
        return m_holder->compute_node();
    }

    /**
     * @brief Returns the current value of the parameter
     */
    const T& value() const
    {
        return m_holder->value();
    }

    /**
     * @brief Write access to the value
     */
    T& change_value()
    {
        return m_holder->access_value();
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
        return m_holder->set_value(value);
    }

    /**
     * @brief Returns the parameter value
     */
    operator const T& () const
    {
        return value();
    }

    /**
     * @brief Sets the value of the parameter, see param::set_value
     */
    param<T>& operator=(const T& other)
    {
        m_holder->set_value(other);
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

    param<T> clone(
        std::shared_ptr<DAGNode::ClonedNodeMap> cloned_nodes = DAGNode::new_cloned_node_map()
    ) const 
    {
        auto p = param(
            std::static_pointer_cast<impl::param_holder<T>>(
                m_holder->clone(cloned_nodes)
            )
        );
        return p;
    }

private:
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
    return parametric::param<T>(std::in_place_t(), args..., id);
}

/// @private
/// Disable connecting two values
template <class C1, class C2>
void add_parent(const std::shared_ptr<parametric::impl::param_holder<C1>>&, const std::shared_ptr<parametric::impl::param_holder<C2>>)
{
    static_assert(AlwaysFalse<C1>::value, "Connecting two parametric values is not allowed");
}

/**
 * @brief This class is the base class to define arbitrary compute nodes.
 *
 * A compute node has to
 *  - inherit from parametric::ComputeNode<YourClass>
 *  - have a (private) parametric::param object for each input and output parameter
 *  - register the input and output parameters using ComputeNode::depends_on and ComputeNode::computes
 *  - override the ComputeNode::eval method to perform the computation
 *  - be constructed with parametric::new_node  (Args &&... args)
 *
 *  Example:
 *  \code{cpp}
 *  // define computing node
 *  class DivComputer : public parametric::ComputeNode<DivComputer>
 *  {
 *  public:
 *    DivComputer(const parametric::param<double>& op1, const parametric::param<double>& op2) : v1(op1), v2(op2) {
 *      this->depends_on(v1);
 *      this->depends_on(v2);
 *      this->computes(theresult, parametric::param<double>("result"));
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
template <typename... T>
using Arguments = std::tuple<param<T> const&...>;

template <typename... R>
using Results = std::tuple<param<R>...>;

// new ComputeNode implementation. Users must
// - inherit this class
// - override eval, using the arg an res member functions
template <typename Derived, typename Results, typename Arguments>
class ComputeNode : public ClonableDAGNode<Derived>
{
public:
    using results_type = Results;
    using arguments_type = Arguments;

    ComputeNode() : ClonableDAGNode<Derived>("") {};
    virtual ~ComputeNode(){}

    virtual void eval() const {};
    virtual void post_connect() const {};

    static Results initialize_results() {
        return initialize_results_impl(std::make_index_sequence<std::tuple_size_v<Results>>{});
    }

    template <int i>
    decltype(auto) arg() const { 
        assert(this->parents.size() == std::tuple_size_v<Arguments>);
        using value_type = typename std::decay_t<std::tuple_element_t<i, Arguments>>::value_type;
        return dynamic_cast<impl::param_holder<value_type>&>(*(this->parents[i]));
    }

    template <int i>
    decltype(auto) res() const { 
        assert(this->childs.size() == std::tuple_size_v<Results>);
        using value_type = typename std::tuple_element_t<i, Results>::value_type;
        return std::dynamic_pointer_cast<impl::param_holder<value_type>>(this->childs[i].lock());
    }

    decltype(auto) args_tuple() const {
        return args_tuple_impl(std::make_index_sequence<std::tuple_size_v<Arguments>>{});
    }
private:

    template <size_t... i>
    static Results initialize_results_impl(std::index_sequence<i...>) {
        return Results(parametric::new_param<typename std::tuple_element_t<i, Results>::value_type>()...);
    };

    template <size_t... i>
    decltype(auto) args_tuple_impl(std::index_sequence<i...>) const {
        return std::make_tuple(arg<i>().value()...);
    }
};

namespace {

    // Some template meta-programming to determine the return_type of parametric::compute
    template <typename R>
    struct compute_return_value_impl{};

    // For more than one output, parametric::compute returns a Results instance
    template <typename... Rs>
    struct compute_return_value_impl<Results<Rs...>> {
        using type = Results<Rs...>;
    };

    // For one output, parametric::compute returns a parameter - the first element of the Results instance
    template <typename R>
    struct compute_return_value_impl<Results<R>> {
        using type = std::tuple_element_t<0, Results<R>>;
    };

    // For no outputs, parametric::compute returns a shared_ptr to the DAGNode representing the compute node.
    template <>
    struct compute_return_value_impl<Results<>> {
        using type = std::shared_ptr<DAGNode>;
    };
}

template <typename C>
using compute_return_value = typename compute_return_value_impl<typename C::results_type>::type;

// factory function that returns all output parameters of a compute node
template <typename C, typename... Args>
compute_return_value<C> compute(std::shared_ptr<C> const& ptr, param<Args>... args)
{ 
    constexpr size_t nresults = std::tuple_size_v<typename C::results_type>;

    // make sure the post_connect callback is called at the end of this function using an RAII wrapper
    struct raii {
        std::shared_ptr<C> const& ptr;
        ~raii() {
            ptr->post_connect();
        }
    };
    raii r{ptr};

    // connect compute node to arguments
    (add_parent(ptr, args.node_pointer()), ...);

    if constexpr (nresults > 0) {

        // initialize the results tuple
        auto res = C::initialize_results();

        // connect results to compute node
        std::apply([&](auto& ...x){(..., add_parent(x.node_pointer(), ptr));}, res);

        if constexpr (nresults == 1) {
            // if there is just one result, don't return a tuple, but only a single parameter
            return std::get<0>(res);
        } else {
            return res;
        }
    } else {
        // if there is no result, simply return the compute node pointer after it has been connected to the inputs
        return ptr;
    }
}

template <typename C, typename... Args>
compute_return_value<C> compute(param<Args>... args)
{
    auto ptr = std::shared_ptr<C>();
    
    return compute(ptr, args...);
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
constexpr decltype(auto)
eval(Fn wrapped_function, const parametric::param<Args>& ... parameterArgs)
{

    using rtype = typename std::invoke_result<Fn, Args...>::type;

    class ComputeWrapperNode
     : public parametric::ComputeNode<
        ComputeWrapperNode,
        Results<rtype>,
        Arguments<Args...>
       >
    {
    public:

        ComputeWrapperNode(Fn f) : _wrapped_function(f) {}
        
        void eval() const override final
        {
            if (auto r = this->template res<0>(); r)
                r->set_value(
                    std::apply(
                        std::forward<const Fn>(_wrapped_function),
                        this->args_tuple()
                    )
                );
        }

    private:
        Fn _wrapped_function;
    };

    return compute(std::make_shared<ComputeWrapperNode>(wrapped_function), parameterArgs...);
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
    // just need a type derived from ClonableDAGNode<Connector>
    struct Connector : public ComputeNode<Connector,void, void> {};

    auto c = std::make_shared<Connector>();
    auto t = param<T>(the_struct, "");

    // connect c to arguments
    (add_parent(c, parametric_members.node_pointer()), ...);

    // connect t to c
    add_parent(t.node_pointer(), c);

    return t;
}

} // namespace parametric


#endif // PARAMETRIC_CORE_HPP
