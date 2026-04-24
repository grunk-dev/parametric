// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

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
    auto v1 = parametric::new_param("v1", 10.0);
    auto v2 = parametric::new_param("v2", 2.0);

    // now we are creating the compute node and getting the result parameter
    auto result = parametric::compute<DivComputer>(v1, v2);

    std::cout << "Expected result: 5" << std::endl;
    std::cout << "Actual result: " << result << std::endl;

    v2 = 4.0;

    std::cout << "Expected result: 2.5" << std::endl;
    std::cout << "Actual result: " << result << std::endl;
}
