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
 * Author: Daniel Arbuckle
 *   modified from dev_laser.c by Andrew Howard
 * Date: 2 Dec 2003
 * Desc: Python bindings for sonar device
 *
 **************************************************************************/

#include <math.h>
#include <pthread.h>
#include "Python.h"
#include "playerc.h"
#include "pyplayerc.h"


/* Python wrapper for sonar type */
typedef struct
{
    PyObject_HEAD
    playerc_client_t *client;
    playerc_sonar_t *sonar;
    PyObject *scan;
} sonar_object_t;

/* Local declarations */
static void sonar_onread(sonar_object_t *sonarob);

PyTypeObject sonar_type;
staticforward PyMethodDef sonar_methods[];


PyObject *sonar_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  sonar_object_t *sonarob;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  sonarob = PyObject_New(sonar_object_t, &sonar_type);
  sonarob->client = pyclient->client;
  sonarob->sonar = playerc_sonar_create(pyclient->client, index);
  sonarob->sonar->info.user_data = sonarob;
  sonarob->scan =  PyList_New(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) sonarob->sonar,
                             (playerc_callback_fn_t) sonar_onread,
                             (void*) sonarob);
    
  return (PyObject*) sonarob;
}


static void sonar_del(PyObject *self)
{
  sonar_object_t *sonarob;
  sonarob = (sonar_object_t*) self;

  playerc_client_delcallback(sonarob->client, (playerc_device_t*) sonarob->sonar,
                             (playerc_callback_fn_t) sonar_onread,
                             (void*) sonarob);    
  Py_DECREF(sonarob->scan);
  playerc_sonar_destroy(sonarob->sonar);
  PyObject_Del(self);
}


static PyObject *sonar_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  sonar_object_t *sonarob;

  sonarob = (sonar_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(sonarob->sonar->info.datatime);
  }
  else if (strcmp(attrname, "scan") == 0)
  {
    Py_INCREF(sonarob->scan);
    result = sonarob->scan;
  }
  else
    result = Py_FindMethod(sonar_methods, self, attrname);

  return result;
}



/* Get string representation (type function) */
static PyObject *sonar_str(PyObject *self)
{
  int i;
  char s[64];
  char str[8192];
  sonar_object_t *sonarob;
  sonarob = (sonar_object_t*) self;

  snprintf(str, sizeof(str),
           "sonar %02d %013.3f ",
           sonarob->sonar->info.index,
           sonarob->sonar->info.datatime);

  for (i = 0; i < sonarob->sonar->scan_count; i++)
  {
    snprintf(s, sizeof(s), "%05.3f ",
             sonarob->sonar->scan[i]);
    assert(strlen(str) + strlen(s) < sizeof(str));
    strcat(str, s);
  }
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void sonar_onread(sonar_object_t *sonarob)
{
  int i;
  double r, x, y;
  PyObject *tuple;

  thread_acquire();
    
  Py_DECREF(sonarob->scan);
  sonarob->scan = PyList_New(sonarob->sonar->scan_count);
    
  for (i = 0; i < sonarob->sonar->scan_count; i++)
  {
    // Compute position of sonar ray end-point
    r = sonarob->sonar->scan[i];
    x = sonarob->sonar->poses[i][0] + r * cos(sonarob->sonar->poses[i][2]);
    y = sonarob->sonar->poses[i][1] + r * sin(sonarob->sonar->poses[i][2]);
    
    tuple = PyTuple_New(4);
    PyTuple_SetItem(tuple, 0, PyFloat_FromDouble(sonarob->sonar->scan[i]));
    PyTuple_SetItem(tuple, 1, PyFloat_FromDouble(sonarob->sonar->poses[i][0]));
    PyTuple_SetItem(tuple, 2, PyFloat_FromDouble(sonarob->sonar->poses[i][1]));
    PyTuple_SetItem(tuple, 3, PyFloat_FromDouble(x));
    PyTuple_SetItem(tuple, 4, PyFloat_FromDouble(y));
    PyList_SetItem(sonarob->scan, i, tuple);
  }

  thread_release();
}


/* Subscribe to the device. */
static PyObject *sonar_subscribe(PyObject *self, PyObject *args)
{
  char access;
  sonar_object_t *sonarob;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  sonarob = (sonar_object_t*) self;

  thread_release();
  result = playerc_sonar_subscribe(sonarob->sonar, access);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  playerc_sonar_get_geom(sonarob->sonar);

  Py_INCREF(Py_None);
  return Py_None;
}


/* Unsubscribe from the device. */
static PyObject *sonar_unsubscribe(PyObject *self, PyObject *args)
{
  sonar_object_t *sonarob;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  sonarob = (sonar_object_t*) self;

  thread_release();
  result = playerc_sonar_unsubscribe(sonarob->sonar);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Assemble python sonar type
 */
PyTypeObject sonar_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "sonar",
  sizeof(sonar_object_t),
  0,
  sonar_del, /*tp_dealloc*/
  0,          /*tp_print*/
  sonar_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
  0,          /*tp_call*/
  sonar_str,  /*tp_string*/
};


static PyMethodDef sonar_methods[] =
{
  {"subscribe", sonar_subscribe, METH_VARARGS},
  {"unsubscribe", sonar_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};
