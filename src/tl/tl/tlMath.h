
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


#ifndef HDR_tlMath
#define HDR_tlMath

#include "tlAssert.h"

#include <cmath>

namespace tl
{

/**
 *  @brief A generic less operator
 */
template <class T>
inline bool less (T a, T b)
{
  return a < b;
}

/**
 *  @brief A generic equal operator
 */
template <class T>
inline bool equal (T a, T b)
{
  return a == b;
}

/**
 *  @brief A generalization of the modulo operator
 */
template <class T>
inline T modulo (T a, T b)
{
  return a % b;
}

/**
 *  @brief A common uncertainty value for double compares
 *  This implementation uses an uncertainty value of 1e-10
 *  which is suitable for values in the order of 1.
 */
const double epsilon = 1e-10;

/**
 *  @brief A specialization for double values
 */
inline bool less (double a, double b)
{
  return a < b - tl::epsilon;
}

/**
 *  @brief A specialization for double values
 */
inline bool equal (double a, double b)
{
  return fabs (a - b) < tl::epsilon;
}

/**
 *  @brief A specialization of the modulo operator for doubles
 *  a % b == a - b * floor (a / b)
 */
inline double modulo (double a, double b)
{
  return a - b * floor (a / b + tl::epsilon);
}

/**
 *  @brief Compute the greatest common divider of two numbers using the euclidian method
 */
template <class T>
inline
T gcd (T a, T b)
{
  while (! equal (b, T (0))) {
    T h = modulo (a, b);
    a = b;
    b = h;
  }
  return a;
}

/**
 *  @brief Compute the lowest common multiple of two numbers using the euclidian method
 */
template <class T>
inline
T lcm (T a, T b)
{
  return a * (b / gcd (a, b));
}

/**
 *  @brief Rounding down to the closest multiple of g
 */
inline double round_down (double x, double g)
{
  return g * floor (x / g + tl::epsilon);
}

/**
 *  @brief Rounding up to the closest multiple of g
 */
inline double round_up (double x, double g)
{
  return g * ceil (x / g - tl::epsilon);
}

/**
 *  @brief Rounding to the closest multiple of g
 *  A value of (n+1/2)*g is rounded down.
 */
inline double round (double x, double g)
{
  return g * floor (0.5 + x / g - tl::epsilon);
}

}

#endif

