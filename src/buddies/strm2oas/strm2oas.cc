
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
#include "dbLayout.h"
#include "dbReader.h"
#include "dbOASISWriter.h"
#include "tlCommandLineParser.h"

int
main_func (int argc, char *argv [])
{
  bd::init ();

  bd::GenericWriterOptions generic_writer_options;
  std::string infile, outfile;

  tl::CommandLineOptions cmd;
  generic_writer_options.add_options_for_oasis (cmd);

  cmd << tl::arg ("input",                     &infile,                        "The input file (any format, may be gzip compressed)")
      << tl::arg ("output",                    &outfile,                       "The output file")
    ;

  cmd.brief ("This program will convert the given file to an OASIS file");

  cmd.parse (argc, argv);

  db::Manager m;
  db::Layout layout (&m);
  db::LayerMap map;

  {
    tl::InputStream stream (infile);
    db::Reader reader (stream);
    map = reader.read (layout);
  }

  {
    db::SaveLayoutOptions save_options;
    generic_writer_options.configure (save_options, layout);

    tl::OutputStream stream (outfile);
    db::OASISWriter writer;
    writer.write (layout, stream, save_options);
  }

  return 0;
}

int
main (int argc, char *argv [])
{
  try {
    return main_func (argc, argv);
  } catch (tl::CancelException & /*ex*/) {
    return 1;
  } catch (std::exception &ex) {
    tl::error << ex.what ();
    return 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return 1;
  } catch (...) {
    tl::error << "ERROR: unspecific error";
  }
}
