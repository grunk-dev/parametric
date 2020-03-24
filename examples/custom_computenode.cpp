#include <iostream>
#include <parametric/core.hpp>

// Lets define a custom compute node that
// simply computes a division of two values
class DivComputer : public parametric::ComputeNode
{
public:
    DivComputer(const parametric::param<double>& op1, const parametric::param<double>& op2)
        : v1(op1), v2(op2)
    {
        // Our result depends on v1 and v2
        depends_on(v1);
        depends_on(v2);
        // The class compute "theresult", which we must register here
        computes(theresult, parametric::param<double>("result"));
    }

    parametric::param<double> result() const {return theresult;}

    // Overriding eval does the actual computation
    void eval() const override
    {
        if (!theresult.expired())
            theresult.set_value(v1.value() / v2.value());
    }

private:
    // We have to define InterfaceParam for all inputs and output parameters
    const parametric::InterfaceParam<double> v1, v2;
    mutable parametric::InterfaceParam<double> theresult;
};

int main()
{
    // just create to values
    auto v1 = parametric::new_param(10.0, "v1");
    auto v2 = parametric::new_param(2.0, "v2");

    // now we are creating the compute node
    auto divNode = parametric::new_node<DivComputer>(v1, v2);

    // now lets get the result. we could also use both lines at once:
    // auto result = parametric::new_node<DivComputer>(v1, v2)->result();
    auto result = divNode->result();

    std::cout << "Expected result: 5" << std::endl;
    std::cout << "Actual result: " << result << std::endl;

    v2 = 4.0;

    std::cout << "Expected result: 2.5" << std::endl;
    std::cout << "Actual result: " << result << std::endl;
}
