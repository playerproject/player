/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2002-2003
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
 * Desc: Python bindings for ptz device
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


/* Python wrapper for ptz type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_ptz_t *ptz;
  PyObject *pan, *tilt, *zoom;
} ptz_object_t;


PyTypeObject ptz_type;
staticforward PyMethodDef ptz_methods[];

/* Local declarations */
static void ptz_onread(ptz_object_t *ptzob);


/* Initialise (type function) */
PyObject *ptz_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  ptz_object_t *ptzob;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  ptzob = PyObject_New(ptz_object_t, &ptz_type);
  ptzob->client = pyclient->client;
  ptzob->ptz = playerc_ptz_create(pyclient->client, index);
  ptzob->ptz->info.user_data = ptzob;
  ptzob->pan = PyFloat_FromDouble(0);
  ptzob->tilt = PyFloat_FromDouble(0);
  ptzob->zoom = PyFloat_FromDouble(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) ptzob->ptz,
                             (playerc_callback_fn_t) ptz_onread,
                             (void*) ptzob);
    
  return (PyObject*) ptzob;
}


/* Finailize (type function) */
static void ptz_del(PyObject *self)
{
  ptz_object_t *ptzob;
  ptzob = (ptz_object_t*) self;

  playerc_client_delcallback(ptzob->client, (playerc_device_t*) ptzob->ptz,
                             (playerc_callback_fn_t) ptz_onread,
                             (void*) ptzob);    

  Py_DECREF(ptzob->pan);
  Py_DECREF(ptzob->tilt);
  Py_DECREF(ptzob->zoom);
  playerc_ptz_destroy(ptzob->ptz);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *ptz_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  ptz_object_t *ptzob;

  ptzob = (ptz_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(ptzob->ptz->info.datatime);
  }
  else if (strcmp(attrname, "pan") == 0)
  {
    Py_INCREF(ptzob->pan);
    result = ptzob->pan;
  }
  else if (strcmp(attrname, "tilt") == 0)
  {
    Py_INCREF(ptzob->tilt);
    result = ptzob->tilt;
  }
  else if (strcmp(attrname, "zoom") == 0)
  {
    Py_INCREF(ptzob->zoom);
    result = ptzob->zoom;
  }
  else
    result = Py_FindMethod(ptz_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *ptz_str(PyObject *self)
{
  char str[128];
  ptz_object_t *ptzob;
  ptzob = (ptz_object_t*) self;

  snprintf(str, sizeof(str),
           "ptz %02d %013.3f"
           " %+07.3f %+07.3f %+07.3f",
           ptzob->ptz->info.index,
           ptzob->ptz->info.datatime,
           ptzob->ptz->pan,
           ptzob->ptz->tilt,
           ptzob->ptz->zoom);
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void ptz_onread(ptz_object_t *ptzob)
{
  thread_acquire();
    
  Py_DECREF(ptzob->pan);
  Py_DECREF(ptzob->tilt);
  Py_DECREF(ptzob->zoom);
  ptzob->pan = PyFloat_FromDouble(ptzob->ptz->pan);
  ptzob->tilt = PyFloat_FromDouble(ptzob->ptz->tilt);
  ptzob->zoom = PyFloat_FromDouble(ptzob->ptz->zoom);    
    
  thread_release();
}


/* Subscribe to the device. */
static PyObject *ptz_subscribe(PyObject *self, PyObject *args)
{
  char access;
  ptz_object_t *ptzob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  ptzob = (ptz_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_ptz_subscribe(ptzob->ptz, access));
  thread_acquire();

  return result;
}


/* Unsubscribe from the device. */
static PyObject *ptz_unsubscribe(PyObject *self, PyObject *args)
{
  ptz_object_t *ptzob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  ptzob = (ptz_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_ptz_unsubscribe(ptzob->ptz));
  thread_acquire();

  return result;
}


// Set the commanded ptz direction.
static PyObject *ptz_set(PyObject *self, PyObject *args)
{
  double pan, tilt, zoom;
  ptz_object_t *ptzob;
    
  if (!PyArg_ParseTuple(args, "ddd", &pan, &tilt, &zoom))
    return NULL;
  ptzob = (ptz_object_t*) self;

  return PyInt_FromLong(playerc_ptz_set(ptzob->ptz, pan, tilt, zoom));
}


/* Assemble python ptz type
 */
PyTypeObject ptz_type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "ptz",
    sizeof(ptz_object_t),
    0,
    ptz_del, /*tp_dealloc*/
    0,          /*tp_print*/
    ptz_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,          /*tp_call*/
    ptz_str, /*tp_string*/
};


static PyMethodDef ptz_methods[] =
{
  {"subscribe", ptz_subscribe, METH_VARARGS},
  {"unsubscribe", ptz_unsubscribe, METH_VARARGS},
  {"set", ptz_set, METH_VARARGS},    
  {NULL, NULL}
};

