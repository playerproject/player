/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for gps device
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


/* Python wrapper for gps type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_gps_t *gps;
  PyObject *px, *py, *pa;
} gps_object_t;


PyTypeObject gps_type;
staticforward PyMethodDef gps_methods[];

/* Local declarations */
static void gps_onread(gps_object_t *pygps);


/* Initialise (type function) */
PyObject *gps_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  gps_object_t *pygps;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  pygps = PyObject_New(gps_object_t, &gps_type);
  pygps->client = pyclient->client;
  pygps->gps = playerc_gps_create(pyclient->client, index);
  pygps->gps->info.user_data = pygps;
  pygps->px = PyFloat_FromDouble(0);
  pygps->py = PyFloat_FromDouble(0);
  pygps->pa = PyFloat_FromDouble(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) pygps->gps,
                             (playerc_callback_fn_t) gps_onread,
                             (void*) pygps);
    
  return (PyObject*) pygps;
}


/* Finailize (type function) */
static void gps_del(PyObject *self)
{
  gps_object_t *pygps;
  pygps = (gps_object_t*) self;

  playerc_client_delcallback(pygps->client, (playerc_device_t*) pygps->gps,
                             (playerc_callback_fn_t) gps_onread,
                             (void*) pygps);    

  Py_DECREF(pygps->px);
  Py_DECREF(pygps->py);
  Py_DECREF(pygps->pa);
  playerc_gps_destroy(pygps->gps);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *gps_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  gps_object_t *pygps;

  pygps = (gps_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(pygps->gps->info.datatime);
  }
  else if (strcmp(attrname, "px") == 0)
  {
    Py_INCREF(pygps->px);
    result = pygps->px;
  }
  else if (strcmp(attrname, "py") == 0)
  {
    Py_INCREF(pygps->py);
    result = pygps->py;
  }
  else if (strcmp(attrname, "pa") == 0)
  {
    Py_INCREF(pygps->pa);
    result = pygps->pa;
  }
  else
    result = Py_FindMethod(gps_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *gps_str(PyObject *self)
{
  char str[128];
  gps_object_t *pygps;
  pygps = (gps_object_t*) self;

  snprintf(str, sizeof(str),
           "gps %02d %013.3f"
           " %+07.3f %+07.3f %+04.3f",
           pygps->gps->info.index,
           pygps->gps->info.datatime,
           pygps->gps->px,
           pygps->gps->py,
           pygps->gps->pa);
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void gps_onread(gps_object_t *pygps)
{
  thread_acquire();
    
  Py_DECREF(pygps->px);
  Py_DECREF(pygps->py);
  Py_DECREF(pygps->pa);
  pygps->px = PyFloat_FromDouble(pygps->gps->px);
  pygps->py = PyFloat_FromDouble(pygps->gps->py);
  pygps->pa = PyFloat_FromDouble(pygps->gps->pa);    
    
  thread_release();
}


/* Subscribe to the device. */
static PyObject *gps_subscribe(PyObject *self, PyObject *args)
{
  char access;
  gps_object_t *pygps;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pygps = (gps_object_t*) self;

  thread_release();
  result = playerc_gps_subscribe(pygps->gps, access);
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
static PyObject *gps_unsubscribe(PyObject *self, PyObject *args)
{
  gps_object_t *pygps;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pygps = (gps_object_t*) self;

  thread_release();
  result = playerc_gps_unsubscribe(pygps->gps);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Assemble python gps type
 */
PyTypeObject gps_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "gps",
  sizeof(gps_object_t),
  0,
  gps_del, /*tp_dealloc*/
  0,          /*tp_print*/
  gps_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  gps_str, /*tp_string*/
};


static PyMethodDef gps_methods[] =
{
  {"subscribe", gps_subscribe, METH_VARARGS},
  {"unsubscribe", gps_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};

