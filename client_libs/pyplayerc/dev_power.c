/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for power device
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


/* Python wrapper for power type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_power_t *power;
} power_object_t;


PyTypeObject power_type;
staticforward PyMethodDef power_methods[];


/* Initialise (type function) */
PyObject *power_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  power_object_t *pypower;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  pypower = PyObject_New(power_object_t, &power_type);
  pypower->client = pyclient->client;
  pypower->power = playerc_power_create(pyclient->client, index);
  pypower->power->info.user_data = pypower;
    
  return (PyObject*) pypower;
}


/* Finailize (type function) */
static void power_del(PyObject *self)
{
  power_object_t *pypower;
  pypower = (power_object_t*) self;

  playerc_power_destroy(pypower->power);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *power_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  power_object_t *pypower;

  pypower = (power_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(pypower->power->info.datatime);
  }
  else if (strcmp(attrname, "charge") == 0)
  {
    result = Py_BuildValue("d", pypower->power->charge);
  }
  else
    result = Py_FindMethod(power_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *power_str(PyObject *self)
{
  char str[128];
  power_object_t *pypower;
  pypower = (power_object_t*) self;

  snprintf(str, sizeof(str),
           "power %02d %013.3f"
           " %+07.3f",
           pypower->power->info.index,
           pypower->power->info.datatime,
           pypower->power->charge);
  return PyString_FromString(str);
}


/* Subscribe to the device. */
static PyObject *power_subscribe(PyObject *self, PyObject *args)
{
  char access;
  power_object_t *pypower;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pypower = (power_object_t*) self;

  thread_release();
  result = playerc_power_subscribe(pypower->power, access);
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
static PyObject *power_unsubscribe(PyObject *self, PyObject *args)
{
  power_object_t *pypower;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pypower = (power_object_t*) self;

  thread_release();
  result = playerc_power_unsubscribe(pypower->power);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Assemble python power type
 */
PyTypeObject power_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "power",
  sizeof(power_object_t),
  0,
  power_del, /*tp_dealloc*/
  0,          /*tp_print*/
  power_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  power_str, /*tp_string*/
};


static PyMethodDef power_methods[] =
{
  {"subscribe", power_subscribe, METH_VARARGS},
  {"unsubscribe", power_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};

