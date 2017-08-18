
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

#ifndef HDR_bdInit
#define HDR_bdInit

#include "bdCommon.h"
#include "tlLog.h"  //  because of BD_MAIN

namespace bd
{

/**
 *  @brief Provides basic initialization
 *  This function must be called at the very beginning of the main program.
 */
void BD_PUBLIC init ();

/**
 *  @brief Provides a main () implementation
 *
 *  Use this macro like this:
 *
 *  @code
 *  #include "bdInit.h"
 *
 *  BD_MAIN_FUNC
 *  {
 *    .. your code. Use argc and argv for the arguments.
 *  }
 *
 *  BD_MAIN
 */

#define BD_MAIN \
  int main (int argc, char *argv []) \
  { \
    try { \
      bd::init (); \
      return main_func (argc, argv); \
    } catch (tl::CancelException & /*ex*/) { \
      return 1; \
    } catch (std::exception &ex) { \
      tl::error << ex.what (); \
      return 1; \
    } catch (tl::Exception &ex) { \
      tl::error << ex.msg (); \
      return 1; \
    } catch (...) { \
      tl::error << "unspecific error"; \
    } \
  }

#define BD_MAIN_FUNC \
  int main_func (int argc, char *argv [])

}

#endif
