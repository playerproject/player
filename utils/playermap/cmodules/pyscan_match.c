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


// Create a scan_match
PyObject *pyscan_match_alloc(pyscan_match_t *self, PyObject *args)
{
  pyscan_group_t *pyscan_a;
  pyscan_group_t *pyscan_b;
  
  if (!PyArg_ParseTuple(args, "O!O!", &pyscan_group_type, &pyscan_a,
                        &pyscan_group_type, &pyscan_b))
    return NULL;

  self = PyObject_New(pyscan_match_t, &pyscan_match_type);

  self->pyscan_a = pyscan_a;
  self->pyscan_b = pyscan_b;
  Py_INCREF(self->pyscan_a);
  Py_INCREF(self->pyscan_b);

  self->scan_match = scan_match_alloc(pyscan_a->ob, pyscan_b->ob);

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
    pypair = Py_BuildValue("i(ii)d(dd)(dd)((dd)(dd))((dd)(dd))",
                           pair->type, pair->ia, pair->ib, pair->w, 
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
  "scan_match",
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
