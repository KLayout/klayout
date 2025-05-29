
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_pexRExtractorTech
#define HDR_pexRExtractorTech

#include "pexCommon.h"

#include <list>
#include <string>

namespace pex
{

/**
 *  @brief Specifies the extraction parameters for vias
 *
 *  Note that the layers are generic IDs. These are usigned ints specifying
 *  a layer.
 */
class PEX_PUBLIC RExtractorTechVia
{
public:
  RExtractorTechVia ()
    : cut_layer (0), top_conductor (0), bottom_conductor (0), resistance (0.0), merge_distance (0.0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns a string describing this object
   */
  std::string to_string () const;

  /**
   *  @brief Specifies the cut layer
   *  This is the layer the via sits on
   */
  unsigned int cut_layer;

  /**
   *  @brief Specifies the top conductor
   *  The value is the ID of the top conductor layer
   */
  unsigned int top_conductor;

  /**
   *  @brief Specifies the bottom conductor
   *  The value is the ID of the bottom conductor layer
   */
  unsigned int bottom_conductor;

  /**
   *  @brief Specifies the resistance in Ohm * sqaure micrometer
   */
  double resistance;

  /**
   *  @brief Specifies the merge distance in micrometers
   *  The merge distance indicates a range under which vias are merged
   *  into bigger effective areas to reduce the complexity of via arrays.
   */
  double merge_distance;
};

/**
 *  @brief Specifies the extraction parameters for a conductor layer
 *
 *  Note that the layers are generic IDs. These are usigned ints specifying
 *  a layer.
 */
class PEX_PUBLIC RExtractorTechConductor
{
public:
  /**
   *  @brief A algorithm to use
   */
  enum Algorithm
  {
    /**
     *  @brief The square counting algorithm
     *  This algorithm is suitable for "long and thin" wires.
     */
    SquareCounting = 0,

    /**
     *  @brief The tesselation algorithm
     *  This algorithm is suitable to "large" sheets, specifically substrate.
     */
    Tesselation = 1
  };

  /**
   *  @brief The constructor
   */
  RExtractorTechConductor ()
    : layer (0), resistance (0.0), algorithm (SquareCounting), triangulation_min_b (-1.0), triangulation_max_area (-1.0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns a string describing this object
   */
  std::string to_string () const;

  /**
   *  @brief Specifies the layer
   *  The value is the generic ID of the layer.
   */
  unsigned int layer;

  /**
   *  @brief Specifies the sheet resistance
   *  The sheet resistance is given in units of Ohm / square
   */
  double resistance;

  /**
   *  @brief The algorihm to use
   */
  Algorithm algorithm;

  /**
   *  @brief The "min_b" parameter for the triangulation
   *  The "b" parameter is a ratio of shortest triangle edge to circle radius.
   *  If a negative value is given, the default value is taken.
   */
  double triangulation_min_b;

  /**
   *  @brief The "max_area" parameter for the triangulation
   *  The "max_area" specifies the maximum area of the triangles produced in square micrometers.
   *  If a negative value is given, the default value is taken.
   */
  double triangulation_max_area;
};

/**
 *  @brief Specifies the extraction parameters
 */
class PEX_PUBLIC RExtractorTech
{
public:
  /**
   *  @brief Constructor
   */
  RExtractorTech ();

  /**
   *  @brief Returns a string describing this object
   */
  std::string to_string () const;

  /**
   *  @brief A list of via definitions
   */
  std::list<RExtractorTechVia> vias;

  /**
   *  @brief A list of conductor definitions
   */
  std::list<RExtractorTechConductor> conductors;

  /**
   *  @brief A flag indicating to skip the simplify step after extraction
   */
  bool skip_simplify;
};

}

#endif

