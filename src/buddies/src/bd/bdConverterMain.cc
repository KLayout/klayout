
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "bdReaderOptions.h"
#include "bdConverterMain.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "tlCommandLineParser.h"

namespace bd
{

int converter_main (int argc, char *argv[], const std::string &format)
{
  bd::GenericWriterOptions generic_writer_options;
  bd::GenericReaderOptions generic_reader_options;
  std::string infile, outfile;

  tl::CommandLineOptions cmd;
  generic_writer_options.add_options (cmd, format);
  generic_reader_options.add_options (cmd);

  cmd << tl::arg ("input",  &infile,  "The input file (any format, may be gzip compressed)")
      << tl::arg ("output", &outfile, tl::sprintf ("The output file (%s format)", format))
    ;

  cmd.brief (tl::sprintf ("This program will convert the given file to a %s file", format));

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
    save_options.set_format (format);

    tl::OutputStream stream (outfile);
    db::Writer writer (save_options);
    writer.write (layout, stream);
  }

  return 0;
}

}
