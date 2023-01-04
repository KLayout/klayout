

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

/**
 *  @brief A helper include file to implement the Python modules
 *
 *  Use this helper file this way:
 *
 *  #include "pymodHelper.h"
 *  DEFINE_PYMOD(mymod, "mymod", "KLayout Test module klayout.mymod")
 */

#include <Python.h>

#include "pyaModule.h"
#include "pyaUtils.h"

#include "gsi.h"
#include "gsiExpression.h"

static PyObject *
module_init (const char *pymod_name, const char *mod_name, const char *mod_description)
{
  static pya::PythonModule module;

  PYA_TRY
  
    gsi::initialize ();

    //  required for the tiling processor for example
    gsi::initialize_expressions ();

    module.init (pymod_name, mod_description);
    module.make_classes (mod_name);

    return module.take_module ();

  PYA_CATCH_ANYWHERE
  
  return 0;
}

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s

#if PY_MAJOR_VERSION < 3

#define DEFINE_PYMOD(__name__, __name_str__, __description__) \
  extern "C" \
  DEF_INSIDE_PUBLIC \
  void init##__name__ () \
  { \
    module_init (STRINGIFY(__name__), __name_str__, __description__); \
  } \

#define DEFINE_PYMOD_WITH_INIT(__name__, __name_str__, __description__, __init__) \
  extern "C" \
  DEF_INSIDE_PUBLIC \
  void init##__name__ () \
  { \
    __init__ (STRINGIFY(__name__), __name_str__, __description__); \
  } \

#else

#define DEFINE_PYMOD(__name__, __name_str__, __description__) \
  extern "C" \
  DEF_INSIDE_PUBLIC \
  PyObject *PyInit_##__name__ () \
  { \
    return module_init (STRINGIFY(__name__), __name_str__, __description__); \
  } \

#define DEFINE_PYMOD_WITH_INIT(__name__, __name_str__, __description__, __init__) \
  extern "C" \
  DEF_INSIDE_PUBLIC \
  PyObject *PyInit_##__name__ () \
  { \
    return __init__ (STRINGIFY(__name__), __name_str__, __description__); \
  } \

#endif

