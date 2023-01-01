
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


#ifndef HDR_dbTypes
#define HDR_dbTypes

#include <stdint.h>
#define _USE_MATH_DEFINES // for MSVC
#include <math.h>
#include <stdio.h>
#include <algorithm>

namespace db {

/**
 *  @brief The standard integer coordinate type
 */
#if defined(HAVE_64BIT_COORD)
typedef int64_t Coord;
#else
typedef int32_t Coord;
#endif

/** 
 *  @brief The standard floating-point coordinate type
 */
typedef double DCoord;

/**
 *  @brief Coordinate types traits (for integer types)
 *
 *  Defines associated types for a certain coordinate type:
 *  coord_type (the coord_type itself), area_type (the type 
 *  of the area associated), dist_type (the type of the distance
 *  between two coordinates).
 *  Also declares other properties like precision (epsilon),
 *  conversion methods etc.
 */

template <class C, class A, class D, class P, class S> 
struct generic_coord_traits 
{
  /** 
   *  @brief The coordinate type itself 
   */
  typedef C coord_type;

  /**
   *  @brief The associated area type
   */
  typedef A area_type;

  /**
   *  @brief The associated distance type
   */
  typedef D distance_type;

  /**
   *  @brief The associated perimeter type
   */
  typedef P perimeter_type;

  /**
   *  @brief The "short" coordinate type
   *
   *  This is a special type mainly used to represent "short" boxes (i.e. small ones) with 
   *  a small memory footprint. It is used mainly for 32bit coordinates and mask data.
   */
  typedef S short_coord_type;

  /**
   *  @brief The precision (resolution) of the coordinate type
   */
  static coord_type prec () { return 1; }

  /**
   *  @brief The precision (resolution) of the distance type
   */
  static distance_type prec_distance () { return 1; }

  /**
   *  @brief The precision (resolution) of the area type
   */
  static area_type prec_area () { return 1; }

  /**
   *  @brief The rounding method
   *  Specialization for double: Converts the given double to the coordinate type.
   */
  static coord_type rounded (double v) { return coord_type (v > 0 ? v + 0.5 : v - 0.5); }

  /**
   *  @brief The rounding method
   *  Specialization for float: Converts the given double to the coordinate type.
   */
  static coord_type rounded (float v) { return coord_type (v > 0 ? v + 0.5 : v - 0.5); }

  /**
   *  @brief The rounding method
   *  Converts the given value to the coordinate type.
   */
  template <class X>
  static coord_type rounded (X v) { return coord_type (v); }

  /**
   *  @brief The rounding method (up)
   */
  static coord_type rounded_up (double v) { return coord_type (ceil (v)); }

  /**
   *  @brief The rounding method (down)
   */
  static coord_type rounded_down (double v) { return coord_type (floor (v)); }

  /**
   *  @brief The rounding method for distances
   */
  static distance_type rounded_distance (double v) { return distance_type (v > 0 ? v + 0.5 : v - 0.5); }

  /**
   *  @brief The rounding method for perimeters
   */
  static perimeter_type rounded_perimeter (double v) { return perimeter_type (v > 0 ? v + 0.5 : v - 0.5); }

  /**
   *  @brief (Fuzzy) equality of coordinates
   */
  static bool equal (coord_type c1, coord_type c2)
  {
    return c1 == c2;
  }

  /**
   *  @brief (Fuzzy) less comparison of coordinates
   */
  static bool less (coord_type c1, coord_type c2)
  {
    return c1 < c2;
  }

  /**
   *  @brief The test for equality with a double
   */
  static bool equals (coord_type c, double v) 
  { 
    return fabs (double (c) - v) < 0.5;
  }  

  /**
   *  @brief The test for equality of the area with a double
   */
  static bool equals_area (area_type a, double v) 
  { 
    return fabs (double (a) - v) < 0.5;
  }  

  /** 
   *  @brief The square length of a vector
   *  
   *  Computes the square length of a vectors.
   *  The vector is a - b, where a and b are points.
   *
   *  @param ax The first points's x component
   *  @param ay The first points's y component
   *  @param bx The second points's x component
   *  @param by The second points's y component
   *
   *  @return The square length: (ax - bx) * (ay - by)
   */
  static area_type sq_length (coord_type ax, coord_type ay, 
                              coord_type bx, coord_type by) 
  {
    return ((area_type) ax - (area_type) bx) * ((area_type) ax - (area_type) bx) + ((area_type) ay - (area_type) by) * ((area_type) ay - (area_type) by);
  }

