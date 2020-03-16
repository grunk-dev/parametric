#include <iostream>
#include <parametric/core.hpp>
#include <parametric/operators.hpp>

double divide(double v1, double v2)
{
    std::cout << "Divide result / k\n";
    return v1 / v2;
}

int main()
{
    auto k = parametric::new_param(3.);
    auto j = parametric::new_param(4.);

    // This is an example to compute j*j using a lambda expression
    auto j_sqr = parametric::eval([](double v) {
        std::cout << "Computing j_sqr\n";
        return v*v;
    }, j);

    // Alternatively, we can also use the operator approach
    auto result = k * k + j_sqr;

    std::cout << "Until here, nothing has been computed!" << std::endl;
    std::cout << "  Result: " << result << std::endl;
    std::cout << "  Result: " << result << std::endl;

    std::cout << "Change k=2" << std::endl;
    k = 2;
    std::cout << "  Result: " << result << std::endl;

    std::cout << "Change j=6" << std::endl;
    j = 6;
    std::cout << "  Result: " << result << std::endl;

    // And now lets use a user defined function
    result = parametric::eval(divide, result, k);
    std::cout << "  Result: " << result << std::endl;

    return 0;
}
