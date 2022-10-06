#ifndef CORE_IMPL_HPP
#define CORE_IMPL_HPP

#include <parametric/dag.hpp>
#include <parametric/impl/optional.hpp>
#include <parametric/serialization.hpp>

#include <cassert>

namespace parametric {

namespace impl
{



struct No {};
template<typename T, typename Arg> No operator== (const T&, const Arg&);

template<typename T, typename Arg = T>
struct EqualityOperatorExists
{
    enum { value = !std::is_same<decltype(std::declval<T>() == std::declval<Arg>()), No>::value };
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

    virtual std::string serialize() const override 
    {
        if (parents.size() == 0) {

            // a root
            if (value.is_initialized()) {
                return parametric::serialize(value.value());
            } else {
                return TypeName<ResultType>::Get();
            }
        }

        return "";
    }

    // in-place constructor
    template <typename... Args>
    param_holder(in_place_t, Args const&... args, std::string const& id)
        : DAGNode(id)
        , value(in_place_t(), args...)
    {}

    const ResultType& Value() const
    {
        if (!IsValid()) {
            eval();
            // this assertion should only fail, if the node was not connected to its compute node
            // and therefore compute fails
            assert(IsValid());
        }

        return value.value();
    }

    ResultType& AccessValue()
    {
        invalidate();
        return value.value();
    }

    template<class T = ResultType>
    typename std::enable_if<EqualityOperatorExists<T>::value>::type
    SetValue(const ResultType& v)
    {
        if (!IsValid() || !(v == value.value())) {
            value = v;
            invalidate();
            validFlag = true;
        }
    }

    template<class T = ResultType>
    typename std::enable_if<!EqualityOperatorExists<T>::value>::type
    SetValue(const ResultType& v)
    {
        if (!IsValid()) {
            value = v;
            invalidate();
            validFlag = true;
        }
    }

    bool IsValid() const
    {
        return validFlag;
    }

    void Clear()
    {
        validFlag = false;
    }

protected:
    void invalidateSelf() override
    {
        Clear();
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
        validFlag = true;
    }

    optional<ResultType> value;
    mutable bool validFlag{false};
};

} // namespace impl
} // namespace parametric

#endif // CORE_IMPL_HPP