  /** 
   *  @brief The sign of the scalar product of two vectors.
   *  
   *  Computes the scalar product of two vectors.
   *  The first vector is a - c, the second b - c,
   *  where a, b and c are points.
   *
   *  @param ax The first points's x component
   *  @param ay The first points's y component
   *  @param bx The second points's x component
   *  @param by The second points's y component
   *  @param cx The third points's x component
   *  @param cy The third points's y component
   *
   *  @return The scalar product: (ax - cx) * (bx - cx) + (ay - cy) * (by - cy)
   */
  static area_type sprod (coord_type ax, coord_type ay, 
                          coord_type bx, coord_type by, 
                          coord_type cx, coord_type cy) 
  {
    return ((area_type) ax - (area_type) cx) * ((area_type) bx - (area_type) cx) + ((area_type) ay - (area_type) cy) * ((area_type) by - (area_type) cy);
  }

  /** 
   *  @brief The sign of the scalar product of two vectors with two 
   *  coordinates.
   *  
   *  Computes the sign of the scalar product of two vectors.
   *  The first vector is a - c, the second b - c,
   *  where a, b and c are points.
   *
   *  @param ax The first points's x component
   *  @param ay The first points's y component
   *  @param bx The second points's x component
   *  @param by The second points's y component
   *  @param cx The third points's x component
   *  @param cy The third points's y component
   *
   *  @return The sign of the scalar product (-1: negative,
   *  0: zero, +1: positive)
   */
  static int sprod_sign (coord_type ax, coord_type ay, 
                         coord_type bx, coord_type by, 
                         coord_type cx, coord_type cy) 
  {
    area_type p1 = ((area_type) ax - (area_type) cx) * ((area_type) bx - (area_type) cx);
    area_type p2 = -(((area_type) ay - (area_type) cy) * ((area_type) by - (area_type) cy));
    if (p1 > p2) {
      return 1;
    } else if (p1 == p2) {
      return 0;
    } else {
      return -1;
    }
  }

  /**
   *  @brief A combination of sign and value of sprod.
   */
  static std::pair<area_type, int> sprod_with_sign (coord_type ax, coord_type ay,
                                                    coord_type bx, coord_type by,
                                                    coord_type cx, coord_type cy)
  {
    area_type p1 = ((area_type) ax - (area_type) cx) * ((area_type) bx - (area_type) cx);
    area_type p2 = -(((area_type) ay - (area_type) cy) * ((area_type) by - (area_type) cy));
    if (p1 > p2) {
      return std::make_pair (p1 - p2, 1);
    } else if (p1 == p2) {
      return std::make_pair (0, 0);
    } else {
      return std::make_pair (p1 - p2, -1);
    }
  }

  /**
   *  @brief the vector product of two vectors.
   *  
   *  Computes the vector product of two vectors.
   *  The first vector is a - c, the second b - c,
   *  where a, b and c are points.
   *
   *  @param ax The first points's x component
   *  @param ay The first points's y component
   *  @param bx The second points's x component
   *  @param by The second points's y component
   *  @param cx The third points's x component
   *  @param cy The third points's y component
   *
   *  @return The vector product: (ax - cx) * (by - cy) - (ax - cx) * (by - cy)
   */
  static area_type vprod (coord_type ax, coord_type ay, 
                          coord_type bx, coord_type by, 
                          coord_type cx, coord_type cy) 
  {
    return ((area_type) ax - (area_type) cx) * ((area_type) by - (area_type) cy) - ((area_type) ay - (area_type) cy) * ((area_type) bx - (area_type) cx);
  }

  /** 
   *  @brief The sign of the vector product of two vectors with two 
   *  coordinates.
   *  
   *  Computes the sign of the vector product of two vectors.
   *  The first vector is a - c, the second b - c,
   *  where a, b and c are points.
   *
   *  @param ax The first points's x component
   *  @param ay The first points's y component
   *  @param bx The second points's x component
   *  @param by The second points's y component
   *  @param cx The third points's x component
   *  @param cy The third points's y component
   *
   *  @return The sign of the vector product (-1: negative,
   *  0: zero, +1: positive)
   */
  static int vprod_sign (coord_type ax, coord_type ay, 
                         coord_type bx, coord_type by, 
                         coord_type cx, coord_type cy) 
  {
    area_type p1 = ((area_type) ax - (area_type) cx) * ((area_type) by - (area_type) cy);
    area_type p2 = ((area_type) ay - (area_type) cy) * ((area_type) bx - (area_type) cx);
    if (p1 > p2) {
      return 1;
    } else if (p1 == p2) {
      return 0;
    } else {
      return -1;
    }
  }

