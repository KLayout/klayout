
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbTestSupport.h"
#include "dbCommonReader.h"
#include "dbStreamLayers.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCell.h"
#include "dbCellInst.h"
#include "dbLayoutDiff.h"

#include "tlUnitTest.h"
#include "tlFileUtils.h"

namespace db
{

void compare_layouts (tl::TestBase *_this, const db::Layout &layout, const std::string &au_file, NormalizationMode norm, db::Coord tolerance)
{
  compare_layouts (_this, layout, au_file, db::LayerMap (), true, norm, tolerance);
}

void compare_layouts (tl::TestBase *_this, const db::Layout &layout, const std::string &au_file, const db::LayerMap &lm, bool read_other_layers, NormalizationMode norm, db::Coord tolerance)
{
  //  normalize the layout by writing to GDS and reading from ..

  //  generate a "unique" name ...
  unsigned int hash = 0;
  for (const char *cp = au_file.c_str (); *cp; ++cp) {
    hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
  }

  const db::Layout *subject = 0;
  db::Layout layout2;

  std::string tmp_file;
  db::SaveLayoutOptions options;

  if (norm == WriteGDS2) {
    tmp_file = _this->tmp_file (tl::sprintf ("tmp_%x.gds", hash));
    options.set_format ("GDS2");
  } else if (norm == WriteOAS) {
    tmp_file = _this->tmp_file (tl::sprintf ("tmp_%x.oas", hash));
    options.set_format ("OASIS");
  } else {
    //  write the temp file in the same format than the au file
    tmp_file = _this->tmp_file (tl::sprintf ("tmp_%x." + tl::extension (au_file), hash));
    options.set_format_from_filename (tmp_file);
  }

  {
    tl::OutputStream stream (tmp_file.c_str ());
    db::Writer writer (options);
    writer.write (const_cast<db::Layout &> (layout), stream);
  }

  if (norm == WriteGDS2 || norm == WriteOAS) {

    //  read all layers from the original layout, so the layer table is the same
    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
      layout2.insert_layer ((*l).first, *(*l).second);
    }

    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (layout2);

    subject = &layout2;

  } else {
    subject = &layout;
  }

  bool equal = false;
  bool any = false;

  int n = 0;
  for ( ; ! equal; ++n) {

    db::Layout layout_au;

    //  read all layers from the original layout, so the layer table is the same
    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
      layout_au.insert_layer ((*l).first, *(*l).second);
    }

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lm;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = read_other_layers;

    std::string fn = au_file;
    if (n > 0) {
      fn += tl::sprintf (".%d", n);
    }

    if (tl::file_exists (fn)) {

      if (n == 1 && any) {
        throw tl::Exception (tl::sprintf ("Inconsistent reference variants for %s: there can be either variants (.1,.2,... suffix) or a single file (without suffix)", au_file));
      }

      any = true;

      tl::InputStream stream (fn);
      db::Reader reader (stream);
      reader.read (layout_au, options);

      equal = db::compare_layouts (*subject, layout_au,
                                     (n > 0 ? db::layout_diff::f_silent : db::layout_diff::f_verbose)
                                     | (norm == AsPolygons ? db::layout_diff::f_boxes_as_polygons + db::layout_diff::f_paths_as_polygons : 0)
                                     | db::layout_diff::f_flatten_array_insts
                                   /*| db::layout_diff::f_no_text_details | db::layout_diff::f_no_text_orientation*/
                                   , tolerance, 100 /*max diff lines*/);
      if (equal && n > 0) {
        tl::info << tl::sprintf ("Found match on golden reference variant %s", fn);
      }

    } else if (n > 0) {
      if (! any) {
        tl::warn << tl::sprintf ("No golden data found (%s)", au_file);
      }
      break;
    }

  }

  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s%s",
                               tl::absolute_file_path (tmp_file),
                               tl::absolute_file_path (au_file),
                               (n > 1 ? "\nand variants" : "")));
  }
}

}
