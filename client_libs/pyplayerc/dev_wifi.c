/***************************************************************************
 *
 * Desc: Python bindings for wifi device
 * Author: Andrew H
 * Date: 20 Nov 2002
 * CVS: $Id$
 *
 **************************************************************************/

#include <pthread.h>
#include "Python.h"
#include "playerc.h"
#include "pyplayerc.h"


// Python wrapper for wifi type
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_wifi_t *wifi;
  PyObject *values;
} pywifi_t;

// Local declarations
static void wifi_onread(pywifi_t *wifiob);


PyTypeObject wifi_type;
staticforward PyMethodDef wifi_methods[];


PyObject *wifi_new(PyObject *self, PyObject *args)
{
  client_object_t *pyclient;
  pywifi_t *pywifi;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  pywifi = PyObject_New(pywifi_t, &wifi_type);
  pywifi->client = pyclient->client;
  pywifi->wifi = playerc_wifi_create(pyclient->client, index);
  pywifi->wifi->info.user_data = pywifi;
  pywifi->values = PyTuple_New(3);

  // Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) pywifi->wifi,
                             (playerc_callback_fn_t) wifi_onread,
                             (void*) pywifi);
    
  return (PyObject*) pywifi;
}


static void wifi_del(PyObject *self)
{
  pywifi_t *pywifi;
  pywifi = (pywifi_t*) self;

  playerc_client_delcallback(pywifi->client, (playerc_device_t*) pywifi->wifi,
                             (playerc_callback_fn_t) wifi_onread,
                             (void*) pywifi);    
  Py_DECREF(pywifi->values);
  playerc_wifi_destroy(pywifi->wifi);
  PyObject_Del(self);
}


static PyObject *wifi_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  pywifi_t *pywifi;

  pywifi = (pywifi_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(pywifi->wifi->info.datatime);
  }
  else if (strcmp(attrname, "link") == 0)
  {
    result = Py_BuildValue("i", pywifi->wifi->link);
  }
  else if (strcmp(attrname, "level") == 0)
  {
    result = Py_BuildValue("i", pywifi->wifi->level);
  }
  else if (strcmp(attrname, "noise") == 0)
  {
    result = Py_BuildValue("i", pywifi->wifi->noise);
  }
  else
    result = Py_FindMethod(wifi_methods, self, attrname);

  return result;
}



// Get string representation (type function)
static PyObject *wifi_str(PyObject *self)
{
  char str[8192];
  pywifi_t *pywifi;

  pywifi = (pywifi_t*) self;

  snprintf(str, sizeof(str),
           "wifi %02d %013.3f %d %d %d",
           pywifi->wifi->info.index,
           pywifi->wifi->info.datatime,
           pywifi->wifi->link,
           pywifi->wifi->level,
           pywifi->wifi->noise);

  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void wifi_onread(pywifi_t *pywifi)
{
  thread_acquire();
    
  // Do nothing for now
  
  thread_release();

  return;
}


// Subscribe to the device.
static PyObject *wifi_subscribe(PyObject *self, PyObject *args)
{
  char access;
  pywifi_t *pywifi;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pywifi = (pywifi_t*) self;

  thread_release();
  result = playerc_wifi_subscribe(pywifi->wifi, access);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_errorstr);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


// Unsubscribe from the device.
static PyObject *wifi_unsubscribe(PyObject *self, PyObject *args)
{
  pywifi_t *pywifi;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pywifi = (pywifi_t*) self;

  thread_release();
  result = playerc_wifi_unsubscribe(pywifi->wifi);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_errorstr);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}



// Assemble python wifi type
PyTypeObject wifi_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "wifi",
  sizeof(pywifi_t),
  0,
  wifi_del, /*tp_dealloc*/
  0,          /*tp_print*/
  wifi_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
  0,          /*tp_call*/
  wifi_str,  /*tp_string*/
};


static PyMethodDef wifi_methods[] =
{
  {"subscribe", wifi_subscribe, METH_VARARGS},
  {"unsubscribe", wifi_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};
