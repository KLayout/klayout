
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

#include "dbLayoutToNetlist.h"
#include "dbLayoutToNetlistReader.h"
#include "dbLayoutToNetlistWriter.h"
#include "dbStream.h"
#include "dbCommonReader.h"
#include "dbNetlistDeviceExtractorClasses.h"

#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

TEST(1_ReaderBasic)
{
  db::Layout ly;

  db::Cell &tc = ly.cell (ly.add_cell ("TOP"));
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::string in_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au.txt");
  tl::InputStream is_in (in_path);

  db::LayoutToNetlistStandardReader reader (is_in);
  reader.read (&l2n);

  //  verify against the input

  std::string path = tmp_file ("tmp_l2nreader_1.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au.txt");

  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  if (is.read_all () != is_au.read_all ()) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }
}
