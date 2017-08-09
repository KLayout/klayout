
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

#include "dbLayout.h"
#include "dbLayoutDiff.h"
#include "dbReader.h"

int 
main (int argc, char *argv [])
{
  if (argc != 3) {
    printf ("Syntax: strmcmp <infile-a> <infile-b>\n");
    return 1;
  }

  std::string infile_a (argv[1]);
  std::string infile_b (argv[2]);

  try {

    db::Manager m;
    db::Layout layout_a (false, &m);
    db::Layout layout_b (false, &m);

    {
      tl::InputStream stream (infile_a);
      db::Reader reader (stream);
      reader.read (layout_a);
    }

    {
      tl::InputStream stream (infile_b);
      db::Reader reader (stream);
      reader.read (layout_b);
    }

    if (! db::compare_layouts (layout_a, layout_b, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_no_text_orientation | db::layout_diff::f_verbose, 0 /*exact match*/)) {
      throw tl::Exception ("layouts differ");
    }

  } catch (std::exception &ex) {
    tl::error << ex.what ();
    return 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return 1;
  } catch (...) {
    tl::error << "unspecific error";
    return 1;
  }

  return 0;
}


