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
 * Date: 14 Sep 2003
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <Python.h>
#include <float.h>

#include "pyscan.h"


/**************************************************************************
 * Scan group object
 *************************************************************************/

// Create a scan
PyObject *pyscan_group_alloc(pyscan_group_t *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  self = PyObject_New(pyscan_group_t, &pyscan_group_type);
  self->ob = scan_group_alloc();
  
  return (PyObject*) self;
}


// Destroy the scan
static void pyscan_group_free(pyscan_group_t *self)
{
  scan_group_free(self->ob);
  PyObject_Del(self);

  return;
}


// Get scan attributes
static PyObject *pyscan_group_getattr(pyscan_group_t *self, char *attrname)
{
  PyObject *result;

  result = Py_FindMethod(pyscan_group_methods, (PyObject*) self, attrname);

  return result;
}


// Reset scan group
static PyObject *pyscan_group_reset(pyscan_group_t *self, PyObject *args)
{
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  scan_group_reset(self->ob);
  
  Py_INCREF(Py_None);
  return Py_None;
}


// Add scan group
static PyObject *pyscan_group_add(pyscan_group_t *self, PyObject *args)
{
  vector_t pose;
  pyscan_t *pyscan;
  
  if (!PyArg_ParseTuple(args, "(ddd)O!",
                        pose.v + 0, pose.v + 1, pose.v + 2,
                        &pyscan_type, &pyscan))
    return NULL;

  scan_group_add(self->ob, pose, pyscan->scan);
  
  Py_INCREF(Py_None);
  return Py_None;
}


// Get the free space polygon
static PyObject *pyscan_group_get_free(pyscan_group_t *self, PyObject *args)
{
  int i, j;
  scan_point_t *point;
  scan_contour_t *contour;
  PyObject *pysolid;
  PyObject *pycontour;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pysolid = PyList_New(self->ob->free->contour_count);
  for (i = 0; i < self->ob->free->contour_count; i++)
  {
    contour = self->ob->free->contours[i];
    pycontour = PyList_New(contour->point_count);
    PyList_SET_ITEM(pysolid, i, pycontour);

    for (j = 0; j < contour->point_count; j++)
    {
      point = contour->points + j;
      PyList_SET_ITEM(pycontour, j, Py_BuildValue("(dd)", point->x, point->y));
    }
  }
    
  return pysolid;
}


// Get the hit list
static PyObject *pyscan_group_get_hits(pyscan_group_t *self, PyObject *args)
{
  int i;
  scan_point_t *point;
  PyObject *pypoints;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pypoints = PyList_New(self->ob->hits->point_count);
  for (i = 0; i < self->ob->hits->point_count; i++)
  {
    point = self->ob->hits->points + i;
    PyList_SET_ITEM(pypoints, i, Py_BuildValue("(dd)d", point->x, point->y, point->w));
  }
    
  return pypoints;
}


// Assemble python scan type
PyTypeObject pyscan_group_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "scan_group",
  sizeof(pyscan_group_t),
  0,
  (void*) pyscan_group_free, /*tp_dealloc*/
  0,              /*tp_print*/
  (void*) pyscan_group_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// List methods
PyMethodDef pyscan_group_methods[] =
{
  {"reset", (PyCFunction) pyscan_group_reset, METH_VARARGS},
  {"add_scan", (PyCFunction) pyscan_group_add, METH_VARARGS},
  {"get_free", (PyCFunction) pyscan_group_get_free, METH_VARARGS},
  {"get_hits", (PyCFunction) pyscan_group_get_hits, METH_VARARGS},
  {NULL, NULL}
};

