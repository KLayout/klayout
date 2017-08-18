
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
#include "bdWriterOptions.h"
#include "bdReaderOptions.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbGDS2Writer.h"
#include "tlCommandLineParser.h"

BD_MAIN_FUNC
{
  bd::GenericWriterOptions generic_writer_options;
  bd::GenericReaderOptions generic_reader_options;
  std::string infile, outfile;

  tl::CommandLineOptions cmd;
  generic_writer_options.add_options_for_gds2 (cmd);
  generic_reader_options.add_options (cmd);

  cmd << tl::arg ("input",                     &infile,                        "The input file (any format, may be gzip compressed)")
      << tl::arg ("output",                    &outfile,                       "The output file")
    ;

  cmd.brief ("This program will convert the given file to a GDS2 file");

  cmd.parse (argc, argv);

  db::Layout layout;

  {
    db::LoadLayoutOptions load_options;
    generic_reader_options.configure (load_options);

    tl::InputStream stream (infile);
    db::Reader reader (stream);
    reader.read (layout, load_options);
  }

  {
    db::SaveLayoutOptions save_options;
    generic_writer_options.configure (save_options, layout);

    tl::OutputStream stream (outfile);
    db::GDS2Writer writer;
    writer.write (layout, stream, save_options);
  }

  return 0;
}

BD_MAIN
