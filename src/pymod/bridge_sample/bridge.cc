
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

PyMODINIT_FUNC
initbridge ()
{
  PyObject *m;

  m = Py_InitModule ("bridge", BridgeMethods);
  if (m == NULL) {
    return;
  }

  BridgeError = PyErr_NewException ((char *) "bridge.error", NULL, NULL);
  Py_INCREF (BridgeError);
  PyModule_AddObject (m, "error", BridgeError);
}
