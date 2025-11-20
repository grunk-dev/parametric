#include <iostream>
#include <parametric/core.hpp>

// This is a simple structure composed of multiple parameters
struct AStruct
{
    parametric::param<int> a{"a", 10};
    parametric::param<int> b{"b", 100};
};

// As a best practice, create a helper function to build parametric
// versions of the structure.
// new_parametric_struct ensures, that the changes of a and b
// are propagated also into the structure itself
template<>
parametric::param<AStruct> parametric::new_param(const AStruct& s)
{
    return parametric::new_parametric_struct(s, s.a, s.b);
}


int main()
{
    auto structSum = [](const AStruct& s) -> int {
        return s.a.value() + s.b.value();
    };

    AStruct s;
    auto parametricS = parametric::new_param(s);
    auto sum = parametric::eval(structSum, parametricS);

    std::cout << "Initial sum: " << sum.value() << std::endl; // 110

    // change one parameter
    s.b = 1000;
    std::cout << "Sum after changing b: " <<sum.value() << std::endl; // 1010
}
