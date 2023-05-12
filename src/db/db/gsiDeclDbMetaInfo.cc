
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

static MetaInfo *layout_meta_info_ctor (const std::string &name, const tl::Variant &value, const std::string &description, bool persisted)
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
  gsi::method_ext ("name=", &layout_meta_set_name, gsi::arg ("name"),
    "@brief Sets the name of the layout meta info object\n"
  ) +
  gsi::method_ext ("value", &layout_meta_get_value,
    "@brief Gets the value of the layout meta info object\n"
  ) +
  gsi::method_ext ("value=", &layout_meta_set_value, gsi::arg ("value"),
    "@brief Sets the value of the layout meta info object\n"
  ) +
  gsi::method_ext ("description", &layout_meta_get_description,
    "@brief Gets the description of the layout meta info object\n"
  ) +
  gsi::method_ext ("description=", &layout_meta_set_description, gsi::arg ("description"),
    "@brief Sets the description of the layout meta info object\n"
  ) +
  gsi::method_ext ("is_persisted?", &layout_meta_get_persisted,
    "@brief Gets a value indicating whether the meta information will be persisted\n"
    "This predicate was introduced in version 0.28.8.\n"
  ) +
  gsi::method_ext ("persisted=", &layout_meta_set_persisted, gsi::arg ("flag"),
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
  "Meta information can be attached to the layout object and to cells. It is similar to "
  "user properties. The differences are:\n"
  "\n"
  "@ul\n"
  "@li Meta information is stored differently in GDS and OASIS files using the context information added "
  "    by KLayout to annotated PCell or library cells too. Hence meta information does not pollute "
  "    the standard user properties space. @/li\n"
  "@li The value of meta information can be complex serializable types such as lists, hashes and elementary "
  "    objects such as \\Box or \\DBox. Scalar types include floats and booleans. @/li\n"
  "@li Meta information keys are strings and are supported also for GDS which only accepts integer number "
  "    keys for user properties. @/li\n"
  "@/ul\n"
  "\n"
  "Elementary (serializable) objects are: \\Box, \\DBox, \\Edge, \\DEdge, \\EdgePair, \\DEdgePair, "
  "\\EdgePairs, \\Edges, \\LayerProperties, \\Matrix2d, \\Matrix3d, \\Path, \\DPath, \\Point, \\DPoint, "
  "\\Polygon, \\DPolygon, \\SimplePolygon, \\DSimplePolygon, \\Region, \\Text, \\DText, \\Texts, "
  "\\Trans, \\DTrans, \\CplxTrans, \\ICplxTrans, \\DCplxTrans, \\VCplxTrans, \\Vector, \\DVector "
  "(list may not be complete).\n"
  "\n"
  "KLayout itself also generates meta information with specific keys. "
  "For disambiguation, namespaces can be established by prefixing "
  "the key strings with some unique identifier in XML fashion, like a domain name - "
  "e.g. 'example.com:key'.\n"
  "\n"
  "@b Note: @/b only meta information marked with \\is_persisted? == true is stored in GDS or OASIS files. "
  "This is not the default setting, so you need to explicitly set that flag.\n"
  "\n"
  "See also \\Layout#each_meta_info, \\Layout#meta_info_value, \\Layout#meta_info and \\Layout#remove_meta_info as "
  "well as the corresponding \\Cell methods.\n"
  "\n"
  "An example of how to attach persisted meta information to a cell is here:\n"
  "\n"
  "@code\n"
  "ly = RBA::Layout::new\n"
  "c1 = ly.create_cell(\"C1\")\n"
  "\n"
  "mi = RBA::LayoutMetaInfo::new(\"the-answer\", 42.0)\n"
  "mi.persisted = true\n"
  "c1.add_meta_info(mi)\n"
  "\n"
  "# will now hold this piece of meta information attached to cell 'C1':\n"
  "ly.write(\"to.gds\")\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.25 and was extended in version 0.28.8."
);

}
