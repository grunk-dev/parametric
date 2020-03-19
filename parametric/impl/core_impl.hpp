#ifndef CORE_IMPL_HPP
#define CORE_IMPL_HPP

#include <parametric/dag.hpp>
#include <parametric/optional.hpp>

namespace parametric {
namespace impl
{

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

    const ResultType& Value() const
    {
        if (!value.is_initialized()) {
            eval();
            // this assertion should only fail, if the node was not connected to its compute node
            // and therefore compute fails
            assert(value.is_initialized());
        }

        return value.value();
    }
    void SetValue(const ResultType& v)
    {
        if (!value.is_initialized() || v != value.value()) {
            value = v;
            invalidate();
        }
    }

    bool IsValid() const
    {
        return value.is_initialized();
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

} // namespace impl
} // namespace parametric

#endif // CORE_IMPL_HPP
