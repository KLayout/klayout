
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

/*

  This Python library demonstrates the use of the GSI/Python binding
  API for developing "bridge" applications - i.e. libraries that access
  the KLayout objects through their C++ API.

  This sample library provides two conversion functions:

    bridge.p2a(poly)    Converts pya.DSimplePolygon objects to Python
                        arrays with the structure [ (x, y), ... ].

    bridge.a2p(array)   Does the inverse transformation

  Use cases for such libraries are fast C++ based conversion of KLayout
  objects into other objects and vice versa.

*/

#include <Python.h>

#include "pyaConvert.h"
#include "pyaRefs.h"
#include "dbPolygon.h"

static PyObject *BridgeError;

static PyObject *
bridge_a2p (PyObject * /*self*/, PyObject *args)
{
  PyObject *a = NULL;
  if (! PyArg_ParseTuple (args, "O", &a)) {
    return NULL;
  }

  //  Iterate over the array elements
  pya::PythonRef iterator (PyObject_GetIter (a));
  if (! iterator) {
    return NULL;
  }

  //  Prepare a vector of points we can create the polygon from later
  std::vector<db::DPoint> points;

  PyObject *item;
  while ((item = PyIter_Next (iterator.get ())) != NULL) {

    //  Iterate over the x/y pair
    pya::PythonRef xy_iterator (PyObject_GetIter (item));
    if (! xy_iterator) {
      return NULL;
    }

    double c[2] = { 0.0, 0.0 };

    //  Gets the x and y value
    for (int i = 0; i < 2; ++i) {
      pya::PythonRef xy_item (PyIter_Next (xy_iterator.get ()));
      if (! xy_item) {
        return NULL;
      }
      if (pya::test_type<double> (xy_item.get ())) {
        c[i] = pya::python2c<double> (xy_item.get ());
      }
    }

    points.push_back (db::DPoint (c[0], c[1]));

  }

  //  Handle iteration errors
  if (PyErr_Occurred()) {
    return NULL;
  }

  //  Create and return a new object of db::DSimplePolygon type
  db::DSimplePolygon *poly = new db::DSimplePolygon ();
  poly->assign_hull (points.begin (), points.end ());
  return pya::c2python_new<db::DSimplePolygon> (poly);
}

static PyObject *
bridge_p2a (PyObject * /*self*/, PyObject *args)
{
  //  Parse the command line arguments
  PyObject *p = NULL;
  if (! PyArg_ParseTuple (args, "O", &p)) {
    return NULL;
  }

  //  Report an error if the input isn't a db::DSimplePolygon
  if (! pya::test_type<const db::DSimplePolygon &> (p)) {
    PyErr_SetString (BridgeError, "Expected a db::DSimplePolygon type");
    return NULL;
  }

  //  Obtain the db::DSimplePolygon
  const db::DSimplePolygon &poly = pya::python2c<const db::DSimplePolygon &> (p);

  //  Prepare an array for the points
  PyObject *array = PyList_New (poly.hull ().size ());
  Py_INCREF (array);

  //  Iterate over the points and fill the array with x/y tuples
  int i = 0;
  for (db::DSimplePolygon::polygon_contour_iterator pt = poly.hull ().begin (); pt != poly.hull ().end (); ++pt, ++i) {
    PyObject *point = PyTuple_New (2);
    PyTuple_SET_ITEM (point, 0, pya::c2python ((*pt).x ()));
    PyTuple_SET_ITEM (point, 1, pya::c2python ((*pt).y ()));
    PyList_SetItem (array, i, point);
  }

  return array;
}

static PyMethodDef BridgeMethods[] = {
  {
    "p2a", bridge_p2a, METH_VARARGS,
    "Converts a DSimplePolygon to an array."
  },
  {
    "a2p", bridge_a2p, METH_VARARGS,
    "Converts an array to a DSimplePolygon."
  },
  { NULL, NULL, 0, NULL }  //  terminal
};

#if PY_MAJOR_VERSION < 3

PyMODINIT_FUNC
initbridge_mod ()
{
  PyObject *m;

  m = Py_InitModule ("bridge_mod", BridgeMethods);
  if (m == NULL) {
    return;
  }

  BridgeError = PyErr_NewException ((char *) "bridge_mod.error", NULL, NULL);
  Py_INCREF (BridgeError);
  PyModule_AddObject (m, "error", BridgeError);
}

#else

static
struct PyModuleDef bridge_module =
{
  PyModuleDef_HEAD_INIT,
  "bridge_mod",
  NULL,
  -1,
  BridgeMethods
};

PyMODINIT_FUNC
PyInit_bridge_mod ()
{
  PyObject *m;

  m = PyModule_Create (&bridge_module);
  if (m == NULL) {
    return NULL;
  }

  BridgeError = PyErr_NewException ((char *) "bridge_mod.error", NULL, NULL);
  Py_INCREF (BridgeError);
  PyModule_AddObject (m, "error", BridgeError);

  return m;
}

#endif
