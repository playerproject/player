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
 * Desc: Python bindings for the occupancy grid library
 * Author: Andrew Howard
 * Date: 5 Nov 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <Python.h>

#include "grid.h"


// Python wrapper for grid type
typedef struct
{
  PyObject_HEAD
  grid_t *grid;
} pygrid_t;

staticforward PyTypeObject grid_type;
staticforward PyMethodDef grid_methods[];


// Create a grid
static PyObject *pygrid_alloc(PyObject *self, PyObject *args)
{
  pygrid_t *pygrid;
  double size_x, size_y;
  double scale;

  if (!PyArg_ParseTuple(args, "ddd", &size_x, &size_y, &scale))
    return NULL;

  pygrid = PyObject_New(pygrid_t, &grid_type);
  pygrid->grid = grid_alloc(size_x / scale, size_y / scale, scale);
  
  return (PyObject*) pygrid;
}


// Destroy the grid
static void pygrid_free(PyObject *self)
{
  pygrid_t *pygrid;

  pygrid = (pygrid_t*) self;
  grid_free(pygrid->grid);
  PyObject_Del(self);

  return;
}


// Get grid attributes
static PyObject *pygrid_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  pygrid_t *pygrid;

  pygrid = (pygrid_t*) self;

  if (strcmp(attrname, "cptr") == 0)
    result = PyCObject_FromVoidPtr(pygrid->grid, NULL);
  else
    result = Py_FindMethod(grid_methods, self, attrname);

  return result;
}


// Reset the grid
static PyObject *pygrid_reset(PyObject *self, PyObject *args)
{
  pygrid_t *pygrid;
  
  pygrid = (pygrid_t*) self;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  grid_reset(pygrid->grid);

  Py_INCREF(Py_None);
  return Py_None;
}


// Set the model parameters
static PyObject *pygrid_set_model(PyObject *self, PyObject *args)
{
  pygrid_t *pygrid;
  int occ_inc, emp_inc;
  int occ_thresh, emp_thresh;
  
  pygrid = (pygrid_t*) self;

  if (!PyArg_ParseTuple(args, "iiii", &occ_inc, &emp_inc, &occ_thresh, &emp_thresh))
    return NULL;

  pygrid->grid->model_occ_inc = occ_inc;
  pygrid->grid->model_emp_inc = emp_inc;
  pygrid->grid->model_occ_thresh = occ_thresh;
  pygrid->grid->model_emp_thresh = emp_thresh;

  Py_INCREF(Py_None);
  return Py_None;
}


// Save the occupancy map
static PyObject *pygrid_save_occ(PyObject *self, PyObject *args)
{
  char *filename;
  pygrid_t *pygrid;
  
  pygrid = (pygrid_t*) self;

  if (!PyArg_ParseTuple(args, "s", &filename))
    return NULL;

  if (grid_save_occ(pygrid->grid, filename) != 0)
    return NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


// Add range data to the grid (fast)
static PyObject *pygrid_add_ranges_fast(PyObject *self, PyObject *args)
{
  pygrid_t *pygrid;
  int i;
  double ox, oy, oa;
  int range_count;
  double ranges[401][2];
  PyObject *pyscan;
  
  pygrid = (pygrid_t*) self;

  if (!PyArg_ParseTuple(args, "(ddd)O", &ox, &oy, &oa, &pyscan))
    return NULL;

  range_count = PyList_Size(pyscan);
  for (i = 0; i < range_count; i++)
  {
    ranges[i][0] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 0));
    ranges[i][1] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 1));
  }

  grid_add_ranges_fast(pygrid->grid, ox, oy, oa, range_count, ranges);

  Py_INCREF(Py_None);
  return Py_None;
}


// Add range data to the grid (slow)
static PyObject *pygrid_add_ranges_slow(PyObject *self, PyObject *args)
{
  pygrid_t *pygrid;
  int i;
  double ox, oy, oa;
  int range_count;
  double ranges[401][2];
  PyObject *pyscan;
  
  pygrid = (pygrid_t*) self;

  if (!PyArg_ParseTuple(args, "(ddd)O", &ox, &oy, &oa, &pyscan))
    return NULL;

  range_count = PyList_Size(pyscan);
  for (i = 0; i < range_count; i++)
  {
    ranges[i][0] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 0));
    ranges[i][1] = PyFloat_AsDouble(PyTuple_GetItem(PyList_GetItem(pyscan, i), 1));
  }

  grid_add_ranges_slow(pygrid->grid, ox, oy, oa, range_count, ranges);

  Py_INCREF(Py_None);
  return Py_None;
}


// Get the list of occupied cells
static PyObject *pygrid_get_occ(pygrid_t *self, PyObject *args)
{
  int i, j;
  grid_cell_t *cell;
  PyObject *pypoint, *pypoints;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pypoints = PyList_New(0);

  for (j = 0; j < self->grid->size_y; j++)
  {
    for (i = 0; i < self->grid->size_x; i++)
    {
      cell = self->grid->cells + GRID_INDEX(self->grid, i, j);

      if (cell->occ_state == +1)
      {
        pypoint = Py_BuildValue("(dd)", GRID_WXGX(self->grid, i), GRID_WYGY(self->grid, j));
        PyList_Append(pypoints, pypoint);
        Py_DECREF(pypoint);
      }
    }
  }
    
  return pypoints;
}


// Test a point to see if it lies in free space
static PyObject *pygrid_test_free(pygrid_t *self, PyObject *args)
{
  double ox, oy;
  int result;

  if (!PyArg_ParseTuple(args, "(dd)", &ox, &oy))
    return NULL;

  result = grid_test_free(self->grid, ox, oy);
  
  return Py_BuildValue("i", result);
}


// Find the distane to the nearest occupied cell
static PyObject *pygrid_test_occ_dist(pygrid_t *self, PyObject *args)
{
  double ox, oy;
  double result;
  grid_cell_t *cell;

  if (!PyArg_ParseTuple(args, "(dd)", &ox, &oy))
    return NULL;

  cell = grid_get_cell(self->grid, ox, oy);
  if (cell == NULL)
    result = self->grid->max_dist;
  else
    result = cell->occ_dist;

  return Py_BuildValue("d", result);
}


// Assemble python grid type
static PyTypeObject grid_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "grid",
  sizeof(pygrid_t),
  0,
  pygrid_free, /*tp_dealloc*/
  0,              /*tp_print*/
  pygrid_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// List methods
static PyMethodDef grid_methods[] =
{
  {"reset", pygrid_reset, METH_VARARGS},
  {"set_model", pygrid_set_model, METH_VARARGS},  
  {"save_occ", pygrid_save_occ, METH_VARARGS},
  {"add_ranges_fast", pygrid_add_ranges_fast, METH_VARARGS},
  {"add_ranges_slow", pygrid_add_ranges_slow, METH_VARARGS},
  {"get_occ", (PyCFunction) pygrid_get_occ, METH_VARARGS},
  {"test_free", (PyCFunction) pygrid_test_free, METH_VARARGS},
  {"test_occ_dist", (PyCFunction) pygrid_test_occ_dist, METH_VARARGS},
  {NULL, NULL}
};


// Module method table
static struct PyMethodDef module_methods[] =
{
  {"grid", pygrid_alloc, METH_VARARGS},
  {NULL, NULL}
};


// Module initialisation
void initgrid(void)
{
  // Finish initialise of type objects here
  grid_type.ob_type = &PyType_Type;

  // Register our new type
  Py_InitModule("grid", module_methods);

  return;
}