  /**
   *  @brief A combination of sign and value of vprod.
   */
  static std::pair<area_type, int> vprod_with_sign (coord_type ax, coord_type ay,
                                                    coord_type bx, coord_type by,
                                                    coord_type cx, coord_type cy)
  {
    area_type p1 = ((area_type) ax - (area_type) cx) * ((area_type) by - (area_type) cy);
    area_type p2 = ((area_type) ay - (area_type) cy) * ((area_type) bx - (area_type) cx);
    if (p1 > p2) {
      return std::make_pair (p1 - p2, 1);
    } else if (p1 == p2) {
      return std::make_pair (0, 0);
    } else {
      return std::make_pair (p1 - p2, -1);
    }
  }
};

/** 
 *  @brief Coord_traits template declaration
 */
template <class C>
struct coord_traits
{
};

/** 
 *  @brief Coord_traits specialization for 32 bit coordinates 
 */
template <>
struct coord_traits<int32_t>
  : public generic_coord_traits<int32_t, int64_t/*area*/, uint32_t/*dist*/, uint64_t/*perimeter*/, int16_t/*short*/>
{
};

/** 
 *  @brief Coord_traits specialization for 16 bit coordinates 
 */
template <>
struct coord_traits<int16_t>
  : public generic_coord_traits<int16_t, int32_t/*area*/, uint32_t/*dist*/, uint32_t/*perimeter*/, int16_t/*short*/>
{
};

#if defined(HAVE_64BIT_COORD)
/** 
 *  @brief Coord_traits specialization for 64 bit coordinates 
 */
template <>
struct coord_traits<int64_t>
  : public generic_coord_traits<int64_t, __int128/*area*/, uint64_t/*dist*/, uint64_t/*perimeter*/, int32_t/*short*/>
{
};
#endif

/** 
 *  @brief Coord_traits specialization for double coordinates 
 *
 *  The precision is chosen such that the double coordinate
 *  can represent "micrometers" with a physical resolution limit of 0.01 nm.
 *  The area precision will render reliable vector product signs for vectors
 *  of roughly up to 60 mm length.
 */
template <>
struct coord_traits<double>
{
  typedef double coord_type;
  typedef double area_type;
  typedef double distance_type;
  typedef double perimeter_type;
  typedef float short_coord_type;

  static double prec ()                       { return 1e-5; }
  static double prec_distance ()              { return 1e-5; }
  static double prec_area ()                  { return 1e-10; }
  template <class X>
  static double rounded (X v)                 { return double (v); }
  static double rounded_up (double v)         { return v; }
  static double rounded_down (double v)       { return v; }
  static double rounded_distance (double v)   { return v; }
  static double rounded_perimeter (double v)  { return v; }

  static area_type sq_length (coord_type ax, coord_type ay, 
                              coord_type bx, coord_type by) 
  {
    return (ax - bx) * (ax - bx) + (ay - by) * (ay - by);
  }

  static area_type sprod (coord_type ax, coord_type ay, 
                          coord_type bx, coord_type by, 
                          coord_type cx, coord_type cy) 
  {
    return (ax - cx) * (bx - cx) + (ay - cy) * (by - cy);
  }


  static int sprod_sign (double ax, double ay, double bx, double by, double cx, double cy) 
  {
    double dx1 = ax - cx, dy1 = ay - cy;
    double dx2 = bx - cx, dy2 = by - cy;
    double pa = (sqrt (dx1 * dx1 + dy1 * dy1) + sqrt (dx2 * dx2 + dy2 * dy2)) * prec ();
    area_type p1 = dx1 * dx2;
    area_type p2 = -dy1 * dy2;
    if (p1 <= p2 - pa) {
      return -1;
    } else if (p1 < p2 + pa) {
      return 0;
    } else {
      return 1;
    }  
  }

