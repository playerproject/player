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

#include "pyscan.h"


/**************************************************************************
 * Scan object
 *************************************************************************/

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
  int result;
  
  if (!PyArg_ParseTuple(args, "(ddd)O",
                        origin.v + 0, origin.v + 1, origin.v + 2, &pyranges))
    return NULL;

  range_count = PyList_Size(pyranges);
  assert(range_count < 401);
  for (i = 0; i < range_count; i++)
  {
    ranges[i][0] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyranges, i), 0));
    ranges[i][1] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyranges, i), 1));
  }

  result = scan_add_ranges(self->scan, origin, range_count, ranges);

  return Py_BuildValue("i", result);
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
    PyList_SET_ITEM(pycontour, i, Py_BuildValue("(dd)", point->x, point->y));
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

  pypoints = PyList_New(self->scan->hits->point_count);
  for (i = 0; i < self->scan->hits->point_count; i++)
  {
    point = self->scan->hits->points + i;
    PyList_SET_ITEM(pypoints, i, Py_BuildValue("(dd)d", point->x, point->y, point->w));
  }
    
  return pypoints;
}


// Get the hit polylines (for faster drawing)
static PyObject *pyscan_get_hit_lines(pyscan_t *self, PyObject *args)
{
  int i;
  double dx, dy;
  scan_point_t *point, *npoint;
  PyObject *pylists, *pypoints, *pypoint;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pylists = PyList_New(0);

  for (i = 0; i < self->scan->hits->point_count; i++)
  {
    point = self->scan->hits->points + i;

    if (i == 0)
    {
      pypoints = PyList_New(0);
      PyList_Append(pylists, pypoints);
      Py_DECREF(pypoints);
    }
    else
    {
      npoint = self->scan->hits->points + i - 1;
      dx = point->x - npoint->x;
      dy = point->y - npoint->y;

      if (sqrt(dx * dx + dy * dy) > 2 * self->scan->hit_dist)
      {
        pypoints = PyList_New(0);
        PyList_Append(pylists, pypoints);
        Py_DECREF(pypoints);
      }
    }
    
    pypoint = Py_BuildValue("(dd)", point->x, point->y);
    PyList_Append(pypoints, pypoint);
    Py_DECREF(pypoint);
  }
    
  return pylists;
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
  double result;

  if (!PyArg_ParseTuple(args, "(dd)", &point.x, &point.y))
    return NULL;

  result = scan_test_free(self->scan, point);
  
  return Py_BuildValue("d", result);
}


// Test a line to see if it lies in free space
static PyObject *pyscan_test_free_line(pyscan_t *self, PyObject *args)
{
  scan_point_t pa;
  scan_point_t pb;
  int result;

  if (!PyArg_ParseTuple(args, "(dd)(dd)", &pa.x, &pa.y, &pb.x, &pb.y))
    return NULL;

  result = scan_test_free_line(self->scan, pa, pb);
  
  return Py_BuildValue("i", result);
}


// Assemble python scan type
PyTypeObject pyscan_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "scan",
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
  {"get_hit_lines", (PyCFunction) pyscan_get_hit_lines, METH_VARARGS},
  {"test_occ", (PyCFunction) pyscan_test_occ, METH_VARARGS},
  {"test_free", (PyCFunction) pyscan_test_free, METH_VARARGS},
  {"test_free_line", (PyCFunction) pyscan_test_free_line, METH_VARARGS},
  {NULL, NULL}
};




/**************************************************************************
 * Module stuff
 *************************************************************************/

// Module method table
struct PyMethodDef module_methods[] =
{
  {"scan", (PyCFunction) pyscan_alloc, METH_VARARGS},
  {"scan_group", (PyCFunction) pyscan_group_alloc, METH_VARARGS},
  {"scan_match", (PyCFunction) pyscan_match_alloc, METH_VARARGS},
  {NULL, NULL}
};


// Module initialisation
void initscan(void)
{
  // Finish initialise of type objects here
  pyscan_type.ob_type = &PyType_Type;
  pyscan_group_type.ob_type = &PyType_Type;  
  pyscan_match_type.ob_type = &PyType_Type;  

  // Register our new type
  Py_InitModule("scan", module_methods);

  return;
}


