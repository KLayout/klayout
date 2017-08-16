
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

#include "bdInit.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbDXFWriter.h"

int 
main (int argc, char *argv [])
{
  if (argc != 3) {
    printf ("Syntax: strm2dxf <infile> <outfile>\n");
    return 1;
  }

  std::string infile (argv[1]);
  std::string outfile (argv[2]);

  try {

    db::Manager m;
    db::Layout layout (&m);
    db::LayerMap map;

    {
      tl::InputStream stream (infile);
      db::Reader reader (stream);
      map = reader.read (layout);
    }

    {
      tl::OutputStream stream (outfile);
      db::DXFWriter writer;
      writer.write (layout, stream, db::SaveLayoutOptions ());
    }

  } catch (std::exception &ex) {
    fprintf (stderr, "*** ERROR: %s\n", ex.what ());
    return 1;
  } catch (tl::Exception &ex) {
    fprintf (stderr, "*** ERROR: %s\n", ex.msg ().c_str ());
    return 1;
  } catch (...) {
    fprintf (stderr, "*** ERROR: unspecific error\n");
    return 1;
  }

  return 0;
}


