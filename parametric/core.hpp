// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
// SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

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
template <typename T, typename S = DefaultSerializer>
class param
{
public:

    using value_type = T;
    using serializer_type = S;
    using node_type = impl::param_holder<T,S>;


    /**
     * @brief Creates a **valid** parameter from a value and an identifier.
     *
     * After construction, the parameter is valid (it contains a value)
     */
    param(const std::string& id, const T& v)
        : m_holder(std::make_shared<node_type>(id, v))
    {}

    /**
     * @brief Creates a **invalid** parameter with an identifier.
     *
     * After construction, the parameter is invalid (it does not contain a value)
     */
    explicit param(const std::string& id)
        : m_holder(std::make_shared<node_type>(id))
    {}

    template <typename... Args>
    param(std::string const& id, std::in_place_t, Args const&... args)
        : m_holder(std::make_shared<node_type>(id, std::in_place_t(), args...))
    {}

    /// @private
    const std::shared_ptr<node_type> node_pointer() const
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
    param& operator=(const T& other)
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

    param clone(
        std::shared_ptr<DAGNode::ClonedNodeMap> cloned_nodes = DAGNode::new_cloned_node_map()
    ) const 
    {
        auto p = param(
            std::static_pointer_cast<node_type>(
                m_holder->clone(cloned_nodes)
            )
        );
        return p;
    }

private:
    param(const std::shared_ptr<node_type>& holder)
        : m_holder(holder)
    {}


    std::shared_ptr<node_type> m_holder;
};

/**
 * @brief Convenience function to create a parameter of type T with value v
 * and identifier id
 */
template <class T, typename S=DefaultSerializer>
param<T, S> new_param(const std::string& id, const T& v)
{
    return param<T, S>(id, v);
}

/**
 * @brief Convenience function to create a parameter of type T with value v
 */
template <class T, typename S=DefaultSerializer>
param<T, S> new_param(const T& v)
{
    return parametric::new_param<T, S>(TypeName<T>::Get(), v);
}

/**
 * @brief Convenience function to create an empty parameter of
 * and identifier id
 */
