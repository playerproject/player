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
 * Date: 9 Jul 2003
 * Desc: Python bindings for position3d device
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


/* Python wrapper for position3d type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_position3d_t *position3d;
  PyObject *px, *py, *pz;
  PyObject *vx, *vy, *vz;
} position3d_object_t;


PyTypeObject position3d_type;
staticforward PyMethodDef position3d_methods[];

/* Local declarations */
static void position3d_onread(position3d_object_t *pyposition3d);


/* Initialise (type function) */
PyObject *position3d_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  position3d_object_t *pyposition3d;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  pyposition3d = PyObject_New(position3d_object_t, &position3d_type);
  pyposition3d->client = pyclient->client;
  pyposition3d->position3d = playerc_position3d_create(pyclient->client, index);
  pyposition3d->position3d->info.user_data = pyposition3d;

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) pyposition3d->position3d,
                             (playerc_callback_fn_t) position3d_onread,
                             (void*) pyposition3d);
    
  return (PyObject*) pyposition3d;
}


/* Finailize (type function) */
static void position3d_del(PyObject *self)
{
  position3d_object_t *pyposition3d;
  pyposition3d = (position3d_object_t*) self;

  playerc_client_delcallback(pyposition3d->client, (playerc_device_t*) pyposition3d->position3d,
                             (playerc_callback_fn_t) position3d_onread,
                             (void*) pyposition3d);    

  playerc_position3d_destroy(pyposition3d->position3d);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *position3d_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  position3d_object_t *pyposition3d;

  pyposition3d = (position3d_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(pyposition3d->position3d->info.datatime);
  }
  
  else if (strcmp(attrname, "pos_x") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->pos_x);
  else if (strcmp(attrname, "pos_y") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->pos_y);
  else if (strcmp(attrname, "pos_z") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->pos_z);

  else if (strcmp(attrname, "pos_roll") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->pos_roll);
  else if (strcmp(attrname, "pos_pitch") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->pos_pitch);
  else if (strcmp(attrname, "pos_yaw") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->pos_yaw);

  else if (strcmp(attrname, "vel_x") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->vel_x);
  else if (strcmp(attrname, "vel_y") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->vel_y);
  else if (strcmp(attrname, "vel_z") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->vel_z);

  else if (strcmp(attrname, "vel_roll") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->vel_roll);
  else if (strcmp(attrname, "vel_pitch") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->vel_pitch);
  else if (strcmp(attrname, "vel_yaw") == 0)
    result = PyFloat_FromDouble(pyposition3d->position3d->vel_yaw);

  else if (strcmp(attrname, "stall") == 0)
    result = PyInt_FromLong(pyposition3d->position3d->stall);

  else
    result = Py_FindMethod(position3d_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *position3d_str(PyObject *self)
{
  char str[2048];
  position3d_object_t *pyposition3d;
  pyposition3d = (position3d_object_t*) self;

  snprintf(str, sizeof(str),
           "position3d %02d %013.3f"
           " %+09.3f %+09.3f %+09.3f"
           " %+09.4f %+09.4f %+09.4f"
           " %+09.3f %+09.3f %+09.3f"
           " %+09.4f %+09.4f %+09.4f"
           " %d",
           
           pyposition3d->position3d->info.index,
           pyposition3d->position3d->info.datatime,
           
           pyposition3d->position3d->pos_x,
           pyposition3d->position3d->pos_y,
           pyposition3d->position3d->pos_z,
           pyposition3d->position3d->pos_roll,
           pyposition3d->position3d->pos_pitch,
           pyposition3d->position3d->pos_yaw,

           pyposition3d->position3d->vel_x,
           pyposition3d->position3d->vel_y,
           pyposition3d->position3d->vel_z,
           pyposition3d->position3d->vel_roll,
           pyposition3d->position3d->vel_pitch,
           pyposition3d->position3d->vel_yaw,

           pyposition3d->position3d->stall);
  
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void position3d_onread(position3d_object_t *pyposition3d)
{
  //thread_acquire();

  // Do nothing
    
  //thread_release();

  return;
}


/* Subscribe to the device. */
static PyObject *position3d_subscribe(PyObject *self, PyObject *args)
{
  char access;
  position3d_object_t *pyposition3d;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pyposition3d = (position3d_object_t*) self;

  thread_release();
  result = playerc_position3d_subscribe(pyposition3d->position3d, access);
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
static PyObject *position3d_unsubscribe(PyObject *self, PyObject *args)
{
  position3d_object_t *pyposition3d;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pyposition3d = (position3d_object_t*) self;

  thread_release();
  result = playerc_position3d_unsubscribe(pyposition3d->position3d);
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
static PyObject *position3d_enable(PyObject *self, PyObject *args)
{
  int enable;
  position3d_object_t *pyposition3d;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "i", &enable))
    return NULL;
  pyposition3d = (position3d_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_position3d_enable(pyposition3d->position3d, enable));
  thread_acquire();
  
  return result;
}


/* Set the target speed
   (vx, vy, va)
*/
static PyObject *position3d_set_speed(PyObject *self, PyObject *args)
{
  double vx, vy, va;
  position3d_object_t *pyposition3d;
    
  if (!PyArg_ParseTuple(args, "ddd", &vx, &vy, &va))
    return NULL;
  pyposition3d = (position3d_object_t*) self;

  return PyInt_FromLong(playerc_position3d_set_speed(pyposition3d->position3d, vx, vy, va, 1));
}


/* Set the target pose (for drivers with position3d control)
   (vx, vy, va)
*/
static PyObject *position3d_set_cmd_pose(PyObject *self, PyObject *args)
{
  double px, py, pa;
  position3d_object_t *pyposition3d;

  pyposition3d = (position3d_object_t*) self;
  if (!PyArg_ParseTuple(args, "ddd", &px, &py, &pa))
    return NULL;

  return PyInt_FromLong(playerc_position3d_set_cmd_pose(pyposition3d->position3d, px, py, pa));
}


/* Assemble python position3d type
 */
PyTypeObject position3d_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "position3d",
  sizeof(position3d_object_t),
  0,
  position3d_del, /*tp_dealloc*/
  0,          /*tp_print*/
  position3d_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  position3d_str, /*tp_string*/
};


static PyMethodDef position3d_methods[] =
{
  {"subscribe", position3d_subscribe, METH_VARARGS},
  {"unsubscribe", position3d_unsubscribe, METH_VARARGS},  
  {"enable", position3d_enable, METH_VARARGS},
  {"set_speed", position3d_set_speed, METH_VARARGS},
  {"set_cmd_pose", position3d_set_cmd_pose, METH_VARARGS},
  {NULL, NULL}
};

