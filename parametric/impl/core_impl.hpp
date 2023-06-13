#ifndef CORE_IMPL_HPP
#define CORE_IMPL_HPP

#include <parametric/dag.hpp>
#include <parametric/serialization.hpp>

#include <memory>
#include <optional>
#include <cassert>
#include <stdexcept>

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
class param_holder : public ClonableDAGNode<param_holder<ResultType>>
{
public:
    param_holder(const ResultType& v, const std::string& id)
        : ClonableDAGNode<param_holder<ResultType>>(id)
        , value(v)
    {
    }
    param_holder(const std::string& id)
        : ClonableDAGNode<param_holder<ResultType>>(id)
    {
    }

    virtual std::string serialize() const override 
    {
        // this triggers evaluation of the node
        return parametric::serialize(Value());
    }

    // in-place constructor
    template <typename... Args>
    param_holder(std::in_place_t, Args const&... args, std::string const& id)
        : ClonableDAGNode<param_holder<ResultType>>(id)
        , value(std::in_place_t(), args...)
    {}

    const ResultType& Value() const
    {
        if (!IsValid()) {
            eval();
            // this assertion should only fail, if the node was not connected to its compute node
            // and therefore compute fails
            assert(IsValid());
        }

        if (value) {
            return *value;
        }
        throw std::runtime_error("value not initialized");
    }

    ResultType& AccessValue()
    {
        this->invalidate();
        if (value) {
            return *value;
        }
        throw std::runtime_error("value not initialized");
    }

    template<class T = ResultType>
    typename std::enable_if<EqualityOperatorExists<T>::value>::type
    SetValue(const ResultType& v)
    {
        if (!IsValid() || !(v == *value)) {
            value = v;
            this->invalidate();
            validFlag = true;
        }
    }

    template<class T = ResultType>
    typename std::enable_if<!EqualityOperatorExists<T>::value>::type
    SetValue(const ResultType& v)
    {
        if (!IsValid()) {
            value = v;
            this->invalidate();
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
        assert(this->parents.size() <= 1);

        if (this->parents.size() > 0) {
            NodeRef p = this->parents[0];
            p->eval();
        }
        validFlag = true;
    }

    std::optional<ResultType> value;
    mutable bool validFlag{false};
};

} // namespace impl
} // namespace parametric

#endif // CORE_IMPL_HPP
