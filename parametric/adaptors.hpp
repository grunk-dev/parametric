/**
 * Objects to adapt to legacy code that does not
 * yet use parameters yet
 */


#ifndef PARAMETRIC_ADAPTORS_HPP
#define PARAMETRIC_ADAPTORS_HPP

#include <parametric/impl/optional.hpp>
#include <parametric/dag.hpp>
#include <parametric/core.hpp>

namespace parametric
{

namespace adaptors
{

/**
 * @brief The Invalidator class acts as an adapter
 * between  parametric variables and legacy code
 * that does not use parameters yet. It is mainly used
 * if the legacy code uses a caching mechanism to avoid
 * repeated recomputation. The invalidator can then be used
 * to clear the result cache when needed.
 *
 * The invalidator acts as a dummy node in the parameter tree
 * whose only role is to call an incvalidation function, whenever
 * its parents change.
 *
 * The invalidation function must  be a function without parameters!
 * The function typically will be a lambda function.
 */
class Invalidator
{

public:
    template <typename FunctionObject>
    Invalidator(FunctionObject invalidation_function) noexcept
        : pimpl(new Invalidator::InvImpl<FunctionObject>(invalidation_function, "InvalidatorNode"))
    {}

    Invalidator() {}

    /**
     * Sets the external invalidation function.
     *
     * Whenever one of the parent parameters changes,
     * This function will be called.
     *
     * @param invalidation_function A function object with no arguments
     */
    template <typename FunctionObject>
    void set_invalidation_function(FunctionObject invalidation_function) noexcept
    {
        pimpl.reset(new Invalidator::InvImpl<FunctionObject>(invalidation_function, "InvalidatorNode"));
    }

    /**
     * Registers the invalidator calling the invalidation function
     * whenever "parameter" changes.
     */
    template <typename T>
    void on(const parametric::param<T>& parameter)
    {
        assert(pimpl && "The invalidation function must be set before connecting parameters");
        parametric::add_parent(pimpl, parameter.node_pointer());
    }

private:
    template <typename Fn>
    class InvImpl : public parametric::DAGNode
    {
    public:
        InvImpl(Fn fn, const std::string& id) : parametric::DAGNode(id), fn(fn) {}

        void invalidateSelf() override
        {
            fn();
        }

        Fn fn;
    };

    // using type erasure here to avoid pinning the function to the Invalidator type
    std::shared_ptr<parametric::DAGNode> pimpl;
};

} // namespace adaptors

} // namespace parametric

#endif // PARAMETRIC_ADAPTORS_HPP
