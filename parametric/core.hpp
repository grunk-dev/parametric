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

    /**
     * @brief returns a shared_ptr to the compute node, if this param is a computation result. If 
     * it is an independent parameter, the returned pointer will be null.
     * 
     * @return const NodeRef 
     */
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

/// @private 
/// A type representing the input arguments of a compute node
template <typename... T>
using Arguments = std::tuple<param<T> const&...>;

/// @private
/// A type representing the ouptut arguments of a compute node
template <typename... R>
using Results = std::tuple<param<R>...>;

/**
 * @brief This class is the base class to define arbitrary compute nodes.
 *
 * A compute node has to
 *  - inherit from parametric::ComputeNode<YourClass, parametric::Results<ResultType1, ResultType2, ...>, parametric::Arguments<ArgType1, ArgType2, ...>>
 *  - be constructed with parametric::compute
 * 
 * A compute node may
 *  - override the ComputeNode::eval method to perform the computation
 *  - override the ComputeNode::post_connect callback to perform some customization after the compute node has been connected to its inputs and outputs, e.g. setting default ids.
 *
 * A compute node may have any number of inputs and outputs
 * 
 * After a compute node has been connected to its inputs and outputs, argument parameters can be queried with ComputeNode::arg<i>() in the overwritten methods. 
 * 
 * Pointers to a result can be queried with ComputeNode::res<i>(). Since it may be possible that a result goes out of scope before the compute node, 
 * pointers queried with ComputeNode::res<i>() may be nullptr. This should be checked in the overriden methods before using the pointer. 
 *
 *  Example:
 *  \code{cpp}
 *  // define computing node
 *  class DivComputer : public parametric::ComputeNode<DivComputer, parametric::Results<double>, parametric::Arguments<double, double>>
 *  {
 *    void eval() const override {
 *        if (auto result = res<0>(); result) {
 *            result->set_value(arg<0>().value() / arg<1>().value());
 *        }
 *    }
 *
 *    void post_connect const override {
 *        if (auto result = res<0>(); result) {
 *            result->set_id("reosonable_default_id");
 *        }
 *    }
 *  };
 *  \endcode
 *
 * The DivComputer compute node is then used as follows
 *
 * \code{cpp}
 *  auto p1 = parametric::new_param(2.0, "p1");
 *  auto p2 = parametric::new_param(5.0, "p2");
 *  auto res = parametric::compute<DivComputer>(p1, p2);
 * \endcode
 */
template <typename Derived, typename Results, typename Arguments>
class ComputeNode : public ClonableDAGNode<Derived>
{
public:
    using results_type = Results;
    using arguments_type = Arguments;

    ComputeNode() : ClonableDAGNode<Derived>("") {};
    virtual ~ComputeNode(){}

    /**
     * @brief override this method to perform the calculation. 
     * 
     * Use arg<i>() and res<i>() to query the inputs and outputs
     */
    virtual void eval() const {};

    /**
     * @brief override this method as a post connection callback. 
     *
     * This method will be called immediately after the compute node has been
     * connected to its inputs and outputs. It can be used to set a default id 
     * for the outputs using arg<i>() and res<i>().
     * 
     */
    virtual void post_connect() const {};

    ///@private 
    /// initializes the results. This is called once in parametric::compute before the compute node
    /// is connected to its inputs and outputs
    static Results initialize_results() {
        return initialize_results_impl(std::make_index_sequence<std::tuple_size_v<Results>>{});
    }

    /**
     * @brief returns the i-th input argument. This function must be called after the compute node has been 
     * connected to its inputs via parametric::compute. 
     * 
     * @tparam i the index of the input argument.
     * @return A reference to an input parameter
     */
    template <int i>
    decltype(auto) arg() const { 
        assert(this->parents.size() == std::tuple_size_v<Arguments>);
        using value_type = typename std::decay_t<std::tuple_element_t<i, Arguments>>::value_type;
        return dynamic_cast<impl::param_holder<value_type>&>(*(this->parents[i]));
    }

    /**
     * @brief returns a pointer to the i-th output argument. The function must be called after the compute node has been
     * connected to its outputs via parametric::compute.
     * 
     * If the output parameter has already been deleted, the returned pointer will be null. This should be checked before 
     * using the pointer.
     *
     * Example:
     * \code{cpp}
     * if (auto result = res<0>(); result) {
     *     result->set_value(arg<0>().value() + arg<1>().value());
     * }
     * \endcode
     * 
     * @tparam i the index of the output argument.
     * @return A shared_ptr to the output parameter
     */
    template <int i>
    decltype(auto) res() const { 
        assert(this->childs.size() == std::tuple_size_v<Results>);
        using value_type = typename std::tuple_element_t<i, Results>::value_type;
        return std::dynamic_pointer_cast<impl::param_holder<value_type>>(this->childs[i].lock());
    }

