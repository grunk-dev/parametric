#ifndef PARAMETRIC_CORE_HPP
#define PARAMETRIC_CORE_HPP

#include <parametric/typename.hpp>
#include <parametric/tupletools.hpp>
#include <parametric/optional.hpp>

#include <memory>
#include <vector>
#include <algorithm>
#include <exception>
#include <cassert>

#include <type_traits> // for std::false_type

namespace parametric
{

template <typename T> struct AlwaysFalse : std::false_type {
};

using namespace std;

class DAGNode;

typedef std::shared_ptr<DAGNode> NodeRef;

class may_not_attach : public std::exception
{};

class DAGNode
{
public:
    DAGNode(const std::string& id)
        : _id(id)
    {}

    friend void addParent(const NodeRef& child, const NodeRef& parent)
    {

        if (!child || !parent) {
            throw std::invalid_argument("Cannot attach node: null pointer passed.");
        }


        if (child == parent || child->precedes(*parent)) {
            throw std::runtime_error("Cannot attach node: cycles are not allowed.");
        }

        // only attach if not already attached
        if (std::find(std::begin(child->parents), std::end(child->parents), parent) == child->parents.end()) {
            child->parents.push_back(parent);
            parent->childs.push_back(child);
        }
    }


    virtual std::string id() const
    {
        return _id;
    }

    void SetId(const std::string& id)
    {
        _id = id;
    }


    /**
     * @brief Checks, whether "node" precides the current node in the DAG
     * @param node The node to be checked
     * @return True, if node precedes this node.
     */
    bool precedes(const DAGNode& node) const
    {
        class HasChildVisitor
        {
        public:
            HasChildVisitor(const DAGNode& c)
                : _c(c)
                , hasChild(false)
            {}

            void visit(const DAGNode& n, size_t depth)
            {
                if (&n == &_c && depth > 0) {
                    hasChild = true;
                }
            }

        private:
            const DAGNode& _c;

        public:
            bool hasChild;
        };

        HasChildVisitor v(node);
        accept(v);

        return v.hasChild;
    }

    /**
     * @brief This provides an interface for the visitor pattern to
     * walk through the whole DAG down from this node
     */
    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0,bool down=true) const
    {
        v.visit(*this, depth);
        if (down) {
            for (std::weak_ptr<DAGNode>& child : childs) {
                if (NodeRef c = child.lock()) {
                    c->accept(v, depth + 1, down);
                }
            }
        }
        else {
            for (NodeRef parent : parents) {
                parent->accept(v, depth + 1, down);
            }
        }
    }

    template <typename Visitor>
    void accept(Visitor& v, size_t depth = 0, bool down=true)
    {
        v.visit(*this, depth);
        if (down) {
            for (std::weak_ptr<DAGNode>& child : childs) {
                if (NodeRef c = child.lock()) {
                    c->accept(v, depth + 1, down);
                }
            }
        }
        else {
            for (NodeRef parent : parents) {
                parent->accept(v, depth + 1, down);
            }
        }
    }

    void removeParent(const DAGNode& parent)
    {
        // remove myself from parent
        auto& p_childs  = parent.childs;
        auto it_to_this = std::find_if(p_childs.begin(), p_childs.end(),
        [this](std::weak_ptr<DAGNode>& child) {
            return child.lock().get() == this;
        });

        if (it_to_this != p_childs.end()) {
            p_childs.erase(it_to_this);
        }

        // remove parent from parent list
        auto parentIt = std::find_if(std::begin(parents), std::end(parents), [&parent](const NodeRef& p) {
            return p.get() == &parent;
        });

        if (parentIt != parents.end()) {
            parents.erase(parentIt);
        }
    }

    // Invalidates this node and all other childs
    void invalidate()
    {
        class InvalVisitor
        {
        public:
            void visit(DAGNode& n, size_t depth)
            {
                if (depth == 0) {
                    return;
                }

                n.invalidateSelf();
            }
        };

        InvalVisitor v;
        accept(v);
    }

    virtual void eval() const {}

    virtual ~DAGNode()
    {
        for (auto parent : parents) {
            if (parent) {
                removeParent(*parent);
            }
        }
    }


protected:
    std::string _id;

    mutable std::vector<std::weak_ptr<DAGNode>> childs;
    std::vector<std::shared_ptr<DAGNode>> parents;

protected:
    virtual void invalidateSelf() {}
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
        : m_holder(std::make_shared<param_holder<T>>(v, id))
    {}

    param(const std::string& id)
        : m_holder(std::make_shared<param_holder<T>>(id))
    {}

    param(const std::shared_ptr<param_holder<T>>& holder)
        : m_holder(holder)
    {}

    const std::shared_ptr<param_holder<T>> node_pointer() const
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
    std::shared_ptr<param_holder<T>> m_holder;
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
void addParent(std::shared_ptr<param_holder<C1>>&, std::shared_ptr<param_holder<C2>>)
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
    std::weak_ptr<parametric::param_holder<T>> p_holder;
};


class ComputeNode : public DAGNode
{
public:
    virtual ~ComputeNode() {}

    template <typename T>
    void DefineInput(const InterfaceParam<T>& p) {
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
        if (!c) {
            return;
        }
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

    ~compute_node_ptr(){
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
compute_node_ptr<T> new_node(Args&& ... args){
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
