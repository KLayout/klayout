
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "bdWriterOptions.h"
#include "dbLayout.h"
#include "dbSaveLayoutOptions.h"
#include "tlCommandLineParser.h"
#include "tlGlobPattern.h"

namespace bd
{

GenericWriterOptions::GenericWriterOptions ()
  : scale_factor (1.0), dbu (0.0),
    dont_write_empty_cells (false), keep_instances (false), write_context_info (false)
{
  //  .. nothing yet ..
}

void
GenericWriterOptions::add_options (tl::CommandLineOptions &cmd, const std::string &format)
{
  cmd << tl::arg ("-os|--scale-factor=factor", &scale_factor, "Scales the layout upon writing",
                  "Specifies layout scaling. If given, the saved layout will be scaled by the "
                  "given factor."
                 );

  if (format == "GDS2" || format == "GDS2Text" || format == "OASIS") {
    cmd << tl::arg ("-ou|--dbu=dbu",           &dbu, "Uses the specified database unit",
                    "Specifies the database unit to save the layout in. The database unit is given "
                    "in micron units. By default, the original unit is used. The layout will not "
                    "change physically because internally, the coordinates are scaled to match the "
                    "new database unit."
                   );
  }

  cmd << tl::arg ("-ox|--drop-empty-cells",    &dont_write_empty_cells, "Drops empty cells",
                  "If given, empty cells won't be written. See --keep-instances for more options."
                 );

  if (format == "GDS2" || format == "GDS2Text") {
    cmd << tl::arg ("-ok|--keep-instances",      &keep_instances, "Keeps instances of dropped cells",
                    "If given, instances of dropped cell's won't be removed. Hence, ghost cells are "
                    "produced. The resulting layout may not be readable by consumers that require "
                    "all instantiated cells to be present as actual cells."
                   );
  }

  if (format == "GDS2" || format == "GDS2Text" || format == "OASIS") {
    cmd << tl::arg ("-oc|--write-context-info",  &write_context_info, "Writes context information",
                    "Include context information for PCell instances and other information in a format-specific "
                    "way. The resulting layout may show unexpected features for other consumers."
                   );
  }

  cmd << tl::arg ("-ow|--write-cells=sel",       &cell_selection, "Specifies cells to write",
                  "This option specifies the cells to write. The value of this option is a sequence of "
                  "select/unselect operations. A select operation is an optional plus sign (+), followed by "
                  "a cell filter. An unselect operation is a minus sign (-) followed by a cell filter. "
                  "A cell filter is a plain cell name, a glob pattern (using '*' and '?' for placeholders). "
                  "If a cell filter is enclosed in round brackets, only this cell is specified. Otherwise "
                  "the cell and it's children are specified.\n"
                  "\n"
                  "Multiple operations can be specified by adding them with a comma separator. "
                  "Cell selection and unselection happens in the order given. Hence it's possible "
                  "to select a cell with it's children and then unselect some children of this cell.\n"
                  "\n"
                  "Examples:\n\n"
                  "* \"TOP1,TOP2\" - Select cells TOP1 and TOP2 with all of their children\n"
                  "* \"(TOP)\" - Select only cell TOP, but none of it's child cells\n"
                  "* \"TOP,-A\" - Select cell TOP (plus children), then remove A (with children)"
                 );
}

static void get_selected_cells (tl::Extractor &ex, const db::Layout &layout, std::set<db::cell_index_type> &selected)
{
  while (! ex.at_end ()) {

    bool remove = ex.test ("-");
    ex.test ("+");

    bool without_children = ex.test ("(");
    std::string filter;
    ex.read_word_or_quoted (filter, "_-.*?{}$[]");

    if (without_children) {
      ex.expect (")");
    }

    if (! ex.at_end ()) {
      ex.expect (",");
    }

    tl::GlobPattern pat (filter);
    for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {

      if (pat.match (layout.cell_name (c->cell_index ()))) {

        std::set<db::cell_index_type> cells;
        cells.insert (c->cell_index ());
        if (! without_children) {
          c->collect_called_cells (cells);
        }

        if (! remove) {
          selected.insert (cells.begin (), cells.end ());
        } else {
          selected.erase (cells.begin (), cells.end ());
        }

      }

    }

  }
}

void
GenericWriterOptions::configure (db::SaveLayoutOptions &save_options, const db::Layout &layout)
{
  save_options.set_scale_factor (scale_factor);
  save_options.set_dbu (dbu);
  save_options.set_dont_write_empty_cells (dont_write_empty_cells);
  save_options.set_keep_instances (keep_instances);
  save_options.set_write_context_info (write_context_info);

  if (!cell_selection.empty ()) {

    std::set<db::cell_index_type> selected;
    tl::Extractor ex (cell_selection.c_str ());
    get_selected_cells (ex, layout, selected);

    save_options.clear_cells ();
    for (std::set<db::cell_index_type>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
      save_options.add_this_cell (*s);
    }

  }
}

}
