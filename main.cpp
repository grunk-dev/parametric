#include <iostream>
#include "parametric_core.hpp"
#include <tuple>


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
parametric::param<typename std::result_of<Fn(Args...)>::type>
eval_parametric(Fn wrapped_function, const parametric::param<Args>&... parameterArgs) {

    using rtype = typename std::result_of<Fn(Args...)>::type;

    class ComputeWrapperNode : public parametric::ComputeNode
    {
    public:
        ComputeWrapperNode(Fn ff, parametric::param<Args>... args)
            : _wrapped_function(ff), _parameters(args...)
            ,  _resultNode(parametric::new_param<rtype>())
        {
            // define input and output arguments
            define_output(_resultNode.node_pointer());
            static_foreach(_parameters, [this](const auto& parm) {
                define_input(parm.node_pointer());
            });

        }

        void compute()
        {
            _resultNode.SetValue(
                apply(
                  std::forward<Fn>(_wrapped_function),
                  _parameters,
                  [](const auto& parm) {return parm.Value();}
            ));
        }

        parametric::param<rtype> result() const
        {
            return _resultNode;
        }

    private:
        Fn _wrapped_function;
        std::tuple<parametric::param<Args>...> _parameters;
        parametric::param<rtype> _resultNode;
    };


    std::shared_ptr<ComputeWrapperNode> computeNode(new ComputeWrapperNode(wrapped_function, parameterArgs...));
    ComputeWrapperNode::connect_ins_outs(computeNode);


   return computeNode->result();
}

template <class T1, class T2>
auto p_add(const parametric::param<T1>& a, const parametric::param<T2>& b)
{

    auto theFun = [](T1 v1, T2 v2) {
        std::cout << "Add" << std::endl;
        return v1 + v2;
    };
    return eval_parametric(theFun, a, b);
}

double mult(double a, int b)
{
    std::cout << "Mult" << std::endl;
    return a * b;
}



int main()
{
    auto k = parametric::new_param(1);
    auto j = parametric::new_param(2.5);


    auto result = eval_parametric(mult, k, k);
    result = p_add(result, j);

    std::cout << "Until here, nothing has been computed!" << std::endl;
    std::cout << "Result: " << result << std::endl;
    std::cout << "Result: " << result << std::endl;

    k = 10;
    std::cout << "Result: " << result << std::endl;
    std::cout << "Result: " << result << std::endl;
    
    j = 11;
    std::cout << "Result: " << result << std::endl;
    std::cout << "Result: " << result << std::endl;

    return 0;
}
