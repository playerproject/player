/*
 * Map maker
 * Copyright (C) 2003 Andrew Howard 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/**************************************************************************
 * Desc: Python bindings for the scan library
 * Author: Andrew Howard
 * Date: 1 Apr 2003
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <Python.h>
#include <float.h>

#include "scan.h"


/**************************************************************************
 * Scan object
 *************************************************************************/


// Python wrapper for scan class
typedef struct
{
  PyObject_HEAD
  scan_t *scan;
} pyscan_t;

extern PyTypeObject pyscan_type;
extern PyMethodDef pyscan_methods[];


// Create a scan
static PyObject *pyscan_alloc(pyscan_t *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  self = PyObject_New(pyscan_t, &pyscan_type);
  self->scan = scan_alloc();
  
  return (PyObject*) self;
}


// Destroy the scan
static void pyscan_free(pyscan_t *self)
{
  scan_free(self->scan);
  PyObject_Del(self);

  return;
}


// Get scan attributes
static PyObject *pyscan_getattr(pyscan_t *self, char *attrname)
{
  PyObject *result;

  result = Py_FindMethod(pyscan_methods, (PyObject*) self, attrname);

  return result;
}


// Add range data to the scan
static PyObject *pyscan_add_ranges(pyscan_t *self, PyObject *args)
{
  int i;
  vector_t origin;
  int range_count;
  double ranges[401][2];
  PyObject *pyranges;
  
  if (!PyArg_ParseTuple(args, "(ddd)O",
                        origin.v + 0, origin.v + 1, origin.v + 2, &pyranges))
    return NULL;

  range_count = PyList_Size(pyranges);
  for (i = 0; i < range_count; i++)
  {
    ranges[i][0] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyranges, i), 0));
    ranges[i][1] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyranges, i), 1));
  }

  scan_add_ranges(self->scan, origin, range_count, ranges);

  Py_INCREF(Py_None);
  return Py_None;
}


// Get the raw range values
static PyObject *pyscan_get_raw(pyscan_t *self, PyObject *args)
{
  int i;
  scan_point_t *point;
  scan_contour_t *contour;
  PyObject *pycontour;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  contour = self->scan->raw;
    
  pycontour = PyList_New(contour->point_count);
  for (i = 0; i < contour->point_count; i++)
  {
    point = contour->points + i;
    PyList_SET_ITEM(pycontour, i, Py_BuildValue("(dddd)", point->r, point->b,
                                                point->x, point->y));
  }
    
  return pycontour;
}


// Get the free space polygon
static PyObject *pyscan_get_free(pyscan_t *self, PyObject *args)
{
  int i;
  scan_point_t *point;
  scan_contour_t *contour;
  PyObject *pycontour;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  contour = self->scan->free;
    
  pycontour = PyList_New(contour->point_count);
  for (i = 0; i < contour->point_count; i++)
  {
    point = contour->points + i;
    PyList_SET_ITEM(pycontour, i, Py_BuildValue("(dd)", point->x, point->y));
  }
    
  return pycontour;
}


// Get the hit list
static PyObject *pyscan_get_hits(pyscan_t *self, PyObject *args)
{
  int i;
  scan_point_t *point;
  PyObject *pypoints;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pypoints = PyList_New(self->scan->hit->point_count);
  for (i = 0; i < self->scan->hit->point_count; i++)
  {
    point = self->scan->hit->points + i;
    PyList_SET_ITEM(pypoints, i, Py_BuildValue("(dd)d", point->x, point->y, point->w));
  }
    
  return pypoints;
}


// Test a point to see if it lies in occupied space
static PyObject *pyscan_test_occ(pyscan_t *self, PyObject *args)
{
  scan_point_t point;
  double dist;
  int result;

  if (!PyArg_ParseTuple(args, "(dd)d", &point.x, &point.y, &dist))
    return NULL;

  result = scan_test_occ(self->scan, point, dist);
  
  return Py_BuildValue("i", result);
}


// Test a point to see if it lies in free space
static PyObject *pyscan_test_free(pyscan_t *self, PyObject *args)
{
  scan_point_t point;
  int result;

  if (!PyArg_ParseTuple(args, "(dd)", &point.x, &point.y))
    return NULL;

  result = scan_test_free(self->scan, point);
  
  return Py_BuildValue("i", result);
}


// Assemble python scan type
PyTypeObject pyscan_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "Scan",
  sizeof(pyscan_t),
  0,
  (void*) pyscan_free, /*tp_dealloc*/
  0,              /*tp_print*/
  (void*) pyscan_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// List methods
