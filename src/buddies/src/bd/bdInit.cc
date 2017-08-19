
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
#include "tlCommandLineParser.h"
#include "version.h"

namespace bd
{

void init ()
{
  std::string version = prg_version;
  version += " r";
  version += prg_rev;
  tl::CommandLineOptions::set_version (version);

  std::string license (prg_author);
  license += "\n";
  license += prg_date;
  license += ", Version ";
  license += prg_version;
  license += " r";
  license += prg_rev;
  license += "\n";
  license += "\n";
  license += prg_about_text;
  tl::CommandLineOptions::set_license (license);
}

int _main_impl (int (*delegate) (int, char *[]), int argc, char *argv[])
{
  try {
    init ();
    return (*delegate) (argc, argv);
  } catch (tl::CancelException & /*ex*/) {
    return 1;
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
}

}
