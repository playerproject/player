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
 * Date: 22 May 2003
 * Desc: Python bindings for localize device
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


/* Python wrapper for localize type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_localize_t *obj;
  PyObject *pyhypoths;
} localize_object_t;


/* Callback for post-processing incoming data */
static void localize_onread(localize_object_t *self);


PyTypeObject localize_type;
staticforward PyMethodDef localize_methods[];


/* Initialise (type function) */
PyObject *localize_new(localize_object_t *self, PyObject *args)
{
  pyclient_t *pyclient;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  self = PyObject_New(localize_object_t, &localize_type);
  self->client = pyclient->client;
  self->obj = playerc_localize_create(pyclient->client, index);
  self->obj->info.user_data = self;
  self->pyhypoths = PyList_New(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) self->obj,
                             (playerc_callback_fn_t) localize_onread, (void*) self);

  return (PyObject*) self;
}


/* Finailize (type function) */
static void localize_del(localize_object_t *self)
{
  playerc_localize_destroy(self->obj);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *localize_getattr(localize_object_t *self, char *attrname)
{
  PyObject *result;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(self->obj->info.datatime);
  }
  else if (strcmp(attrname, "pending_count") == 0)
  {
    result = PyInt_FromLong(self->obj->pending_count);
  }
  if (strcmp(attrname, "pending_time") == 0)
  {
    result = PyFloat_FromDouble(self->obj->pending_time);
  }
  else if (strcmp(attrname, "hypoths") == 0)
  {
    Py_INCREF(self->pyhypoths);
    return self->pyhypoths;
  }
  else if (strcmp(attrname, "fresh") == 0)
  {
    result = Py_BuildValue("i", self->obj->info.fresh);
    // HACKETY-HACK-HACK: i'm too lazy to implement a setattr function to
    // allow the user to set the fresh flag to 0, so we'll just do it here,
    // on the assumption that the user will immediately read the data (if
    // it's currently fresh) and thereafter consider it stale
    //
    // Furthermore, for this mechanism to work in general analagous attribute 
    // support should be added to other device proxies.
    self->obj->info.fresh = 0;
  }
  else
    result = Py_FindMethod(localize_methods, (PyObject*) self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *localize_str(localize_object_t *self)
{
  int i;
  char s[1024], str[4096];
  playerc_localize_hypoth_t *h;

  snprintf(str, sizeof(str),
           "localize %02d %013.3f %03d %013.3f ",
           self->obj->info.index, self->obj->info.datatime,
           self->obj->pending_count, self->obj->pending_time);
  
  for (i = 0; i < self->obj->hypoth_count; i++)
  {
    h = self->obj->hypoths + i;
    snprintf(s, sizeof(s), "%e %e %e %e %e %e %e %e %e %e %e %e %e ",
             h->weight, h->mean[0], h->mean[1], h->mean[2],
             h->cov[0][0], h->cov[0][1], h->cov[0][2],
             h->cov[1][0], h->cov[1][1], h->cov[1][2],
             h->cov[2][0], h->cov[2][1], h->cov[2][2]);
    assert(strlen(str) + strlen(s) < sizeof(str));
    strcat(str, s);
  }
  
  return PyString_FromString(str);
}


/* Subscribe to the device. */
static PyObject *localize_subscribe(localize_object_t *self, PyObject *args)
{
  char access;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;

  thread_release();
  result = playerc_localize_subscribe(self->obj, access);
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
static PyObject *localize_unsubscribe(localize_object_t *self, PyObject *args)
{
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  self = (localize_object_t*) self;

  thread_release();
  result = playerc_localize_unsubscribe(self->obj);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/* set pose */
static PyObject *localize_set_pose(localize_object_t *self, PyObject *args)
{
  int result;
  double pose[3];
  double var[3];
  double cov[3][3];
    
  if (!PyArg_ParseTuple(args, "(ddd)(ddd)",
      pose,pose+1,pose+2,var,var+1,var+2))
    return NULL;
  self = (localize_object_t*) self;
  memset(cov,0,3*3*sizeof(double));
  cov[0][0] = var[0]*var[0];
  cov[1][1] = var[1]*var[0];
  cov[2][2] = var[2]*var[2];

  thread_release();
  result = playerc_localize_set_pose(self->obj, pose, cov);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Callback for post-processing incoming data */
static void localize_onread(localize_object_t *self)
{
  int i;
  playerc_localize_hypoth_t *h;
  PyObject *pyh;

  thread_acquire();

  Py_DECREF(self->pyhypoths);

  self->pyhypoths = PyList_New(0);

  for (i = 0; i < self->obj->hypoth_count; i++)
  {
    h = self->obj->hypoths + i;
    pyh = Py_BuildValue("(d(ddd)((ddd)(ddd)(ddd)))",
                        h->weight, h->mean[0], h->mean[1], h->mean[2],
                        h->cov[0][0], h->cov[0][1], h->cov[0][2],
                        h->cov[1][0], h->cov[1][1], h->cov[1][2],
                        h->cov[2][0], h->cov[2][1], h->cov[2][2]);
    PyList_Append(self->pyhypoths, pyh);
  }
  
  thread_release();

  return;
}


/* Assemble python localize type
 */
PyTypeObject localize_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "localize",
  sizeof(localize_object_t),
  0,
  (void*) localize_del, /*tp_dealloc*/
  0,          /*tp_print*/
  (void*) localize_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  (void*) localize_str, /*tp_string*/
};


static PyMethodDef localize_methods[] =
{
  {"subscribe", (void*) localize_subscribe, METH_VARARGS},
  {"unsubscribe", (void*) localize_unsubscribe, METH_VARARGS},  
  {"set_pose", (void*) localize_set_pose, METH_VARARGS},  
  {NULL, NULL}
};

