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
 * Desc: Python bindings for the relaxation engine
 * Author: Andrew Howard
 * Date: 5 Nov 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <Python.h>

#include "relax.h"


// Forward declarations
struct _pyrelax_t;

// Python wrapper for node type
typedef struct
{
  PyObject_HEAD
  relax_node_t *node;
  struct _pyrelax_t *pyrelax;
} pyrelax_node_t;


staticforward PyTypeObject pyrelax_node_type;
staticforward PyMethodDef pyrelax_node_methods[];

// Python wrapper for link type
typedef struct
{
  PyObject_HEAD
  relax_link_t *link;
  struct _pyrelax_t *pyrelax;
  pyrelax_node_t *pynode_a, *pynode_b;
} pyrelax_link_t;

staticforward PyTypeObject pyrelax_link_type;
staticforward PyMethodDef pyrelax_link_methods[];


// Python wrapper for relax type
typedef struct _pyrelax_t
{
  PyObject_HEAD
  relax_t *relax;
} pyrelax_t;

staticforward PyTypeObject pyrelax_type;
staticforward PyMethodDef pyrelax_methods[];


/**************************************************************************
 * Node
 *************************************************************************/


// Create a node
static PyObject *pyrelax_node_alloc(pyrelax_node_t *self, PyObject *args)
{
  pyrelax_t *pyrelax;

  if (!PyArg_ParseTuple(args, "O!", &pyrelax_type, &pyrelax))
    return NULL;
  
  self = PyObject_New(pyrelax_node_t, &pyrelax_node_type);
  self->pyrelax = pyrelax;
  Py_INCREF(self->pyrelax);

  self->node = relax_node_alloc(self->pyrelax->relax);

  //printf("alloc node %p\n", self);

  return (PyObject*) self;
}


// Destroy the node
static void pyrelax_node_free(pyrelax_node_t *self)
{
  //printf("free node  %p\n", self);
    
  relax_node_free(self->pyrelax->relax, self->node);

  Py_DECREF(self->pyrelax);
  PyObject_Del(self);

  return;
}


// Get node attributes
static PyObject *pyrelax_node_getattr(pyrelax_node_t *self, char *attrname)
{
  PyObject *result;

  if (strcmp(attrname, "pose") == 0)
  {
    result = Py_BuildValue("(ddd)",
                           self->node->pose.v[0],
                           self->node->pose.v[1],
                           self->node->pose.v[2]);
  }
  else
    result = NULL;

  return result;
}


// Set node attributes
static int pyrelax_node_setattr(pyrelax_node_t *self, char *attrname, PyObject *value)
{
  int result;
    
  if (strcmp(attrname, "pose") == 0)
  {
    result = !PyArg_ParseTuple(value, "ddd",
                               self->node->pose.v + 0,
                               self->node->pose.v + 1,
                               self->node->pose.v + 2);
  }
  else if (strcmp(attrname, "free") == 0)
  {
    self->node->free = PyInt_AsLong(value);
    result = 0;
  }
  else
    result = -1;

  return result;
}


// Assemble python node type
static PyTypeObject pyrelax_node_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "node",
  sizeof(pyrelax_node_t),
  0,
  (void*) pyrelax_node_free, /*tp_dealloc*/
  0,              /*tp_print*/
  (void*) pyrelax_node_getattr, /*tp_getattr*/
  (void*) pyrelax_node_setattr, /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


/**************************************************************************
 * Link
 *************************************************************************/

// Create a link
static PyObject *pyrelax_link_alloc(pyrelax_link_t *self, PyObject *args)
{
  pyrelax_t *pyrelax;
  pyrelax_node_t *pynode_a, *pynode_b;
  
  if (!PyArg_ParseTuple(args, "O!OO", &pyrelax_type, &pyrelax, &pynode_a, &pynode_b))
    return NULL;

  self = PyObject_New(pyrelax_link_t, &pyrelax_link_type);
  self->pyrelax = pyrelax;
  self->pynode_a = pynode_a;
  self->pynode_b = pynode_b;
  Py_INCREF(pyrelax);
  Py_INCREF(pynode_a);
  Py_INCREF(pynode_b);

  self->link = relax_link_alloc(pyrelax->relax, pynode_a->node, pynode_b->node);

  //printf("alloc link %p\n", self);
    
  return (PyObject*) self;
}


// Destroy the link
static void pyrelax_link_free(pyrelax_link_t *self)
{
  //printf("free link  %p\n", self);
    
  relax_link_free(self->pyrelax->relax, self->link);

  Py_DECREF(self->pynode_a);
  Py_DECREF(self->pynode_b);
  Py_DECREF(self->pyrelax);
  
  PyObject_Del(self);

  return;
}


// Get link attributes
static PyObject *pyrelax_link_getattr(pyrelax_link_t *self, char *attrname)
{
  PyObject *result;

  if (strcmp(attrname, "node_a") == 0)
  {
    Py_INCREF(self->pynode_a);
    result = (PyObject*) self->pynode_a;
  }
  else if (strcmp(attrname, "node_b") == 0)
  {
    Py_INCREF(self->pynode_b);
    result = (PyObject*) self->pynode_b;
  }
  else
    result = Py_FindMethod(pyrelax_link_methods, (PyObject*) self, attrname);

  return result;
}


