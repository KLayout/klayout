
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


#include "gsiDecl.h"
#include "layLayoutHandle.h"

namespace gsi
{

static std::vector <std::string> get_names ()
{
  std::vector <std::string> names;
  lay::LayoutHandle::get_names (names);
  return names;
}

static lay::LayoutHandleRef *find (const std::string &name)
{
  auto h = lay::LayoutHandle::find (name);
  return h ? new lay::LayoutHandleRef (h) : 0;
}

static lay::LayoutHandleRef *find_layout (const db::Layout *layout)
{
  auto h = lay::LayoutHandle::find_layout (layout);
  return h ? new lay::LayoutHandleRef (h) : 0;
}

static lay::LayoutHandleRef *new_handle1 (const std::string &filename, const db::LoadLayoutOptions &options, const std::string &technology)
{
  std::unique_ptr<lay::LayoutHandle> handle (new lay::LayoutHandle (new db::Layout (), filename));
  handle->load (options, technology);
  return new lay::LayoutHandleRef (handle.release ());
}

static lay::LayoutHandleRef *new_handle2 (db::Layout *layout)
{
  return new lay::LayoutHandleRef (new lay::LayoutHandle (layout, std::string ()));
}

static bool is_valid (const lay::LayoutHandleRef *ref)
{
  return ref->get () != 0;
}

static const std::string &name (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return ref->get ()->name ();
}

static void set_name (lay::LayoutHandleRef *ref, const std::string &name)
{
  tl_assert (ref->get () != 0);
  ref->get ()->rename (name, true);
}

static db::Layout *layout (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return &ref->get ()->layout ();
}

static const std::string &filename (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return ref->get ()->filename ();
}

static int get_ref_count (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return ref->get ()->get_ref_count ();
}

static bool is_dirty (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return ref->get ()->is_dirty ();
}

static void save_as (lay::LayoutHandleRef *ref, const std::string &filename, const db::SaveLayoutOptions &options, int keep_backups)
{
  tl_assert (ref->get () != 0);
  ref->get ()->save_as (filename, tl::OutputStream::OM_Auto, options, true, keep_backups);
  ref->get ()->set_save_options (options, true);
}

static tl::Variant save_options (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return ref->get ()->save_options_valid () ? tl::Variant (ref->get ()->save_options ()) : tl::Variant ();
}

static const db::LoadLayoutOptions &load_options (const lay::LayoutHandleRef *ref)
{
  tl_assert (ref->get () != 0);
  return ref->get ()->load_options ();
}


Class<lay::LayoutHandleRef> decl_LayoutHandle ("lay", "LayoutHandle",
  gsi::method ("names", &get_names,
    "@brief Gets the names of all layout handles registered currently.\n"
  ) +
  gsi::constructor ("find", &find, gsi::arg ("name"),
    "@brief Finds a layout handle by name.\n"
    "If no handle with that name exists, nil is returned."
  ) +
  gsi::constructor ("find", &find_layout, gsi::arg ("layout"),
    "@brief Finds a layout handle for the given layout object.\n"
    "If no handle for that layout exists, nil is returned."
  ) +
  gsi::constructor ("new", &new_handle1, gsi::arg ("filename"), gsi::arg ("options"), gsi::arg ("technology", std::string ()),
    "@brief Creates a handle from a file.\n"
    "Loads the layout and creates a handle from that layout. The name is derived from the file name initially. "
    "You can pass load options and assign a technology using 'options' and 'technology' respectively."
  ) +
  gsi::constructor ("new", &new_handle2, gsi::arg ("layout"),
    "@brief Creates a handle from an existing layout object.\n"
    "Creates a layout handle for an existing layout object. The ownership over the layout object is transferred to the handle. "
  ) +
  gsi::method_ext ("is_valid?", &is_valid,
    "@brief Gets a value indicating whether the layout handle is valid.\n"
    "Invalid layout handles cannot be used. Default-created layout handles are invalid for example."
  ) +
  gsi::method_ext ("name", &name,
    "@brief Gets the name of the layout handle.\n"
    "The name identifies a layout handle in the global context. Names should be unique. "
    "Handles can be retrieved by name using \\find."
  ) +
  gsi::method_ext ("name=", &set_name,
    "@brief Sets the name of the layout handle.\n"
    "The caller is responsible for selecting a unique name for the handle. Only uniquely named handles "
    "can be retrieved by \\find."
  ) +
  gsi::method_ext ("layout", &layout,
    "@brief Gets the layout object kept by the handle.\n"
  ) +
  gsi::method_ext ("filename", &filename,
    "@brief Gets the file name the layout was loaded from.\n"
    "If the handle was not constructed from a file, this attribute is an empty string."
  ) +
  gsi::method_ext ("ref_count", &get_ref_count,
    "@brief Gets the reference count.\n"
    "The reference count indicates how many references are present for the handle. If the "
    "reference count reaches zero, the layout object is destroyed. References are kept by "
    "layout views showing the layout, but can also be kept by separate handles."
  ) +
  gsi::method_ext ("is_dirty?", &is_dirty,
    "@brief Gets a value indicating whether the layout needs saving.\n"
  ) +
  gsi::method_ext ("save_as?", &save_as, gsi::arg ("filename"), gsi::arg ("options"), gsi::arg ("keep_backups", 0),
    "@brief Saves the layout to a file.\n"
    "This method will save the layout kept by the handle to the given file. Calling this method will change "
    "the file name and reset the \\is_dirty? flag and set \\save_options to the options passed.\n"
    "\n"
    "@param filename The path where to save the layout to\n"
    "@param options The options used for saving the file\n"
    "@param keep_backups The number of backup files to keep (0 for 'no backups')\n"
  ) +
  gsi::method_ext ("save_options", &save_options,
    "@brief Gets the save options used most recently when saving the file or 'nil' if no save options are available."
  ) +
  gsi::method_ext ("load_options", &load_options,
    "@brief Gets the load options used when the layout was read.\n"
    "Default options will be returned when the layout was not created from a file."
  ),
  "@brief A handle to a global layout object\n"
  "Layout objects shown in layout views are stored in a global repository. The layout handle is a pointer "
  "into that repository. Handles are reference counted - when no handle points to a layout, the layout is discarded.\n"
  "A handle stores some more information about the layout, i.e. whether it is 'dirty' (needs saving) and the "
  "options used for loading or saving the layout.\n"
  "\n"
  "The layout handle object is useful to hold a separate reference to a layout. This way it is possible "
  "to close a layout view, but still have a reference to the layout:\n"
  "\n"
  "You can use layout handles to move a layout to a different view for example:\n"
  "\n"
  "@code\n"
  "cellview = ...   # the source cellview\n"
  "new_view = ...   # the new target view\n"
  "\n"
  "h = cellview.handle\n"
  "cellview.close\n"
  "new_view.show_layout(h, true)\n"
  "@/code\n"
  "\n"
  "Handles are named. You can use \\LayoutHandle#find the find a handle by name or to obtain the handle for "
  "a given layout object. You can use \\LayoutHandle#names to get the names of all handles registered in the system.\n"
  "\n"
  "This class has been introduced in version 0.30.7."
);

}
