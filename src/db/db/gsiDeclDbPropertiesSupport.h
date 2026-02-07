
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

#ifndef HDR_dbPropertiesSupport
#define HDR_dbPropertiesSupport

#include "gsiDecl.h"
#include "tlTypeTraits.h"
#include "dbPropertiesRepository.h"
#include "dbObjectWithProperties.h"
#include "dbTrans.h"
#include "dbEdge.h"

namespace gsi
{

template <class T>
static void delete_property_meth_impl (T *s, const tl::Variant &key)
{
  db::properties_id_type id = s->properties_id ();
  if (id == 0) {
    return;
  }

  db::PropertiesSet props = db::properties (id);
  props.erase (key);
  s->properties_id (db::properties_id (props));
}

template <class T>
static void set_property_meth_impl (T *s, const tl::Variant &key, const tl::Variant &value)
{
  db::properties_id_type id = s->properties_id ();

  db::PropertiesSet props = db::properties (id);
  props.erase (key);
  props.insert (key, value);
  s->properties_id (db::properties_id (props));
}

template <class T>
static tl::Variant get_property_meth_impl (const T *s, const tl::Variant &key)
{
  db::properties_id_type id = s->properties_id ();

  const db::PropertiesSet &props = db::properties (id);
  return props.value (key);
}

template <class T>
static tl::Variant get_properties_meth_impl (const T *s)
{
  db::properties_id_type id = s->properties_id ();

  const db::PropertiesSet &props = db::properties (id);
  return props.to_dict_var ();
}

template <class T>
static typename tl::result_of_method<decltype (& T::scaled)>::type
scaled_meth_impl (const T *s, double scale)
{
  typename tl::result_of_method<decltype (& T::scaled)>::type res (s->scaled (scale), s->properties_id ());
  return res;
}

template <class T>
static T
transformed_meth_impl0 (const T *s, const db::simple_trans<typename T::coord_type> &tr)
{
  return s->transformed (tr);
}

template <class T>
static typename tl::result_of_method<decltype (& T::template transformed<db::complex_trans<typename T::coord_type, db::DCoord> >)>::type
transformed_meth_impl1 (const T *s, const db::complex_trans<typename T::coord_type, db::DCoord> &tr)
{
  typename tl::result_of_method<decltype (& T::template transformed<db::complex_trans<typename T::coord_type, db::DCoord> >)>::type res (s->transformed (tr), s->properties_id ());
  return res;
}

template <class T>
static typename tl::result_of_method<decltype (& T::template transformed<db::complex_trans<typename T::coord_type, db::Coord> >)>::type
transformed_meth_impl2 (const T *s, const db::complex_trans<typename T::coord_type, db::Coord> &tr)
{
  typename tl::result_of_method<decltype (& T::template transformed<db::complex_trans<typename T::coord_type, db::Coord> >)>::type res (s->transformed (tr), s->properties_id ());
  return res;
}

template <class T>
static void
transform_meth_impl0 (T *s, const db::simple_trans<typename T::coord_type> &tr)
{
  s->transform (tr);
}

template <class T>
static void
transform_meth_impl1 (T *s, const db::complex_trans<typename T::coord_type, typename T::coord_type> &tr)
{
  s->transform (tr);
}

template <class T>
static T &move_xy_meth_impl (T *s, typename T::coord_type dx, typename T::coord_type dy)
{
  return s->move (typename T::vector_type (dx, dy));
}

template <class T>
static T moved_xy_meth_impl (const T *s, typename T::coord_type dx, typename T::coord_type dy)
{
  return s->moved (typename T::vector_type (dx, dy));
}

template <class T>
struct downcast_impl_helper;

template <class T>
struct downcast_impl_helper<db::object_with_properties<T> >
{
  static T impl (const db::object_with_properties<T> *obj)
  {
    return *obj;
  }
};

template <class T>
static gsi::Methods properties_support_methods ()
{
  return
  gsi::method_ext ("downcast", &downcast_impl_helper<T>::impl,
    "@brief Gets the corresponding object without the properties\n"
  ) +
  gsi::method ("prop_id", (db::properties_id_type (T::*) () const) &T::properties_id,
    "@brief Gets the properties ID associated with the object\n"
  ) +
  gsi::method ("prop_id=", (void (T::*) (db::properties_id_type)) &T::properties_id, gsi::arg ("id"),
    "@brief Sets the properties ID of the object\n"
  ) +
  gsi::method_ext ("*", &scaled_meth_impl<T>, gsi::arg ("f"),
    "@brief Scales the object by some factor\n"
    "\n"
    "Returns the scaled object. All coordinates are multiplied with the given factor and, if "
    "necessary, rounded."
  ) +
  gsi::method_ext ("transform", &transform_meth_impl0<T>, gsi::arg ("t"),
    "@brief Transforms the object (in-place version)\n"
  ) +
  gsi::method_ext ("transform", &transform_meth_impl1<T>, gsi::arg ("t"),
    "@brief Transforms the object (in-place version)\n"
  ) +
  gsi::method_ext ("transformed", &transformed_meth_impl0<T>, gsi::arg ("t"),
    "@brief Returns the transformed object\n"
    "\n"
    "Returns a copy of the object, transformed by the given transformation. "
    "The result is equivalent to 'tr * self'."
  ) +
  gsi::method_ext ("transformed", &transformed_meth_impl1<T>, gsi::arg ("t"),
    "@brief Returns the transformed object\n"
    "\n"
    "Returns a copy of the object, transformed by the given transformation. "
    "The result is equivalent to 'tr * self'."
  ) +
  gsi::method_ext ("transformed", &transformed_meth_impl2<T>, gsi::arg ("t"),
    "@brief Returns the transformed object\n"
    "\n"
    "Returns a copy of the object, transformed by the given transformation. "
    "The result is equivalent to 'tr * self'."
  ) +
  gsi::method ("move", &T::move, gsi::arg ("v"),
    "@brief Moves the object.\n"
    "\n"
    "Moves the object by the given offset and returns the \n"
    "moved object. The object is overwritten.\n"
    "\n"
    "@param v The distance to move the object.\n"
    "\n"
    "@return The moved object (self).\n"
  ) +
  gsi::method_ext ("move", &move_xy_meth_impl<T>, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Moves the object.\n"
    "\n"
    "Moves the object by the given offset and returns the \n"
    "moved object. The object is overwritten.\n"
    "\n"
    "@param dx The x distance to move the object.\n"
    "@param dy The y distance to move the object.\n"
    "\n"
    "@return The moved object (self).\n"
  ) +
  gsi::method ("moved", &T::moved, gsi::arg ("v"),
    "@brief Returns the moved object\n"
    "\n"
    "Moves the object by the given offset and returns the \n"
    "moved object. The object is not modified.\n"
    "\n"
    "@param v The distance to move the object.\n"
    "\n"
    "@return The moved object.\n"
  ) +
  gsi::method_ext ("moved", &moved_xy_meth_impl<T>, gsi::arg ("dx", 0), gsi::arg ("dy", 0),
    "@brief Returns the moved object (does not modify self)\n"
    "\n"
    "Moves the object by the given offset and returns the \n"
    "moved object. The object is not modified.\n"
    "\n"
    "@param dx The x distance to move the object.\n"
    "@param dy The y distance to move the object.\n"
    "\n"
    "@return The moved object.\n"
  ) +
  gsi::method_ext ("delete_property", &delete_property_meth_impl<T>, gsi::arg ("key"),
    "@brief Deletes the user property with the given key\n"
    "This method is a convenience method that deletes the property with the given key. "
    "It does nothing if no property with that key exists. Using that method is more "
    "convenient than creating a new property set with a new ID and assigning that properties ID.\n"
    "This method may change the properties ID."
  ) +
  gsi::method_ext ("set_property", &set_property_meth_impl<T>, gsi::arg ("key"), gsi::arg ("value"),
    "@brief Sets the user property with the given key to the given value\n"
    "This method is a convenience method that sets the user property with the given key to the given value. "
    "If no property with that key exists, it will create one. Using that method is more "
    "convenient than creating a new property set with a new ID and assigning that properties ID.\n"
    "This method may change the properties ID. "
    "Note: GDS only supports integer keys. OASIS supports numeric and string keys.\n"
  ) +
  gsi::method_ext ("property", &get_property_meth_impl<T>, gsi::arg ("key"),
    "@brief Gets the user property with the given key\n"
    "This method is a convenience method that gets the user property with the given key. "
    "If no property with that key does not exist, it will return nil. Using that method is more "
    "convenient than using the layout object and the properties ID to retrieve the property value. "
  ) +
  gsi::method ("to_s", (std::string (T::*) () const) &T::to_string,
    "@brief Returns a string representing the polygon\n"
  ) +
  gsi::method_ext ("properties", &get_properties_meth_impl<T>,
    "@brief Gets the user properties\n"
    "This method is a convenience method that gets the properties of the shape as a single hash.\n"
  );
}

}

#endif