// Set link attributes
static int pyrelax_link_setattr(pyrelax_link_t *self, char *attrname, PyObject *value)
{
  int result;

  if (strcmp(attrname, "type") == 0)
  {
    result = !PyArg_ParseTuple(value, "i", &self->link->type);
  }
  else if (strcmp(attrname, "w") == 0)
  {
    result = !PyArg_ParseTuple(value, "d", &self->link->w);
  }
  else if (strcmp(attrname, "outlier") == 0)
  {
    result = !PyArg_ParseTuple(value, "d", &self->link->outlier);
  }
  else if (strcmp(attrname, "pa") == 0)
  {
    result = !PyArg_ParseTuple(value, "dd", &self->link->pa.x, &self->link->pa.y);
  }
  else if (strcmp(attrname, "pb") == 0)
  {
    result = !PyArg_ParseTuple(value, "dd", &self->link->pb.x, &self->link->pb.y);
  }
  else if (strcmp(attrname, "la") == 0)
  {
    result = !PyArg_ParseTuple(value, "(dd)(dd)",
                               &self->link->la.pa.x, &self->link->la.pa.y,
                               &self->link->la.pb.x, &self->link->la.pb.y);
  }
  else if (strcmp(attrname, "lb") == 0)
  {
    result = !PyArg_ParseTuple(value, "(dd)(dd)",
                               &self->link->lb.pa.x, &self->link->lb.pa.y,
                               &self->link->lb.pb.x, &self->link->lb.pb.y);
  }
  else
    result = -1;

  return result;
}


// Assemble python link type
static PyTypeObject pyrelax_link_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "link",
  sizeof(pyrelax_link_t),
  0,
  (void*) pyrelax_link_free, /*tp_dealloc*/
  0,              /*tp_print*/
  (void*) pyrelax_link_getattr, /*tp_getattr*/
  (void*) pyrelax_link_setattr, /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// Link methods
static PyMethodDef pyrelax_link_methods[] =
{
  {NULL, NULL}
};


/**************************************************************************
 * Relax
 *************************************************************************/

// Create a relax
static PyObject *pyrelax_alloc(pyrelax_t *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  self = PyObject_New(pyrelax_t, &pyrelax_type);
  self->relax = relax_alloc();

  //printf("alloc graph %p\n", self);
  
  return (PyObject*) self;
}


// Destroy the relax
static void pyrelax_free(pyrelax_t *self)
{
  //printf("free graph  %p\n", self);
    
  relax_free(self->relax);
  PyObject_Del(self);

  return;
}


// Get relax attributes
static PyObject *pyrelax_getattr(pyrelax_t *self, char *attrname)
{
  PyObject *result;

  result = Py_FindMethod(pyrelax_methods, (PyObject*) self, attrname);

  return result;
}


// Iterate the relaxation process.
static PyObject *pyrelax_relax_ls(pyrelax_t *self, PyObject *args)
{
  int steps;
  double epsabs, epsrel;
  double err;
  
  if (!PyArg_ParseTuple(args, "idd", &steps, &epsabs, &epsrel))
    return NULL;

  Py_BEGIN_ALLOW_THREADS
  err = relax_relax_ls(self->relax, steps, epsabs, epsrel);
  Py_END_ALLOW_THREADS
  
  return Py_BuildValue("d", err);
}


// Iterate the relaxation process.
static PyObject *pyrelax_relax_nl(pyrelax_t *self, PyObject *args)
{
  int steps;
  double step, tol, epsabs, epsrel;
  double err, stats[3];
  
  if (!PyArg_ParseTuple(args, "idddd", &steps, &epsabs, &epsrel, &step, &tol))
    return NULL;

  Py_BEGIN_ALLOW_THREADS
  err = relax_relax_nl(self->relax, &steps, epsabs, epsrel, step, tol, stats);
  Py_END_ALLOW_THREADS
  
  return Py_BuildValue("di(ddd)", err, steps, stats[0], stats[1], stats[2]);
}


// Assemble python relax type
static PyTypeObject pyrelax_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "relax",
  sizeof(pyrelax_t),
  0,
  (void*) pyrelax_free, /*tp_dealloc*/
  0,              /*tp_print*/
  (void*) pyrelax_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


// List methods
static PyMethodDef pyrelax_methods[] =
{
  {"relax_ls", (PyCFunction) pyrelax_relax_ls, METH_VARARGS},
  {"relax_nl", (PyCFunction) pyrelax_relax_nl, METH_VARARGS},
  {NULL, NULL}
};


// Module method table
static struct PyMethodDef module_methods[] =
{
  {"Relax", (PyCFunction) pyrelax_alloc, METH_VARARGS},
  {"Node", (PyCFunction) pyrelax_node_alloc, METH_VARARGS},
  {"Link", (PyCFunction) pyrelax_link_alloc, METH_VARARGS},
  {NULL, NULL}
};


// Module initialisation
void initrelax(void)
{
  // Finish initialise of type objects here
  pyrelax_node_type.ob_type = &PyType_Type;
  pyrelax_link_type.ob_type = &PyType_Type;
  pyrelax_type.ob_type = &PyType_Type;

  // Register our new type
  Py_InitModule("relax", module_methods);

  return;
}


