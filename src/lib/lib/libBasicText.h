
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


#ifndef HDR_libBasicText
#define HDR_libBasicText

#include "libCommon.h"

#include "dbPCellDeclaration.h"

namespace lib
{

/**
 *  @brief Implements the "TEXT" PCell of the basic library
 */
class BasicText 
  : public db::PCellDeclaration
{
public:
  /**
   *  @brief The constructor
   */
  BasicText ();

  /**
   *  @brief This PCell can be created from a shape
   */
  virtual bool can_create_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const;

  /**
   *  @brief Get the parameters from a shape
   */
  virtual db::pcell_parameters_type parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const;

  /**
   *  @brief Get the instance transformation from a shape
   */
  virtual db::Trans transformation_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const;

  /**
   *  @brief Get the layer declarations
   */
  virtual std::vector<db::PCellLayerDeclaration> get_layer_declarations (const db::pcell_parameters_type &parameters) const;

  /**
   *  @brief Coerces the parameters (in particular updates the computed ones)
   */
  virtual void coerce_parameters (const db::Layout &layout, db::pcell_parameters_type &parameters) const;

  /**
   *  @brief Produces the layout
   */
  virtual void produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const;

  /**
   *  @brief Get the display name for a PCell with the given parameters
   */
  virtual std::string get_display_name (const db::pcell_parameters_type &) const;

  /**
   *  @brief Get the parameter declarations
   */
  virtual std::vector<db::PCellParameterDeclaration> get_parameter_declarations () const;

protected:
  /**
   *  @brief Returns a value indicating that this PCell wants to update it's parameter declarations dynamically
   *
   *  This is be required because the fonts can be updated dynamically when new packages are installed.
   */
  virtual bool wants_parameter_declaration_caching () const
  {
    return false;
  }

public:
  int get_font_index (const db::pcell_parameters_type &parameters) const;
};

}

#endif

