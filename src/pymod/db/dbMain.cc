
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include <Python.h>
#include "pya.h"
#include "gsi.h"
#include "gsiExpression.h"

static PyObject *module_init ()
{
  gsi::initialize ();
  gsi::initialize_expressions ();

  static pya::PythonModule module;
  module.init ("klayout.db", "KLayout core module (db)");
  module.make_classes ();

  return module.module ();
}

#if PY_MAJOR_VERSION < 3
PyMODINIT_FUNC
DEF_INSIDE_PUBLIC
initdb ()
{
  module_init ();
}
#else
PyMODINIT_FUNC
DEF_INSIDE_PUBLIC
PyMODINIT_FUNC PyInit_themodulename ()
{
  return module_init();
}
#endif
