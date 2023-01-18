
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


#ifndef HDR_gsiDeclDbContainerHelpers
#define HDR_gsiDeclDbContainerHelpers

#include "dbPropertiesRepository.h"
#include "tlVariant.h"
#include "gsiDecl.h"

#include <vector>
#include <map>

namespace gsi
{

template <class Container>
static void enable_properties (Container *c)
{
  c->apply_property_translator (db::PropertiesTranslator::make_pass_all ());
}

template <class Container>
static void remove_properties (Container *c)
{
  c->apply_property_translator (db::PropertiesTranslator::make_remove_all ());
}

template <class Container>
static void filter_properties (Container *c, const std::vector<tl::Variant> &keys)
{
  if (c->has_properties_repository ()) {
    std::set<tl::Variant> kf;
    kf.insert (keys.begin (), keys.end ());
    c->apply_property_translator (db::PropertiesTranslator::make_filter (c->properties_repository (), kf));
  }
}

template <class Container>
static void map_properties (Container *c, const std::map<tl::Variant, tl::Variant> &map)
{
  if (c->has_properties_repository ()) {
    c->apply_property_translator (db::PropertiesTranslator::make_key_mapper (c->properties_repository (), map));
  }
}

template <class Container>
static gsi::Methods
make_property_methods ()
{
  return
  gsi::method_ext ("enable_properties", &enable_properties<Container>,
    "@brief Enables properties for the given container.\n"
    "This method has an effect mainly on original layers and will import properties from such layers. "
    "By default, properties are not enabled on original layers. Alternatively you can apply \\filter_properties "
    "or \\map_properties to enable properties with a specific name key.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("remove_properties", &remove_properties<Container>,
    "@brief Removes properties for the given container.\n"
    "This will remove all properties on the given container.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("filter_properties", &filter_properties<Container>, gsi::arg ("keys"),
    "@brief Filters properties by certain keys.\n"
    "Calling this method on a container will reduce the properties to values with name keys from the 'keys' list.\n"
    "As a side effect, this method enables properties on original layers.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("map_properties", &map_properties<Container>, gsi::arg ("key_map"),
    "@brief Maps properties by name key.\n"
    "Calling this method on a container will reduce the properties to values with name keys from the 'keys' hash and "
    "renames the properties. Properties not listed in the key map will be removed.\n"
    "As a side effect, this method enables properties on original layers.\n"
    "\n"
    "This method has been introduced in version 0.28.4."
  );
}

}

#endif