template <class T, typename S=DefaultSerializer>
param<T, S> new_param()
{
    return param<T, S>(TypeName<T>::Get());
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
template <typename T, typename S=DefaultSerializer, typename... Args>
param<T, S> make_param(std::string const& id, Args const&... args){
    static_assert(std::is_constructible<T, Args...>::value, "make_param: T is not constructible from given arguments");
    return parametric::param<T, S>(id, std::in_place_t(), args...);
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
template <typename S=DefaultSerializer>
struct ComputeNodeTraits {

private:
    template <bool=true, typename... Ts>
    struct ResultsImpl 
    {
        using type = std::tuple<param<Ts, S>...>;
    };

    template <bool dummy, typename... Ts>
    struct ResultsImpl<dummy, std::tuple<Ts...>>
    {
        using type = std::tuple<param<Ts, S>...>;
    };

    template <bool dummy>
    struct ResultsImpl<dummy, void>
    {
        using type = std::tuple<>;
    };
public:

    template <typename... T>
    using Arguments = std::tuple<param<T, S> const&...>;

    template <typename... R>
    using Results = typename ResultsImpl<true, R...>::type;
};

template <typename... Ts>
using Arguments = typename ComputeNodeTraits<DefaultSerializer>::template Arguments<Ts...>;

template <typename... Ts>
using Results = typename ComputeNodeTraits<DefaultSerializer>::template Results<Ts...>;

namespace {

    // Some template meta-programming to determine the return_type of parametric::compute
    template <typename R>
    struct compute_return_value_impl
    {
        using type = std::shared_ptr<DAGNode>;
    };

    // For more than one output, parametric::compute returns a Results instance
    template <typename R, typename U, typename... Rs>
    struct compute_return_value_impl<std::tuple<R, U, Rs...>> {
        using type = std::tuple<R, U, Rs...>;
    };

    // For one output, parametric::compute returns a parameter - the first element of the Results instance
    template <typename R>
    struct compute_return_value_impl<std::tuple<R>> {
        using type = R;
    };

    template <>
    struct compute_return_value_impl<std::tuple<void>> {
        using type = std::shared_ptr<DAGNode>;
    };
}

template <typename T>
using compute_return_value = typename compute_return_value_impl<T>::type;

/**
 * @brief tag type used as default template type for results or arguments of ComputeNode. 
 */
struct ignore{};

/**
 * @brief This class is the base class to define arbitrary compute nodes.
 *
 * A compute node has to
 *  - inherit from parametric::ComputeNode<YourClass, parametric::Results<ResultType1, ResultType2, ...>, parametric::Arguments<ArgType1, ArgType2, ...>>
 *  - be constructed with parametric::compute
 * 
 * A compute node may
 *  - override the ComputeNode::eval method to perform the computation
 *  - customize parametric::compute by
 *     - providing a custom void connect_inputs(args...) function to connect inputs to the compute node
 *     - providing a custom R initialize_results() function to initialize the output parameters
 *     - providing a custom void connect_results(R const&) function to connect the outputs to the compute node
 *     - providing a custom void post_connect() callback to perform some customization after the compute node has been connected to its inputs and outputs, e.g. setting default ids.
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
template <typename Derived, typename Results = ignore, typename Arguments = ignore>
class ComputeNode
 : public ClonableDAGNode<Derived>
 , public std::enable_shared_from_this<ComputeNode<Derived, Results, Arguments>>
{
public:
    using results_type = Results;
    using arguments_type = Arguments;

    ComputeNode() : ClonableDAGNode<Derived>("") {};
    virtual ~ComputeNode(){}

    /**
     * @brief default implementation for connecting the inputs to the outputs.
     *
     * This function may be provided by the derived class to customize the behavior of 
     * ::parametric::compute
     * 
     * @tparam Args arguments forwarded from ::parametric::compute
     * @param args input arguments that shall be connected to the compute node
     */
    template <typename... Args>
    void connect_inputs(Args&&... args)
    {
        (this->depends_on(args), ...);
    }

    /**
     * @brief default implementation for initializing the results of the compute node
     *
     * This function may be provided by the derived class to customize the behavior of 
     * ::parametric::compute
     * 
     * @return compute_return_value<Results> a tuple of param<T> values for multi-output functions, 
     *         a param<T> for single output functions or a shared_ptr to this for void functions
     */
    compute_return_value<Results> initialize_results() {
        auto ret = initialize_results_impl(std::make_index_sequence<std::tuple_size_v<Results>>{});
        constexpr size_t nresults = std::tuple_size_v<Results>;
        if constexpr (nresults > 1) {
            return ret;
        } else if constexpr (nresults == 1) {
            return std::get<0>(ret);
        } else {
            auto ptr = this->shared_from_this();
            return ptr;
        }
    }

    /**
     * @brief default implementation for connecting the results of the compute node to the 
     * compute node.
     * 
     * This function may be provided by the derived class to customize the behavior of 
     * ::parametric::compute
     * 
     * @param res The results as returned by ::initialize_results
     */
    void connect_results(compute_return_value<Results> const& res)
    {
        constexpr size_t nresults = std::tuple_size_v<Results>;
        if constexpr (nresults > 0) {
            if constexpr (nresults > 1) {
                std::apply([&](auto& ...x){(..., this->computes(x));}, res);
            } else if constexpr (nresults == 1) {
                this->computes(res);
            }
        }
    }

    /**
     * @brief override this method as a post connection callback. 
     *
     * This method will be called immediately after the compute node has been
     * connected to its inputs and outputs. It can be used to set a default id 
     * for the outputs using arg<i>() and res<i>().
     * 
     */
    void post_connect() const {};

    /**
     * @brief override this method to perform the calculation. 
     * 
     * Use arg<i>() and res<i>() to query the inputs and outputs
     */
    virtual void eval() const {};

    /**
     * @brief returns the i-th input argument. This function must be called after the compute node has been 
     * connected to its inputs via parametric::compute. It ignores the Argument list and expects the argument 
     * type to be specified by the user. 
     *
     * Caution: UB if the value_type does not match the actual type of the i-th argument!
     * 
     * @tparam value_type the value_type of the i-th argument. 
     * @param i the index of the input argument.
     * @return A reference to an input parameter
     */
    template <typename value_type, typename S = DefaultSerializer>
    decltype(auto) arg(int i) const { 
        assert(i < this->parents.size());
        return dynamic_cast<impl::param_holder<value_type, S>&>(*(this->parents[i]));
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
        using serializer_type = typename std::decay_t<std::tuple_element_t<i, Arguments>>::serializer_type;
        return this->arg<value_type, serializer_type>(i);
    }

    /**
     * @brief returns a pointer to the i-th output argument. The function must be called after the compute node has been
     * connected to its outputs via parametric::compute.
     * 
     * If the output parameter has already been deleted, the returned pointer will be null. This should be checked before 
     * using the pointer. This overload ignores the Result list and requires the user to specify the type of the i-th result
     *
     * Caution: UB if the value_type does not match the actual type of the i-th result!
     *
     * Example:
     * \code{cpp}
     * if (auto result = res<double>(0); result) {
     *     result->set_value(arg<double>(0).value() + arg<double>(1).value());
     * }
     * \endcode
     * 
     * @tparam i the index of the output argument.
     * @return A shared_ptr to the output parameter
     */
    template <typename value_type, typename S=DefaultSerializer>
    decltype(auto) res(int i) const { 
        assert(i < this->childs.size());
        return std::dynamic_pointer_cast<impl::param_holder<value_type, S>>(this->childs[i].lock());
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
        using serializer_type = typename std::tuple_element_t<i, Results>::serializer_type;
        return this->res<value_type, serializer_type>(i);
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

    /**
     * @brief convenience function to connect this compute node to an input parameter
     * 
     * @tparam T the type held by the input parameter
     * @param input the input parameter
     */
    template <typename T, typename S>
    void depends_on(param<T, S> const& input) {
        auto ptr = this->shared_from_this();
        add_parent(ptr, input.node_pointer());
    }

    /**
     * @brief convenience function to connect this compute node to an output parameter
     * 
     * @tparam T the type held by the output parameter
     * @param output the output parameter
     */
    template <typename T, typename S>
    void computes(param<T, S> const& output) {
        auto ptr = this->shared_from_this();
        add_parent(output.node_pointer(), ptr);
    }

private:

    template <size_t i>
    using Arg = typename std::remove_reference_t<std::tuple_element_t<i, Arguments>>::value_type;

    template <size_t... i>
    static Results initialize_results_impl(std::index_sequence<i...>) {
        return Results(
            parametric::new_param<
                typename std::tuple_element_t<i, Results>::value_type, 
                typename std::tuple_element_t<i, Results>::serializer_type
            >()...
        );
    };

    template <size_t... i>
    decltype(auto) args_tuple_impl(std::index_sequence<i...>) const {
        return std::tuple<Arg<i> const&...>(arg<i>().value()...);
    }
};

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
decltype(auto) compute(std::shared_ptr<C> const& ptr, Args&&... args)
{ 
    ptr->connect_inputs(std::forward<Args>(args)...);
    auto res = ptr->initialize_results();
    ptr->connect_results(res);
    ptr->post_connect();
    return res;
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
template <typename C, typename S, typename... Args>
decltype(auto) compute(param<Args, S> const&... args)
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
template<typename Fn, typename S, typename... Args>
constexpr decltype(auto)
eval(Fn wrapped_function, const parametric::param<Args, S>& ... parameterArgs)
{

    using rtype = typename std::invoke_result<Fn, Args const&...>::type;
    using Results = typename ComputeNodeTraits<S>::template Results<rtype>;
        using Arguments = typename ComputeNodeTraits<S>::template Arguments<Args...>;

    if constexpr (!std::is_void_v<rtype>) {

        class NonVoidComputer
        : public parametric::ComputeNode<NonVoidComputer, Results, Arguments>
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
        : public parametric::ComputeNode<VoidComputer, Results, Arguments>
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
template<class T, typename S, typename... Args>
parametric::param<T>
new_parametric_struct(const T& the_struct, const parametric::param<Args, S>& ... parametric_members)
{
    using Arguments = typename ComputeNodeTraits<S>::template Arguments<>;
    using Results  = typename ComputeNodeTraits<S>::template Results<>;

    // just need a type derived from ClonableDAGNode<Connector>
    struct Connector : public ComputeNode<Connector,Results, Arguments> {};

    auto c = std::make_shared<Connector>();
    auto t = param<T>("", the_struct);

    // connect c to arguments
    (add_parent(c, parametric_members.node_pointer()), ...);

    // connect t to c
    add_parent(t.node_pointer(), c);

    return t;
}

} // namespace parametric


#endif // PARAMETRIC_CORE_HPP
