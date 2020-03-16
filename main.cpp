#include <iostream>
#include <parametric/core.hpp>
#include <tuple>
#include <cmath>


template <class T1, class T2>
auto p_add(const parametric::param<T1>& a, const parametric::param<T2>& b)
{

    auto theFun = [](const T1& v1, const T2& v2) {
        std::cout << "Add" << std::endl;
        return v1 + v2;
    };
    return parametric::eval(theFun, a, b);
}

template <class T1, class T2>
auto p_mult(const parametric::param<T1>& a, const parametric::param<T2>& b)
{

    auto theFun = [](T1 v1, T2 v2) {
        std::cout << "Mult" << std::endl;
        return v1 * v2;
    };
    return parametric::eval(theFun, a, b);
}

template <class T1, class T2>
auto operator+(const parametric::param<T1>& a, const parametric::param<T2>& b)
{

    return p_add(a, b);
}

template <class T1, class T2>
auto operator*(const parametric::param<T1>& a, const parametric::param<T2>& b)
{

    return p_mult(a, b);
}


int main()
{
    auto k = parametric::new_param(2);
    auto j = parametric::new_param(-1.);


    auto result = k * k + j;

    result = parametric::eval([](double v) {return v*v;}, result);

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
