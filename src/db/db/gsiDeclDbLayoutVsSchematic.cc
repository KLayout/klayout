
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
#include "dbLayoutVsSchematic.h"
#include "tlStream.h"
#include "tlVariant.h"

namespace gsi
{

extern Class<db::LayoutToNetlist> decl_dbLayoutToNetlist;

static db::LayoutVsSchematic *make_lvs (const db::RecursiveShapeIterator &iter)
{
  return new db::LayoutVsSchematic (iter);
}

static db::LayoutVsSchematic *make_lvs_default ()
{
  return new db::LayoutVsSchematic ();
}

static db::LayoutVsSchematic *make_lvs_from_existing_dss_with_layout (db::DeepShapeStore *dss, unsigned int layout_index)
{
  return new db::LayoutVsSchematic (dss, layout_index);
}

static db::LayoutVsSchematic *make_lvs_from_existing_dss (db::DeepShapeStore *dss)
{
  return new db::LayoutVsSchematic (dss);
}

static db::LayoutVsSchematic *make_lvs_flat (const std::string &topcell_name, double dbu)
{
  return new db::LayoutVsSchematic (topcell_name, dbu);
}

static void save_l2n (db::LayoutVsSchematic *lvs, const std::string &path, bool short_format)
{
  lvs->db::LayoutToNetlist::save (path, short_format);
}

static void load_l2n (db::LayoutVsSchematic *lvs, const std::string &path)
{
  lvs->db::LayoutToNetlist::load (path);
}

Class<db::LayoutVsSchematic> decl_dbLayoutVsSchematic (decl_dbLayoutToNetlist, "db", "LayoutVsSchematic",
  gsi::constructor ("new", &make_lvs, gsi::arg ("iter"),
    "@brief Creates a new LVS object with the extractor connected to an original layout\n"
    "This constructor will attach the extractor of the LVS object to an original layout through the "
    "shape iterator.\n"
  ) +
  gsi::constructor ("new", &make_lvs_default,
    "@brief Creates a new LVS object\n"
    "The main objective for this constructor is to create an object suitable for reading and writing LVS database files.\n"
  ) +
  gsi::constructor ("new", &make_lvs_from_existing_dss, gsi::arg ("dss"),
    "@brief Creates a new LVS object with the extractor object reusing an existing \\DeepShapeStore object\n"
    "See the corresponding constructor of the \\LayoutToNetlist object for more details."
  ) +
  gsi::constructor ("new", &make_lvs_from_existing_dss_with_layout, gsi::arg ("dss"), gsi::arg ("layout_index"),
    "@brief Creates a new LVS object with the extractor object reusing an existing \\DeepShapeStore object\n"
    "See the corresponding constructor of the \\LayoutToNetlist object for more details."
  ) +
  gsi::constructor ("new", &make_lvs_flat, gsi::arg ("topcell_name"), gsi::arg ("dbu"),
    "@brief Creates a new LVS object with the extractor object taking a flat DSS\n"
    "See the corresponding constructor of the \\LayoutToNetlist object for more details."
  ) +
  gsi::method ("reference=", &db::LayoutVsSchematic::set_reference_netlist, gsi::arg ("reference_netlist"),
    "@brief Sets the reference netlist.\n"
    "This will set the reference netlist used inside \\compare as the second netlist to compare against "
    "the layout-extracted netlist.\n"
    "\n"
    "The LVS object will take ownership over the netlist - i.e. if it goes out of scope, the "
    "reference netlist is deleted.\n"
  ) +
  gsi::method ("reference", (db::Netlist *(db::LayoutVsSchematic::*) ()) &db::LayoutVsSchematic::reference_netlist,
    "@brief Gets the reference netlist.\n"
  ) +
  gsi::method ("compare", &db::LayoutVsSchematic::compare_netlists, gsi::arg ("comparer"),
    "@brief Compare the layout-extracted netlist against the reference netlist using the given netlist comparer.\n"
  ) +
  gsi::method ("xref", (db::NetlistCrossReference *(db::LayoutVsSchematic::*) ()) &db::LayoutVsSchematic::cross_ref,
    "@brief Gets the cross-reference object\n"
    "The cross-reference object is created while comparing the layout-extracted netlist against the "
    "reference netlist - i.e. during \\compare. Before \\compare is called, this object is nil.\n"
    "It holds the results of the comparison - a cross-reference between the nets and other objects "
    "in the match case and a listing of non-matching nets and other objects for the non-matching cases."
    "\n"
    "See \\NetlistCrossReference for more details.\n"
  ) +
  gsi::method_ext ("write_l2n", &save_l2n, gsi::arg ("path"), gsi::arg ("short_format", false),
    "@brief Writes the \\LayoutToNetlist part of the object to a file.\n"
    "This method employs the native format of KLayout.\n"
  ) +
  gsi::method_ext ("read_l2n", &load_l2n, gsi::arg ("path"),
    "@brief Reads the \\LayoutToNetlist part of the object from a file.\n"
    "This method employs the native format of KLayout.\n"
  ) +
  gsi::method ("write", &db::LayoutVsSchematic::save, gsi::arg ("path"), gsi::arg ("short_format", false),
    "@brief Writes the LVS object to a file.\n"
    "This method employs the native format of KLayout.\n"
  ) +
  gsi::method ("read", &db::LayoutVsSchematic::load, gsi::arg ("path"),
    "@brief Reads the LVS object from the file.\n"
    "This method employs the native format of KLayout.\n"
  ),
  "@brief A generic framework for doing LVS (layout vs. schematic)\n"
  "\n"
  "This class extends the concept of the netlist extraction from a layout to LVS verification. "
  "It does so by adding these concepts to the \\LayoutToNetlist class:\n"
  "\n"
  "@ul\n"
  "@li A reference netlist. This will be the netlist against which the layout-derived netlist is "
  "compared against. See \\reference and \\reference=.\n"
  "@/li\n"
  "@li A compare step. During the compare the layout-derived netlist and the reference netlists "
  "are compared. The compare results are captured in the cross-reference object. "
  "See \\compare and \\NetlistComparer for the comparer object.\n"
  "@/li\n"
  "@li A cross-reference. This object (of class \\NetlistCrossReference) will keep the relations "
  "between the objects of the two netlists. It also lists the differences between the netlists. "
  "See \\xref about how to access this object."
  "@/li\n"
  "@/ul\n"
  "\n"
  "The LVS object can be persisted to and from a file in a specific format, so it is sometimes "
  "referred to as the \"LVS database\".\n"
  "\n"
  "LVS objects can be attached to layout views with \\LayoutView#add_lvsdb so they become available in the "
  "netlist database browser.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}
