# Parametric #

[![pipeline status](https://gitlab.dlr.de/sc-hpc/parametric/badges/master/pipeline.svg)](https://gitlab.dlr.de/sc-hpc/parametric/commits/master)
[![documentation](https://img.shields.io/badge/docs-online-blue)](https://paradigms.pages.gitlab.dlr.de/parametric/)

Parametric is a conceptual C++ library for parametric lazy evaluation of functions. The base concept are parameters from which computational expensive parametric results are derived. 

The result is only computed, if it is required (lazy evaluation). If any input parameter has changed, the result will be invalidated and the computation is computed again.

[Read the documentation](https://paradigms.pages.gitlab.dlr.de/parametric/) to learn more.

## A simple example ##

```c++
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

    // 1. This is an example to compute j*j using a lambda expression
    auto j_sqr = parametric::eval([](double v) {
        std::cout << "Computing j_sqr\n";
        return v*v;
    }, j);

    // 2. Alternatively, we can also use the operator approach
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

    // 3. And now lets use a user defined function
    result = parametric::eval(divide, result, k);
    std::cout << "  Result: " << result << std::endl;

    return 0;
}
```

In this example, we use different ways to define parametric expressions.

 1. With Lambda expressions
 2. Using operators
 3. User-defined functions
 
Lets have a look at the output

```
Until here, nothing has been computed!
Computing j_sqr
  Result: 25
  Result: 25
Change k=2
  Result: 20
Change j=6
Computing j_sqr
  Result: 40
Divide result / k
  Result: 20
```

What do we see here?
 - If no input (``j`` or ``k``) is changed, no computation is performed again.
 - If we change only ``k``, ``j_sqr`` is not computed again
 - If we change ``j``, ``j_sqr`` has to be recomputed 
