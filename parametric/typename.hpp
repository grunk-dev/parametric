// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
// SPDX-FileCopyrightText: 2026 Martin Siggel <martin.siggel@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef TYPENAME_HPP
#define TYPENAME_HPP

#include <typeinfo>

/**
 * @brief TypeName provides an interface to
 * access the string representation of T
 *
 * A customized string representation can be defined
 * by providing a template specialization of TypeName
 */
template <typename T>
struct TypeName
{
    /// @brief Returns the type string
    static const char* Get()
    {
        return typeid(T).name();
    }
};

/// @private
template <>
struct TypeName<int>
{
    static const char* Get()
    {
        return "int";
    }
};

/// @private
template <>
struct TypeName<double>
{
    static const char* Get()
    {
        return "double";
    }
};

#endif // TYPENAME_HPP