PyMethodDef pyscan_methods[] =
{
  {"add_ranges", (PyCFunction) pyscan_add_ranges, METH_VARARGS},
  {"get_raw", (PyCFunction) pyscan_get_raw, METH_VARARGS},
  {"get_free", (PyCFunction) pyscan_get_free, METH_VARARGS},
  {"get_hits", (PyCFunction) pyscan_get_hits, METH_VARARGS},
  {"test_occ", (PyCFunction) pyscan_test_occ, METH_VARARGS},
  {"test_free", (PyCFunction) pyscan_test_free, METH_VARARGS},
  {NULL, NULL}
};


/**************************************************************************
 * Scan match object
 *************************************************************************/

// Python wrapper for scan match class
typedef struct
{
  PyObject_HEAD
  pyscan_t *pyscan_a, *pyscan_b;
  scan_match_t *scan_match;
} pyscan_match_t;

extern PyTypeObject pyscan_match_type;
extern PyMethodDef pyscan_match_methods[];


// Create a scan_match
PyObject *pyscan_match_alloc(pyscan_match_t *self, PyObject *args)
{
  pyscan_t *pyscan_a, *pyscan_b;
  
  if (!PyArg_ParseTuple(args, "O!O!", &pyscan_type, &pyscan_a, &pyscan_type, &pyscan_b))
    return NULL;

  self = PyObject_New(pyscan_match_t, &pyscan_match_type);

  self->pyscan_a = pyscan_a;
  self->pyscan_b = pyscan_b;
  Py_INCREF(self->pyscan_a);
  Py_INCREF(self->pyscan_b);

  self->scan_match = scan_match_alloc(pyscan_a->scan, pyscan_b->scan);

  return (PyObject*) self;
}


// Destroy the scan_match
static void pyscan_match_free(pyscan_match_t *self)
{
  scan_match_free(self->scan_match);
  
  Py_DECREF(self->pyscan_a);
  Py_DECREF(self->pyscan_b);
  PyObject_Del(self);

  return;
}


// Get scan_match attributes
static PyObject *pyscan_match_getattr(pyscan_match_t *self, char *attrname)
{
  PyObject *result;

  result = Py_FindMethod(pyscan_match_methods, (PyObject*) self, attrname);

  return result;
}


// Find points pairs
static PyObject *pyscan_match_pairs(pyscan_match_t *self, PyObject *args)
{
  int i;
  double dist;
  vector_t pose_a, pose_b;
  scan_pair_t *pair;
  PyObject *pypair, *pypairs;
  
  if (!PyArg_ParseTuple(args, "(ddd)(ddd)d",
                        pose_a.v + 0, pose_a.v + 1, pose_a.v + 2,
                        pose_b.v + 0, pose_b.v + 1, pose_b.v + 2,
                        &dist))
    return NULL;

  self->scan_match->outlier_dist = dist;
  
  scan_match_pairs(self->scan_match, pose_a, pose_b);

  pypairs = PyList_New(self->scan_match->pair_count);
  
  for (i = 0; i < self->scan_match->pair_count; i++)
  {
    pair = self->scan_match->pairs + i;
    pypair = Py_BuildValue("id(dd)(dd)((dd)(dd))((dd)(dd))",
                           pair->type, pair->w,
                           pair->pa.x, pair->pa.y, pair->pb.x, pair->pb.y,
                           pair->la.pa.x, pair->la.pa.y, pair->la.pb.x, pair->la.pb.y,
                           pair->lb.pa.x, pair->lb.pa.y, pair->lb.pb.x, pair->lb.pb.y);
    PyList_SET_ITEM(pypairs, i, pypair);
  }
  return pypairs;    
}


// Assemble python scan_match type
PyTypeObject pyscan_match_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "ScanMatch",
  sizeof(pyscan_match_t),
  0,
  (void*) pyscan_match_free, /*tp_dealloc*/
  0,              /*tp_print*/
  (void*) pyscan_match_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// Methods
PyMethodDef pyscan_match_methods[] =
{
  {"pairs", (PyCFunction) pyscan_match_pairs, METH_VARARGS},
  {NULL, NULL}
};


/**************************************************************************
 * Module stuff
 *************************************************************************/

// Module method table
struct PyMethodDef module_methods[] =
{
  {"Scan", (PyCFunction) pyscan_alloc, METH_VARARGS},
  {"ScanMatch", (PyCFunction) pyscan_match_alloc, METH_VARARGS},
  {NULL, NULL}
};


// Module initialisation
void initscan(void)
{
  // Finish initialise of type objects here
  pyscan_type.ob_type = &PyType_Type;
  pyscan_match_type.ob_type = &PyType_Type;  

  // Register our new type
  Py_InitModule("scan", module_methods);

  return;
}


