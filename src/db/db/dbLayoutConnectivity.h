
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_dbLayoutConnectivity
#define HDR_dbLayoutConnectivity

#include "dbCommon.h"
#include "dbPropertiesRepository.h"
#include "tlString.h"

namespace db
{

/**
 *  @brief Describes a layout pin
 *
 *  Layout pins are shapes that are declared "pins".
 *  Technically this happens by adding a property
 *  with a key named with pin_property_name_id and
 *  adding a LayoutPin object as a value.
 *
 *  A pin has:
 *  * A name
 *  * A flag indicating whether all pins of the same
 *    name need to be connected ("must_connect")
 */
class DB_PUBLIC LayoutPin
{
public:
  LayoutPin ();
  LayoutPin (const std::string &name, bool must_connect = false);

  void set_name (const std::string &n);
  const std::string &name () const
  {
    return m_name;
  }

  void set_must_connect (bool mc);
  bool must_connect () const
  {
    return m_must_connect;
  }

  std::string to_string () const;
  bool parse (tl::Extractor &ex);

private:
  std::string m_name;
  bool m_must_connect;
};


/**
 *  @brief Encodes pin information
 *
 *  The value is LayoutPin object.
 */
extern DB_PUBLIC db::property_names_id_type pin_property_name_id;

}

/**
 *  @brief Special extractors for the objects
 */

namespace tl
{
  template<> inline bool test_extractor_impl (tl::Extractor &ex, db::LayoutPin &lp)
  {
    return lp.parse (ex);
  }

  template<> inline void extractor_impl (tl::Extractor &ex, db::LayoutPin &lp)
  {
    if (! test_extractor_impl (ex, lp)) {
      ex.error (tl::to_string (tr ("Expected a LayoutPin specification")));
    }
  }

} // namespace tl

#endif
