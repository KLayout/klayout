
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


#include "gsiDecl.h"
#include "dbVia.h"

namespace gsi
{

//  TODO: this is generic. Move it to gsiDecl?

template <class T, class R, R (T::*member)>
struct getter_def
{
  static const R &impl (const T *t)
  {
    return t->*member;
  }
};

template <class T, class R, R (T::*member)>
struct setter_def
{
  static void impl (T *t, const R &r)
  {
    t->*member = r;
  }
};

template <class T, class R, R (T::*member)>
gsi::Methods make_getter_setter (const std::string &name, const std::string &doc)
{
  return gsi::method_ext (name, &getter_def<T, R, member>::impl, doc) +
         gsi::method_ext (name + "=", &setter_def<T, R, member>::impl, gsi::arg ("value"), doc);
}

static db::ViaType *new_via_type (const std::string &name, const std::string &description)
{
  return new db::ViaType (name, description);
}

Class<db::ViaType> decl_dbViaType ("db", "ViaType",
  gsi::constructor ("new", &new_via_type, gsi::arg ("name"), gsi::arg ("description", std::string ()),
    "@brief Creates a new via type object with the given name and description."
  ) +
  make_getter_setter<db::ViaType, std::string, &db::ViaType::name> ("name",
    "@brief The formal name of the via type.\n"
    "The name should be unique and identify the via type in the context of the "
    "via declaration."
  ) +
  make_getter_setter<db::ViaType, std::string, &db::ViaType::description> ("description",
    "@brief The description of the via type.\n"
    "The description is an optional free-style text that describes the via type for a human."
  ) +
  make_getter_setter<db::ViaType, double, &db::ViaType::wbmin> ("wbmin",
    "@brief The minimum bottom-layer width of the via.\n"
    "This values specifies the minimum width of the bottom layer in micrometers. "
    "The default is zero."
  ) +
#if 0 // TODO: not used currently
  make_getter_setter<db::ViaType, double, &db::ViaType::wbmax> ("wbmax",
    "@brief The maximum bottom-layer width of the via.\n"
    "This values specifies the maximum width of the bottom layer in micrometers. "
    "A negative value indicates that no specific upper limit is given. "
    "The default is 'unspecified'."
  ) +
#endif
  make_getter_setter<db::ViaType, double, &db::ViaType::wtmin> ("wtmin",
    "@brief The minimum top-layer width of the via.\n"
    "This values specifies the minimum width of the top layer in micrometers. "
    "The default is zero."
  ) +
#if 0 // TODO: not used currently
  make_getter_setter<db::ViaType, double, &db::ViaType::wtmax> ("wtmax",
    "@brief The maximum top-layer width of the via.\n"
    "This values specifies the maximum width of the top layer in micrometers. "
    "A negative value indicates that no specific upper limit is given. "
    "The default is 'unspecified'."
  ) +
#endif
  make_getter_setter<db::ViaType, double, &db::ViaType::hbmin> ("hbmin",
    "@brief The minimum bottom-layer height of the via.\n"
    "This values specifies the minimum height of the bottom layer in micrometers. "
    "The default is zero."
  ) +
#if 0 // TODO: not used currently
  make_getter_setter<db::ViaType, double, &db::ViaType::hbmax> ("hbmax",
    "@brief The maximum bottom-layer height of the via.\n"
    "This values specifies the maximum height of the bottom layer in micrometers. "
    "A negative value indicates that no specific upper limit is given. "
    "The default is 'unspecified'."
  ) +
#endif
  make_getter_setter<db::ViaType, double, &db::ViaType::htmin> ("htmin",
    "@brief The minimum top-layer height of the via.\n"
    "This values specifies the minimum height of the top layer in micrometers. "
    "The default is zero."
  ) +
#if 0 // TODO: not used currently
  make_getter_setter<db::ViaType, double, &db::ViaType::htmax> ("htmax",
    "@brief The maximum top-layer height of the via.\n"
    "This values specifies the maximum height of the top layer in micrometers. "
    "A negative value indicates that no specific upper limit is given. "
    "The default is 'unspecified'."
  ) +
#endif
  make_getter_setter<db::ViaType, db::LayerProperties, &db::ViaType::bottom> ("bottom",
    "@brief The bottom layer of the via.\n"
  ) +
  make_getter_setter<db::ViaType, db::LayerProperties, &db::ViaType::cut> ("cut",
    "@brief The cut layer of the via.\n"
  ) +
  make_getter_setter<db::ViaType, db::LayerProperties, &db::ViaType::top> ("top",
    "@brief The top layer of the via.\n"
  ) +
#if 0 // TODO: not used currently
  make_getter_setter<db::ViaType, bool, &db::ViaType::bottom_wired> ("bottom_wired",
    "@brief A flag indicating that the bottom layer is a wiring layer.\n"
    "If false, the bottom layer is assume to be a sheet layer, such as diffusion. "
    "In this case, changing the routing layer will not continue drawing a path. "
    "If true (the default), drawing will continue on the bottom layer as a path."
  ) +
#endif
#if 0 // TODO: not used currently
  make_getter_setter<db::ViaType, bool, &db::ViaType::top_wired> ("top_wired",
    "@brief A flag indicating that the top layer is a wiring layer.\n"
    "If false, the bottom layer is assume to be a sheet layer, such as diffusion. "
    "In this case, changing the routing layer will not continue drawing a path. "
    "If true (the default), drawing will continue on the bottom layer as a path."
  ) +
#endif
  make_getter_setter<db::ViaType, double, &db::ViaType::bottom_grid> ("bottom_grid",
    "@brief If non-zero, the bottom layer's dimensions will be rounded to this grid.\n"
  ) +
  make_getter_setter<db::ViaType, double, &db::ViaType::top_grid> ("top_grid",
    "@brief If non-zero, the top layer's dimensions will be rounded to this grid.\n"
  ),
  "@brief Describes a via type\n"
  "These objects are used by PCellDeclaration#via_types to specify the via types a "
  "via PCell is able to provide.\n"
  "\n"
  "The basic parameters of a via type are bottom and top layers (the layers that are "
  "connected by the via) and width and height. Width and height are the dimensions of the "
  "core via area - that is the part where bottom and top layers overlap. The actual "
  "layout may exceed these dimensions if different enclosure rules require so for example.\n"
  "\n"
  "This class has been introduced in version 0.30."
);

}
