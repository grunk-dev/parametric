#include <iostream>
#include <parametric/core.hpp>

// Lets define a custom compute node that
// simply computes a division of two values
class DivComputer : public parametric::ComputeNode<DivComputer, parametric::Results<double>, parametric::Arguments<double, double>>
{
public:
    // Overriding eval does the actual computation
    void eval() const override
    {
        if (auto r = res<0>(); r)
            r->set_value(arg<0>().value() / arg<1>().value());
    }

};

int main()
{
    // just create to values
    auto v1 = parametric::new_param(10.0, "v1");
    auto v2 = parametric::new_param(2.0, "v2");

    // now we are creating the compute node and getting the result parameter
    auto result = parametric::compute<DivComputer>(v1, v2);

    std::cout << "Expected result: 5" << std::endl;
    std::cout << "Actual result: " << result << std::endl;

    v2 = 4.0;

    std::cout << "Expected result: 2.5" << std::endl;
    std::cout << "Actual result: " << result << std::endl;
}
