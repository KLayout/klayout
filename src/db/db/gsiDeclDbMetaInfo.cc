
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


#include "gsiDecl.h"
#include "gsiDeclDbMetaInfo.h"

namespace gsi
{

static MetaInfo *layout_meta_info_ctor (const std::string &name, const std::string &value, const std::string &description, bool persisted)
{
  return new MetaInfo (name, description, value, persisted);
}

static void layout_meta_set_name (MetaInfo *mi, const std::string &n)
{
  mi->name = n;
}

static const std::string &layout_meta_get_name (const MetaInfo *mi)
{
  return mi->name;
}

static void layout_meta_set_value (MetaInfo *mi, const tl::Variant &n)
{
  mi->value = n;
}

static const tl::Variant &layout_meta_get_value (const MetaInfo *mi)
{
  return mi->value;
}

static void layout_meta_set_description (MetaInfo *mi, const std::string &n)
{
  mi->description = n;
}

static const std::string &layout_meta_get_description (const MetaInfo *mi)
{
  return mi->description;
}

static void layout_meta_set_persisted (MetaInfo *mi, bool f)
{
  mi->persisted = f;
}

static bool layout_meta_get_persisted (const MetaInfo *mi)
{
  return mi->persisted;
}


Class<MetaInfo> decl_LayoutMetaInfo ("db", "LayoutMetaInfo",
  gsi::constructor ("new", &layout_meta_info_ctor, gsi::arg ("name"), gsi::arg ("value"), gsi::arg ("description", std::string ()), gsi::arg ("persisted", false),
    "@brief Creates a layout meta info object\n"
    "@param name The name\n"
    "@param value The value\n"
    "@param description An optional description text\n"
    "@param persisted If true, the meta information will be persisted in some file formats, like GDS2\n"
    "\n"
    "The 'persisted' attribute has been introduced in version 0.28.8.\n"
  ) +
  gsi::method_ext ("name", &layout_meta_get_name,
    "@brief Gets the name of the layout meta info object\n"
  ) +
  gsi::method_ext ("name=", &layout_meta_set_name,
    "@brief Sets the name of the layout meta info object\n"
  ) +
  gsi::method_ext ("value", &layout_meta_get_value,
    "@brief Gets the value of the layout meta info object\n"
  ) +
  gsi::method_ext ("value=", &layout_meta_set_value,
    "@brief Sets the value of the layout meta info object\n"
  ) +
  gsi::method_ext ("description", &layout_meta_get_description,
    "@brief Gets the description of the layout meta info object\n"
  ) +
  gsi::method_ext ("description=", &layout_meta_set_description,
    "@brief Sets the description of the layout meta info object\n"
  ) +
  gsi::method_ext ("persisted", &layout_meta_get_persisted,
    "@brief Gets a value indicating whether the meta information will be persisted\n"
    "This predicate was introduced in version 0.28.8.\n"
  ) +
  gsi::method_ext ("persisted=", &layout_meta_set_persisted,
    "@brief Sets a value indicating whether the meta information will be persisted\n"
    "This predicate was introduced in version 0.28.8.\n"
  ),
  "@brief A piece of layout meta information\n"
  "Layout meta information is basically additional data that can be attached to a layout. "
  "Layout readers may generate meta information and some writers will add layout information to "
  "the layout object. Some writers will also read meta information to determine certain attributes.\n"
  "\n"
  "Multiple layout meta information objects can be attached to one layout using \\Layout#add_meta_info. "
  "Meta information is identified by a unique name and carries a string value plus an optional description string. "
  "The description string is for information only and is not evaluated by code.\n"
  "\n"
  "See also \\Layout#each_meta_info, \\Layout#meta_info_value, \\Layout#meta_info and \\Layout#remove_meta_info as "
  "well as the corresponding \\Cell methods.\n"
  "\n"
  "This class has been introduced in version 0.25 and was extended in version 0.28.8."
);

}
