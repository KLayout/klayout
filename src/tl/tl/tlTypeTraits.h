
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#ifndef HDR_tlTypeTraits
#define HDR_tlTypeTraits

#include "tlCommon.h"

namespace tl
{

/**
 *  @brief A general "true" tag
 */
struct true_tag { };

/**
 *  @brief A general "false" tag
 */
struct false_tag { };

/**
 *  @brief Convert a boolean value to a type
 */
template <bool>
struct boolean_value;

template <>
struct boolean_value<true>
{
  typedef true_tag value;
};

template <>
struct boolean_value<false>
{
  typedef false_tag value;
};

/**
 *  @brief Convert a true_tag to a boolean value
 */
inline bool value_of (true_tag) { return true; }

/**
 *  @brief Convert a false_tag to a boolean value
 */
inline bool value_of (false_tag) { return false; }

//  SFINAE boolean types
typedef char __yes_type [1];
typedef char __no_type [2];

/**
 *  @brief Detects whether a class has a "to_variant" method with a matching signature
 */
template <typename T> static __yes_type &__test_to_variant_func (decltype (&T::to_variant));
template <typename> static __no_type &__test_to_variant_func (...);

template <typename T>
struct has_to_variant
{
  static constexpr bool value = sizeof (__test_to_variant_func<T> (nullptr)) == sizeof (__yes_type);
};

/**
 *  @brief Detects whether a class has a "to_string" method with a matching signature
 */
template <typename T> static __yes_type &__test_to_string_func (decltype (&T::to_string));
template <typename> static __no_type &__test_to_string_func (...);

template <typename T>
struct has_to_string
{
  static constexpr bool value = sizeof (__test_to_string_func<T> (nullptr)) == sizeof (__yes_type);
};

/**
 *  @brief Detects whether a class has a "to_int" method with a matching signature
 */
template <typename T> static __yes_type &__test_to_int_func (decltype (&T::to_int));
template <typename> static __no_type &__test_to_int_func (...);

template <typename T>
struct has_to_int
{
  static constexpr bool value = sizeof (__test_to_int_func<T> (nullptr)) == sizeof (__yes_type);
};

/**
 *  @brief Detects whether a class has a "to_double" method with a matching signature
 */
template <typename T> static __yes_type &__test_to_double_func (decltype (&T::to_double));
template <typename> static __no_type &__test_to_double_func (...);

template <typename T>
struct has_to_double
{
  static constexpr bool value = sizeof (__test_to_double_func<T> (nullptr)) == sizeof (__yes_type);
};

/**
 *  @brief Detects whether a class has an equal operator
 */
template <typename T> static __yes_type &__test_equal_func (decltype (&T::operator==));
template <typename> static __no_type &__test_equal_func (...);

template <typename T>
struct has_equal_operator
{
  static constexpr bool value = sizeof (__test_equal_func<T> (nullptr)) == sizeof (__yes_type);
};

/**
 *  @brief Detects whether a class has a less operator
 */
template <typename T> static __yes_type &__test_less_func (decltype (&T::operator<));
template <typename> static __no_type &__test_less_func (...);

template <typename T>
struct has_less_operator
{
  static constexpr bool value = sizeof (__test_less_func<T> (nullptr)) == sizeof (__yes_type);
};

/**
 *  @brief Detects whether a class has a "swap" method with a matching signature
 */
template <typename T> static __yes_type &__test_swap_func (decltype (&T::swap));
template <typename> static __no_type &__test_swap_func (...);

template <typename T>
struct has_swap
{
  static constexpr bool value = sizeof (__test_swap_func<T> (nullptr)) == sizeof (__yes_type);
};

}

#endif


