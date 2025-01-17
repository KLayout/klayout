
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

#include "bdInit.h"
#include "tlStaticObjects.h"
#include "rba.h"

#if defined(HAVE_QT)
#include <QCoreApplication>
#endif

BD_PUBLIC int BD_TARGET (int argc, char *argv []);

/**
 *  @brief The continuation function to support Ruby's special top-level hook
 */
static int main_cont (int &argc, char **argv)
{
#if defined(HAVE_QT)
  QCoreApplication app (argc, argv);
#endif
  return bd::_main_impl (&BD_TARGET, argc, argv);
}

/**
 *  @brief Provides a main () implementation
 *
 *  NOTE:
 *  This file is not part of the bd sources, but the template for the
 *  main() function of the various applications. It's configured through the
 *  BD_TARGET macro which is set to the application name in the app's .pro
 *  files.
 */
int main (int argc, char *argv [])
{
  //  This special initialization is required by the Ruby interpreter because it wants to mark the stack
  int ret = rba::RubyInterpreter::initialize (argc, argv, &main_cont);

  //  clean up all static data now, since we don't trust the static destructors.
  //  NOTE: this needs to happen after the Ruby interpreter went down since otherwise the GC will
  //  access objects that are already cleaned up.
  tl::StaticObjects::cleanup ();

  return ret;
}
