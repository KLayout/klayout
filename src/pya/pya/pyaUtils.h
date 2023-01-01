
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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


#ifndef _HDR_pyaUtils
#define _HDR_pyaUtils

#include "tlScriptError.h"

namespace pya
{



/**
 *  Some helper macros that translate C++ exceptions into Python errors
 */

#define PYA_TRY \
  { \
    try {

#define PYA_CATCH(where) \
    } catch (tl::ExitException &ex) { \
      PyErr_SetObject (PyExc_SystemExit, PyLong_FromLong (ex.status ())); \
    } catch (std::exception &ex) { \
      std::string msg = std::string(ex.what ()) + tl::to_string (tr (" in ")) + (where); \
      PyErr_SetString (PyExc_RuntimeError, msg.c_str ()); \
    } catch (tl::TypeError &ex) { \
      std::string msg; \
      msg = ex.msg () + tl::to_string (tr (" in ")) + (where); \
      PyErr_SetString (PyExc_TypeError, msg.c_str ()); \
    } catch (tl::Exception &ex) { \
      std::string msg; \
      msg = ex.msg () + tl::to_string (tr (" in ")) + (where); \
      PyErr_SetString (PyExc_RuntimeError, msg.c_str ()); \
    } catch (...) { \
      std::string msg = tl::to_string (tr ("Unspecific exception in ")) + (where); \
      PyErr_SetString (PyExc_RuntimeError, msg.c_str ()); \
    } \
  }

#define PYA_CATCH_ANYWHERE \
    } catch (tl::ExitException &ex) { \
      PyErr_SetObject (PyExc_SystemExit, PyLong_FromLong (ex.status ())); \
    } catch (std::exception &ex) { \
      PyErr_SetString (PyExc_RuntimeError, ex.what ()); \
    } catch (tl::Exception &ex) { \
      PyErr_SetString (PyExc_RuntimeError, ex.msg ().c_str ()); \
    } catch (...) { \
      PyErr_SetString (PyExc_RuntimeError, tl::to_string (tr ("Unspecific exception in ")).c_str ()); \
    } \
  }

/**
 *  @brief Turn Python errors into C++ exceptions
 */
void check_error ();

}

#endif