    /**
     * @brief A convenience function that collects the input arguments in a tuple. 
     * 
     * This can be used to forward the input arguments to std::apply for instance.
     * 
     * @return A tuple of the evaluated input arguments. 
     */
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
    template <typename R, typename... Rs>
    struct compute_return_value_impl<Results<R, Rs...>> {
        using type = Results<R, Rs...>;
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

/**
 * @brief This function should be used to connect compute nodes.
 *
 * - it initializes the outputs of a compute node, if any.
 * - it connects the compute node to its inputs and outputs
 * - it calls the ComputeNode::post_connect callback.
 *
 * @tparam C a class derived from parametric::ComputeNode<C, parametric::Results<R...>, parametric::Arguments<A...>>
 * @tparam Args The types of the input arguments
 * @param ptr A shared_ptr to a C instance. It is assumed that this compute node has not been connected to its inputs and outputs yet.
 * @param args The input parameters.
 * @return There are three possible return types of this function depending on the return types of C:
 *         - If there are more than one output, it returns a parametric::Results<R1, R2, ...> instance. This is a tuple of output parameters.
 *         - If there is one ouput, it returns a parametric::param<R> instead of a tuple.
 *         - If there is no output, it returns ptr.
 */
template <typename C, typename... Args>
compute_return_value<C> compute(std::shared_ptr<C> const& ptr, param<Args> const&... args)
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

/**
 * @brief This function should be used to connect compute nodes.
 *
 * It is a convenience wrapper around compute node instances that are default constructible. If your class is not default
 * constructible, you can provide a specialization as a convenience to your users. 
 *
 * Example
 * \code{cpp}
  *  // define computing node
 *  class Add : public parametric::ComputeNode<Add, parametric::Results<double>, parametric::Arguments<double, double>>
 *  {
 *  public:
 * 
 *    void eval() const override {
 *        if (auto result = res<0>(); result) {
 *            result->set_value(arg<0>().value() + arg<1>().value());
 *        }
 *    }
 *
 *    void post_connect const override {
 *        if (auto result = res<0>(); result) {
 *            result->set_id("reosonable_default_id");
 *        }
 *    }
 *  };
 * 
 *  auto l = parametric::new_param(0.5);
 *  auto l = parametric::new_param(0.2);
 *  auto r = compute<Add>(l, r);
 * \endcode
 *
 * @tparam C a class derived from parametric::ComputeNode<C, parametric::Results<R...>, parametric::Arguments<A...>>
 * @tparam Args The types of the input arguments
 * @param args The input parameters.
 * @return There are three possible return types of this function depending on the return types of C:
 *         - If there are more than one output, it returns a parametric::Results<R1, R2, ...> instance. This is a tuple of output parameters.
 *         - If there is one ouput, it returns a parametric::param<R> instead of a tuple.
 *         - If there is no output, it returns ptr.
 */
template <typename C, typename... Args>
compute_return_value<C> compute(param<Args> const&... args)
{
    auto ptr = std::make_shared<C>();
    
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

    if constexpr (!std::is_void_v<rtype>) {

        class NonVoidComputer
        : public parametric::ComputeNode<NonVoidComputer, parametric::Results<rtype>, parametric::Arguments<Args...>>
        {
        public:

            NonVoidComputer(Fn f) : _wrapped_function(f) {}
            
            void eval() const override final
            {
                if (auto r = this->template res<0>(); r) {
                    // handle non-void-function
                    r->set_value(
                        std::apply(
                            std::forward<const Fn>(_wrapped_function),
                            this->args_tuple()
                        )
                    );
                }
            }

        private:
            Fn _wrapped_function;
        };

        return compute(std::make_shared<NonVoidComputer>(wrapped_function), parameterArgs...);

    } else {

        class VoidComputer
        : public parametric::ComputeNode<VoidComputer, parametric::Results<>, parametric::Arguments<Args...>>
        {
        public:

            VoidComputer(Fn f) : _wrapped_function(f) {}
            
            void eval() const override final
            {
                // handle void-function
                std::apply(
                    std::forward<const Fn>(_wrapped_function),
                    this->args_tuple()
                );
            }

        private:
            Fn _wrapped_function;
        };

        return compute(std::make_shared<VoidComputer>(wrapped_function), parameterArgs...);
    }
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
