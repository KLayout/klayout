
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
#include "dbCIFWriter.h"
#include "tlLog.h"
#include "tlCommandLineParser.h"

#include <QFileInfo>

int 
main (int argc, char *argv [])
{
  bd::init ();

  db::SaveLayoutOptions save_options;
  db::CIFWriterOptions cif_options;
  std::string infile, outfile;

  tl::CommandLineOptions cmd;

  cmd << tl::arg ("-od|--dummy-calls",         &cif_options.dummy_calls,       "Produces dummy calls",
                  "If this option is given, the writer will produce dummy cell calls on global level for all top cells"
                 )
      << tl::arg ("-ob|--blank-separator",     &cif_options.blank_separator,   "Uses blanks as x/y separators",
                  "If this option is given, blank characters will be used to separate x and y values. "
                  "Otherwise comma characters will be used.\n"
                  "Use this option if your CIF consumer cannot read comma characters as x/y separators."
                 )
      << tl::arg ("-os|--scale-factor=factor", &save_options,  &db::SaveLayoutOptions::set_scale_factor,   "Scales the layout upon writing",
                  "Specifies layout scaling. If given, the saved layout will be scaled by the "
                  "given factor."
                 )
      << tl::arg ("input",                     &infile,                        "The input file (any format, may be gzip compressed)")
      << tl::arg ("output",                    &outfile,                       "The output file")
    ;

  save_options.set_options (cif_options);

  cmd.brief ("This program will convert the given file to a CIF file");

  try {

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
      tl::OutputStream stream (outfile);
      db::CIFWriter writer;
      writer.write (layout, stream, save_options);
    }

  } catch (tl::CancelException &ex) {
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

  return 0;
}


