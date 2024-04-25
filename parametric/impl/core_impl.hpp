#ifndef CORE_IMPL_HPP
#define CORE_IMPL_HPP

#include <parametric/dag.hpp>
#include <parametric/serialization.hpp>

#include <mutex>

#include <memory>
#include <optional>
#include <cassert>
#include <stdexcept>
#include <type_traits>

#include <algorithm>

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

template <class ResultType, typename S = DefaultSerializer>
class param_holder : public ClonableDAGNode<param_holder<ResultType, S>>
{
public:

    using value_type = std::conditional_t<
        std::is_lvalue_reference_v<ResultType>,
        std::reference_wrapper<std::remove_reference_t<ResultType>>,
        ResultType
    >;
    using serializer_type = S;

    param_holder(param_holder const& other) 
        : ClonableDAGNode<param_holder<ResultType, S>>(other)
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);

        validFlag = other.validFlag;
        m_value = other.m_value;
    }

    param_holder(const ResultType& v, const std::string& id)
        : ClonableDAGNode<param_holder<ResultType, S>>(id)
    {
        if constexpr (std::is_lvalue_reference_v<ResultType>){
            m_value = std::ref(v);    
        }
        else {
            m_value = v;
        }
    }
    param_holder(const std::string& id)
        : ClonableDAGNode<param_holder<ResultType, S>>(id)
    {
    }

    virtual std::string serialize() const override 
    {
        // this triggers evaluation of the node
        return S::serialize(value());
    }

    // in-place constructor
    template <typename... Args>
    param_holder(std::in_place_t, Args const&... args, std::string const& id)
        : ClonableDAGNode<param_holder<ResultType, S>>(id)
        , m_value(std::in_place_t(), args...)
    {}

    const ResultType& value() const
    {
        if (!IsValid()) {
            eval();
            // this assertion should only fail, if the node was not connected to its compute node
            // and therefore compute fails
            assert(IsValid());
        }

        if (m_value) {
            return *m_value;
        }
        throw std::runtime_error("value not initialized");
    }

    ResultType& access_value()
    {
        this->invalidate();
        if (m_value) {
            return *m_value;
        }
        throw std::runtime_error("value not initialized");
    }

    template<class T = value_type>
    typename std::enable_if<EqualityOperatorExists<T>::value>::type
    set_value(const value_type& v)
    {
        if (!IsValid() || !(v == *m_value)) {
            m_value.reset();
            m_value = v;
            this->invalidate();
            validFlag = true;
        }
    }

    template<class T = value_type>
    typename std::enable_if<!EqualityOperatorExists<T>::value>::type
    set_value(const value_type& v)
    {
        if (!IsValid()) {
            m_value.reset();
            m_value = v;
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

    const NodeRef compute_node() const {
        assert(this->parents.size() <= 1);

        if (this->parents.size() > 0) {
            return this->parents[0];
        }
        return nullptr;
    }

protected:
    void invalidateSelf() override
    {
        Clear();
    }

private:
    void eval() const override
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);

        if (auto p = compute_node(); p && !IsValid()) {
            pre_evaluate_arguments();
            p->eval();
        }
        validFlag = true;
    }

    void pre_evaluate_arguments() const
    {
        if (auto p = compute_node(); p && !IsValid()) {
            auto& arguments = p->get_parents();
            std::vector<std::thread> threads;
            for(int i=0; i < arguments.size(); ++i) {
                auto& arg = arguments[i];
                threads.push_back(
                    std::thread(
                        [&]{ arg->eval(); }
                    )
                );
            }
            for (auto& thread : threads) {
                thread.join();
            }
        }
    }

    std::optional<value_type> m_value;
    mutable std::mutex m_mutex;
    mutable bool validFlag{false};
};

} // namespace impl
} // namespace parametric

#endif // CORE_IMPL_HPP
