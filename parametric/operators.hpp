// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

/**
 * @file operators.hpp
 * 
 * @brief This file contains the parametric variant of arithmetic functions and their operator overloads.
 */
#ifndef OPERATORS_HPP
#define OPERATORS_HPP

#include <parametric/core.hpp>

namespace parametric
{
    template <class T1, class T2>
    auto p_add(const parametric::param<T1>& a, const parametric::param<T2>& b)
    {

        auto theFun = [](const T1& v1, const T2& v2) {
            return v1 + v2;
        };
        return parametric::eval(theFun, a, b);
    }

    template <class T1, class T2>
    auto p_mult(const parametric::param<T1>& a, const parametric::param<T2>& b)
    {

        auto theFun = [](const T1& v1, const T2& v2) {
            return v1 * v2;
        };
        return parametric::eval(theFun, a, b);
    }
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


#endif // OPERATORS_HPP
