/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *                      
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
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for position device
 *
 * CVS info:
 * $Source$
 * $Author$
 * $Revision$
 *
 **************************************************************************/

#include <pthread.h>
#include "Python.h"
#include "playerc.h"
#include "pyplayerc.h"


/* Python wrapper for position type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_position_t *position;
  PyObject *px, *py, *pa;
  PyObject *vx, *vy, *va;
} position_object_t;


PyTypeObject position_type;
staticforward PyMethodDef position_methods[];

/* Local declarations */
static void position_onread(position_object_t *pyposition);


/* Initialise (type function) */
PyObject *position_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  position_object_t *pyposition;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  pyposition = PyObject_New(position_object_t, &position_type);
  pyposition->client = pyclient->client;
  pyposition->position = playerc_position_create(pyclient->client, index);
  pyposition->position->info.user_data = pyposition;
  pyposition->px = PyFloat_FromDouble(0);
  pyposition->py = PyFloat_FromDouble(0);
  pyposition->pa = PyFloat_FromDouble(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) pyposition->position,
                             (playerc_callback_fn_t) position_onread,
                             (void*) pyposition);
    
  return (PyObject*) pyposition;
}


/* Finailize (type function) */
static void position_del(PyObject *self)
{
  position_object_t *pyposition;
  pyposition = (position_object_t*) self;

  playerc_client_delcallback(pyposition->client, (playerc_device_t*) pyposition->position,
                             (playerc_callback_fn_t) position_onread,
                             (void*) pyposition);    

  Py_DECREF(pyposition->px);
  Py_DECREF(pyposition->py);
  Py_DECREF(pyposition->pa);
  playerc_position_destroy(pyposition->position);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *position_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  position_object_t *pyposition;

  pyposition = (position_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(pyposition->position->info.datatime);
  }
  else if (strcmp(attrname, "px") == 0)
  {
    Py_INCREF(pyposition->px);
    result = pyposition->px;
  }
  else if (strcmp(attrname, "py") == 0)
  {
    Py_INCREF(pyposition->py);
    result = pyposition->py;
  }
  else if (strcmp(attrname, "pa") == 0)
  {
    Py_INCREF(pyposition->pa);
    result = pyposition->pa;
  }
  else if (strcmp(attrname, "vx") == 0)
  {
    Py_INCREF(pyposition->vx);
    result = pyposition->vx;
  }
  else if (strcmp(attrname, "vy") == 0)
  {
    Py_INCREF(pyposition->vy);
    result = pyposition->vy;
  }
  else if (strcmp(attrname, "va") == 0)
  {
    Py_INCREF(pyposition->va);
    result = pyposition->va;
  }
  else if (strcmp(attrname, "stall") == 0)
  {
    result = PyInt_FromLong(pyposition->position->stall);
  }
  else
    result = Py_FindMethod(position_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *position_str(PyObject *self)
{
  char str[128];
  position_object_t *pyposition;
  pyposition = (position_object_t*) self;

  snprintf(str, sizeof(str),
           "position %02d %013.3f"
           " %+07.3f %+07.3f %+04.3f"
           " %+04.3f %+04.3f %+04.3f",
           pyposition->position->info.index,
           pyposition->position->info.datatime,
           pyposition->position->px,
           pyposition->position->py,
           pyposition->position->pa,
           pyposition->position->vx,
           pyposition->position->vy,
           pyposition->position->va);
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void position_onread(position_object_t *pyposition)
{
  thread_acquire();
    
  Py_DECREF(pyposition->px);
  Py_DECREF(pyposition->py);
  Py_DECREF(pyposition->pa);
  pyposition->px = PyFloat_FromDouble(pyposition->position->px);
  pyposition->py = PyFloat_FromDouble(pyposition->position->py);
  pyposition->pa = PyFloat_FromDouble(pyposition->position->pa);    
  pyposition->vx = PyFloat_FromDouble(pyposition->position->vx);
  pyposition->vy = PyFloat_FromDouble(pyposition->position->vy);
  pyposition->va = PyFloat_FromDouble(pyposition->position->va);    
    
  thread_release();
}


/* Subscribe to the device. */
static PyObject *position_subscribe(PyObject *self, PyObject *args)
{
  char access;
  position_object_t *pyposition;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pyposition = (position_object_t*) self;

  thread_release();
  result = playerc_position_subscribe(pyposition->position, access);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Unsubscribe from the device. */
static PyObject *position_unsubscribe(PyObject *self, PyObject *args)
{
  position_object_t *pyposition;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pyposition = (position_object_t*) self;

  thread_release();
  result = playerc_position_unsubscribe(pyposition->position);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Enable/disable motors
   Args: (enable)
   Returns: (0 on success)
*/
static PyObject *position_enable(PyObject *self, PyObject *args)
{
  int enable;
  position_object_t *pyposition;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "i", &enable))
    return NULL;
  pyposition = (position_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_position_enable(pyposition->position, enable));
  thread_acquire();
  
  return result;
}


/* Set the target speed
   (vx, vy, va)
*/
static PyObject *position_set_speed(PyObject *self, PyObject *args)
{
  double vx, vy, va;
  position_object_t *pyposition;
    
  if (!PyArg_ParseTuple(args, "ddd", &vx, &vy, &va))
    return NULL;
  pyposition = (position_object_t*) self;

  return PyInt_FromLong(playerc_position_set_speed(pyposition->position, vx, vy, va));
}


/* Set the target pose (for drivers with position control)
   (vx, vy, va)
*/
static PyObject *position_set_cmd_pose(PyObject *self, PyObject *args)
{
  double px, py, pa;
  position_object_t *pyposition;

  pyposition = (position_object_t*) self;
  if (!PyArg_ParseTuple(args, "ddd", &px, &py, &pa))
    return NULL;

  return PyInt_FromLong(playerc_position_set_cmd_pose(pyposition->position, px, py, pa));
}


/* Assemble python position type
 */
PyTypeObject position_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "position",
  sizeof(position_object_t),
  0,
  position_del, /*tp_dealloc*/
  0,          /*tp_print*/
  position_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  position_str, /*tp_string*/
};


static PyMethodDef position_methods[] =
{
  {"subscribe", position_subscribe, METH_VARARGS},
  {"unsubscribe", position_unsubscribe, METH_VARARGS},  
  {"enable", position_enable, METH_VARARGS},
  {"set_speed", position_set_speed, METH_VARARGS},
  {"set_cmd_pose", position_set_cmd_pose, METH_VARARGS},
  {NULL, NULL}
};

