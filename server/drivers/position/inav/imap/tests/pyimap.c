/**************************************************************************
 * Desc: Python bindings for the local map library
 * Author: Andrew Howard
 * Date: 5 Nov 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <Python.h>

#include <rtk.h>
#include "imap.h"


// Python wrapper for imap type
typedef struct
{
  PyObject_HEAD
  imap_t *imap;
} pyimap_t;

staticforward PyTypeObject imap_type;
staticforward PyMethodDef imap_methods[];


// Create a imap
static PyObject *pyimap_alloc(PyObject *self, PyObject *args)
{
  pyimap_t *pyimap;
  double size_x, size_y;
  double scale, max_occ_dist, max_fit_dist;

  if (!PyArg_ParseTuple(args, "ddddd", &size_x, &size_y, &scale,
                        &max_occ_dist, &max_fit_dist))
    return NULL;

  pyimap = PyObject_New(pyimap_t, &imap_type);
  pyimap->imap = imap_alloc(size_x / scale, size_y / scale, scale,
                            max_occ_dist, max_fit_dist);
  
  return (PyObject*) pyimap;
}


// Destroy the imap
static void pyimap_free(PyObject *self)
{
  pyimap_t *pyimap;

  pyimap = (pyimap_t*) self;
  imap_free(pyimap->imap);
  PyObject_Del(self);

  return;
}


// Get imap attributes
static PyObject *pyimap_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  pyimap_t *pyimap;

  pyimap = (pyimap_t*) self;

  if (strcmp(attrname, "cptr") == 0)
    result = PyCObject_FromVoidPtr(pyimap->imap, NULL);
  else
    result = Py_FindMethod(imap_methods, self, attrname);

  return result;
}


// Reset the imap
static PyObject *pyimap_reset(PyObject *self, PyObject *args)
{
  pyimap_t *pyimap;
  
  pyimap = (pyimap_t*) self;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  imap_reset(pyimap->imap);

  Py_INCREF(Py_None);
  return Py_None;
}


// Translate the imap
static PyObject *pyimap_translate(PyObject *self, PyObject *args)
{
  pyimap_t *pyimap;
  int di, dj;
  
  pyimap = (pyimap_t*) self;

  if (!PyArg_ParseTuple(args, "ii", &di, &dj))
    return NULL;

  imap_translate(pyimap->imap, di, dj);

  Py_INCREF(Py_None);
  return Py_None;
}


// Test the fit between range data and the imap
static PyObject *pyimap_fit_ranges(PyObject *self, PyObject *args)
{
  pyimap_t *pyimap;
  int i;
  double ox, oy, oa;
  PyObject *pyscan;
  int range_count;
  double ranges[401][2];
  double err;
  
  pyimap = (pyimap_t*) self;

  if (!PyArg_ParseTuple(args, "(ddd)O", &ox, &oy, &oa, &pyscan))
    return NULL;

  range_count = PyList_Size(pyscan);
  for (i = 0; i < range_count; i++)
  {
    assert(i < sizeof(ranges) / sizeof(ranges[0]));
    ranges[i][0] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 0));
    ranges[i][1] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 1));
  }

  err = imap_fit_ranges(pyimap->imap, &ox, &oy, &oa, range_count, ranges);

  return Py_BuildValue("(ddd)d", ox, oy, oa, err);
}


// Add range data to the imap
static PyObject *pyimap_add_ranges(PyObject *self, PyObject *args)
{
  pyimap_t *pyimap;
  int i;
  double ox, oy, oa;
  int range_count;
  double ranges[401][2];
  PyObject *pyscan;
  
  pyimap = (pyimap_t*) self;

  if (!PyArg_ParseTuple(args, "(ddd)O", &ox, &oy, &oa, &pyscan))
    return NULL;

  range_count = PyList_Size(pyscan);
  for (i = 0; i < range_count; i++)
  {
    ranges[i][0] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 0));
    ranges[i][1] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 1));
  }

  imap_add_ranges(pyimap->imap, ox, oy, oa, range_count, ranges);

  Py_INCREF(Py_None);
  return Py_None;
}


// Draw the imap
static PyObject *pyimap_draw(PyObject *self, PyObject *args)
{
  pyimap_t *pyimap;
  PyObject *pyfig;
  rtk_fig_t *fig;
  
  pyimap = (pyimap_t*) self;

  if (!PyArg_ParseTuple(args, "O", &pyfig))
    return NULL;
  
  pyfig = PyObject_GetAttrString(pyfig, "cptr");
  if (!pyfig)
    return NULL;
  if (!PyCObject_Check(pyfig))
    return NULL;
  fig = (rtk_fig_t*) PyCObject_AsVoidPtr(pyfig);

  imap_draw(pyimap->imap, fig);

  Py_INCREF(Py_None);
  return Py_None;
}


// Assemble python imap type
static PyTypeObject imap_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "imap",
  sizeof(pyimap_t),
  0,
  pyimap_free, /*tp_dealloc*/
  0,              /*tp_print*/
  pyimap_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// List methods
static PyMethodDef imap_methods[] =
{
  {"reset", pyimap_reset, METH_VARARGS},
  {"translate", pyimap_translate, METH_VARARGS},
  {"fit_ranges", pyimap_fit_ranges, METH_VARARGS},
  {"add_ranges", pyimap_add_ranges, METH_VARARGS},
  {"draw", pyimap_draw, METH_VARARGS},
  {NULL, NULL}
};


// Module method table
static struct PyMethodDef module_methods[] =
{
  {"imap", pyimap_alloc, METH_VARARGS},
  {NULL, NULL}
};


// Module initialisation
void initimap(void)
{
  // Finish initialise of type objects here
  imap_type.ob_type = &PyType_Type;

  // Register our new type
  Py_InitModule("imap", module_methods);

  return;
}


