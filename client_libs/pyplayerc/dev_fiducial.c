/***************************************************************************
 *
 * Desc: Python bindings for fiducial  device
 * Author: Andrew H
 * Date: 24 Aug 2001
 * CVS: $Id$
 *
 **************************************************************************/

#include <pthread.h>
#include "Python.h"
#include "playerc.h"
#include "pyplayerc.h"


/* Python wrapper for fiducial type */
typedef struct
{
    PyObject_HEAD
    playerc_client_t *client;
    playerc_fiducial_t *fiducial;
    PyObject *fiducials;
} fiducial_object_t;

/* Local declarations */
static void fiducial_onread(fiducial_object_t *fiducialob);

PyTypeObject fiducial_type;
staticforward PyMethodDef fiducial_methods[];

 
PyObject *fiducial_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  fiducial_object_t *fiducialob;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  fiducialob = PyObject_New(fiducial_object_t, &fiducial_type);
  fiducialob->client = pyclient->client;
  fiducialob->fiducial = playerc_fiducial_create(pyclient->client, index);
  fiducialob->fiducial->info.user_data = fiducialob;
  fiducialob->fiducials = PyList_New(0);
    
  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client,
                             (playerc_device_t*) fiducialob->fiducial,
                             (playerc_callback_fn_t) fiducial_onread,
                             (void*) fiducialob);
    
  return (PyObject*) fiducialob;
}


static void fiducial_del(PyObject *self)
{
  fiducial_object_t *fiducialob;
  fiducialob = (fiducial_object_t*) self;

  playerc_client_delcallback(fiducialob->client,
                             (playerc_device_t*) fiducialob->fiducial,
                             (playerc_callback_fn_t) fiducial_onread,
                             (void*) fiducialob);    
  playerc_fiducial_destroy(fiducialob->fiducial);
  PyObject_Del(self);
}


static PyObject *fiducial_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  fiducial_object_t *fiducialob;

  fiducialob = (fiducial_object_t*) self;

  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(fiducialob->fiducial->info.datatime);
  }
  else if (strcmp(attrname, "fiducials") == 0)
  {
    Py_INCREF(fiducialob->fiducials);
    result = fiducialob->fiducials;
  }
  else
    result = Py_FindMethod(fiducial_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *fiducial_str(PyObject *self)
{
  int i;
  char s[64];
  char str[8192];
  fiducial_object_t *fiducialob;
  fiducialob = (fiducial_object_t*) self;

  snprintf(str, sizeof(str),
           "fiducial %02d %013.3f ",
           fiducialob->fiducial->info.index,
           fiducialob->fiducial->info.datatime);

  for (i = 0; i < fiducialob->fiducial->fiducial_count; i++)
  {
    snprintf(s, sizeof(s), "%03d %05.3f %+05.3f %+05.3f ",
             fiducialob->fiducial->fiducials[i].id,
             fiducialob->fiducial->fiducials[i].range,
             fiducialob->fiducial->fiducials[i].bearing,
             fiducialob->fiducial->fiducials[i].orient);
    assert(strlen(str) + strlen(s) < sizeof(str));
    strcat(str, s);
  }
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void fiducial_onread(fiducial_object_t *fiducialob)
{
  int i;
  PyObject *tuple;

  thread_acquire();
    
  Py_DECREF(fiducialob->fiducials);
  fiducialob->fiducials = PyList_New(fiducialob->fiducial->fiducial_count);
    
  for (i = 0; i < fiducialob->fiducial->fiducial_count; i++)
  {
    tuple = PyTuple_New(4);
    PyTuple_SetItem(tuple, 0, PyInt_FromLong(fiducialob->fiducial->fiducials[i].id));
    PyTuple_SetItem(tuple, 1, PyFloat_FromDouble(fiducialob->fiducial->fiducials[i].range));
    PyTuple_SetItem(tuple, 2, PyFloat_FromDouble(fiducialob->fiducial->fiducials[i].bearing));
    PyTuple_SetItem(tuple, 3, PyFloat_FromDouble(fiducialob->fiducial->fiducials[i].orient));
    PyList_SetItem(fiducialob->fiducials, i, tuple);
  }
    
  thread_release();
}


/* Subscribe to the device. */
static PyObject *fiducial_subscribe(PyObject *self, PyObject *args)
{
  char access;
  fiducial_object_t *fiducialob;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  fiducialob = (fiducial_object_t*) self;

  thread_release();
  result = playerc_fiducial_subscribe(fiducialob->fiducial, access);
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
static PyObject *fiducial_unsubscribe(PyObject *self, PyObject *args)
{
  fiducial_object_t *fiducialob;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  fiducialob = (fiducial_object_t*) self;

  thread_release();
  result = playerc_fiducial_unsubscribe(fiducialob->fiducial);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Assemble python fiducial type
 */
PyTypeObject fiducial_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "fiducial",
  sizeof(fiducial_object_t),
  0,
  fiducial_del, /*tp_dealloc*/
  0,          /*tp_print*/
  fiducial_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
  0,          /*tp_call*/
  fiducial_str,  /*tp_string*/
};


static PyMethodDef fiducial_methods[] =
{
  {"subscribe", fiducial_subscribe, METH_VARARGS},
  {"unsubscribe", fiducial_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};