  static std::pair<area_type, int> sprod_with_sign (double ax, double ay, double bx, double by, double cx, double cy)
  {
    double dx1 = ax - cx, dy1 = ay - cy;
    double dx2 = bx - cx, dy2 = by - cy;
    double pa = (sqrt (dx1 * dx1 + dy1 * dy1) + sqrt (dx2 * dx2 + dy2 * dy2)) * prec ();
    area_type p1 = dx1 * dx2;
    area_type p2 = -dy1 * dy2;
    if (p1 <= p2 - pa) {
      return std::make_pair (p1 - p2, -1);
    } else if (p1 < p2 + pa) {
      return std::make_pair (0, 0);
    } else {
      return std::make_pair (p1 - p2, 1);
    }
  }

  static area_type vprod (coord_type ax, coord_type ay, 
                          coord_type bx, coord_type by, 
                          coord_type cx, coord_type cy) 
  {
    return (ax - cx) * (by - cy) - (ay - cy) * (bx - cx);
  }

  static int vprod_sign (double ax, double ay, double bx, double by, double cx, double cy) 
  {
    double dx1 = ax - cx, dy1 = ay - cy;
    double dx2 = bx - cx, dy2 = by - cy;
    double pa = (sqrt (dx1 * dx1 + dy1 * dy1) + sqrt (dx2 * dx2 + dy2 * dy2)) * prec ();
    area_type p1 = dx1 * dy2;
    area_type p2 = dy1 * dx2;
    if (p1 <= p2 - pa) {
      return -1;
    } else if (p1 < p2 + pa) {
      return 0;
    } else {
      return 1;
    }
  }

  static std::pair<area_type, int> vprod_with_sign (double ax, double ay, double bx, double by, double cx, double cy)
  {
    double dx1 = ax - cx, dy1 = ay - cy;
    double dx2 = bx - cx, dy2 = by - cy;
    double pa = (sqrt (dx1 * dx1 + dy1 * dy1) + sqrt (dx2 * dx2 + dy2 * dy2)) * prec ();
    area_type p1 = dx1 * dy2;
    area_type p2 = dy1 * dx2;
    if (p1 <= p2 - pa) {
      return std::make_pair (p1 - p2, -1);
    } else if (p1 < p2 + pa) {
      return std::make_pair (0, 0);
    } else {
      return std::make_pair (p1 - p2, 1);
    }
  }

  static bool equal (double c1, double c2)
  {
    return fabs (c1 - c2) < prec (); 
  }

  static bool less (double c1, double c2)
  {
    return c1 < c2 - prec () * 0.5;
  }

  static bool equals (double c, double v) 
  { 
    return fabs (double (c) - v) < prec (); 
  }  

  static bool equals_area (double a, double v) 
  { 
    return fabs (double (a) - v) < prec_area (); 
  }  

};

/**
 *  @brief A generic conversion operator from double coordinates to any type
 */
template <class D, class C>
struct coord_converter
{
  D operator() (C c) const
  {
    return coord_traits<D>::rounded (c);
  }
};

/**
 *  @brief A very generic cast operator from T to U
 */
template <class U, class T>
struct cast_op 
{
  U operator() (const T &t) const
  {
    return U (t);
  }
};

/**
 *  @brief A generic constant describing the "fuzzyness" of a double comparison of a value around 1
 */
const double epsilon = 1e-10;

/**
 *  @brief A generic constant describing the "fuzzyness" of a float comparison of a value around 1
 */
const double fepsilon = 1e-6;

/**
 *  @brief A functor wrapping the epsilon constant in a templatized form
 */
template <class F>
struct epsilon_f 
{
  operator double () const { return 0.0; } 
};

/**
 *  @brief And the specialization of epsilon_f for double 
 */
template <>
struct epsilon_f<double>
{
  operator double () const { return epsilon; }
};

/**
 *  @brief And the specialization of epsilon_f for float
 */
template <>
struct epsilon_f<float>
{
  operator double () const { return fepsilon; }
};

/**
 *  @brief The type of a cell index
 */
typedef unsigned int cell_index_type;

/**
 *  @brief The type of a properties id
 */
typedef size_t properties_id_type;

/**
 *  @brief The type of a properties name id
 */
typedef size_t property_names_id_type;

/**
 *  @brief The type of the PCell id
 */
typedef unsigned int pcell_id_type;

/**
 *  @brief The type of the library id
 */
typedef size_t lib_id_type;

} // namespace db

#endif

