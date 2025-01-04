
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include <QApplication>

#include "gtfUiDialog.h"
#include "tlLog.h"

int main (int argc, char *argv[]) 
{
  int result = 0;

  try {

    std::string fn_au, fn_current;
    unsigned int fn_count = 0;

    for (int i = 1; i < argc; ++i) {

      std::string a = argv [i];

      if (a == "-d" && (i + 1) < argc) {

        int v = std::max (0, atoi (argv [++i]));
        tl::verbosity (v);

      } else if (a == "-h") {

        tl::info << tl::to_string (QObject::tr ("gtfui [<options>] [<file-au>] [<file-current>]")) << tl::endl
                 << tl::to_string (QObject::tr ("options")) << tl::endl
                 << tl::to_string (QObject::tr ("  -d <debug level>   Set debug level")) << tl::endl
                 ;
        exit (0);

      } else if (fn_count == 0) {
        fn_au = a;
        ++fn_count;
      } else if (fn_count == 1) {
        fn_current = a;
        ++fn_count;
      } else {
        throw tl::Exception (tl::to_string (QObject::tr ("Too many file name arguments (usr -h to show usage)")));
      }
    }

    if (fn_count < 2) {
      throw tl::Exception (tl::to_string (QObject::tr ("Too few file name arguments (usr -h to show usage)")));
    }

    QApplication app (argc, argv);

    gtf::UiDialog *dialog = new gtf::UiDialog ();
    dialog->open_files (fn_au, fn_current);
    dialog->show ();

    result = app.exec ();

    delete dialog;

  } catch (std::exception &ex) {
    tl::error << ex.what ();
    result = 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    result = 1;
  } catch (...) {
    tl::error << tl::to_string (QObject::tr ("unspecific error"));
    result = 1;
  }

  return result;

}

