/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for truth device
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


/* Python wrapper for truth type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_truth_t *truth;
  PyObject *px, *py, *pa;
} truth_object_t;


PyTypeObject truth_type;
staticforward PyMethodDef truth_methods[];

/* Callback for post-processing incoming data */
static void truth_onread(truth_object_t *pytruth)
{
  thread_acquire();
    
  Py_DECREF(pytruth->px);
  Py_DECREF(pytruth->py);
  Py_DECREF(pytruth->pa);
  pytruth->px = PyFloat_FromDouble(pytruth->truth->px);
  pytruth->py = PyFloat_FromDouble(pytruth->truth->py);
  pytruth->pa = PyFloat_FromDouble(pytruth->truth->pa);    
    
  thread_release();
}

/* Initialise (type function) */
PyObject *truth_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  truth_object_t *pytruth;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &pyclient, &index))
    return NULL;

  pytruth = PyObject_New(truth_object_t, &truth_type);
  pytruth->client = pyclient->client;
  pytruth->truth = playerc_truth_create(pyclient->client, index);
  pytruth->truth->info.user_data = pytruth;
  pytruth->px = PyFloat_FromDouble(0);
  pytruth->py = PyFloat_FromDouble(0);
  pytruth->pa = PyFloat_FromDouble(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, 
                             (playerc_device_t*) pytruth->truth,
                             (playerc_callback_fn_t) truth_onread,
                             (void*) pytruth);
    
  return (PyObject*) pytruth;
}


/* Finailize (type function) */
static void truth_del(PyObject *self)
{
  truth_object_t *pytruth;
  pytruth = (truth_object_t*) self;

  playerc_truth_destroy(pytruth->truth);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *truth_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  truth_object_t *pytruth;

  pytruth = (truth_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
    result = PyFloat_FromDouble(pytruth->truth->info.datatime);
  else if (strcmp(attrname, "px") == 0)
  {
    Py_INCREF(pytruth->px);
    result = pytruth->px;
  }
  else if (strcmp(attrname, "py") == 0)
  {
    Py_INCREF(pytruth->py);
    result = pytruth->py;
  }
  else if (strcmp(attrname, "pa") == 0)
  {
    Py_INCREF(pytruth->pa);
    result = pytruth->pa;
  }
  else
    result = Py_FindMethod(truth_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *truth_str(PyObject *self)
{
  char str[128];
  truth_object_t *pytruth;
  pytruth = (truth_object_t*) self;

  snprintf(str, sizeof(str),
           "truth %02d %013.3f",
           pytruth->truth->info.index,
           pytruth->truth->info.datatime);
  return PyString_FromString(str);
}

/* Subscribe to the device. */
static PyObject *truth_subscribe(PyObject *self, PyObject *args)
{
  char access;
  truth_object_t *pytruth;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pytruth = (truth_object_t*) self;

  thread_release();
  result = playerc_truth_subscribe(pytruth->truth, access);
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
static PyObject *truth_unsubscribe(PyObject *self, PyObject *args)
{
  truth_object_t *pytruth;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pytruth = (truth_object_t*) self;

  thread_release();
  result = playerc_truth_unsubscribe(pytruth->truth);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/* Get the robot pose
*/
static PyObject *truth_get_pose(PyObject *self, PyObject *args)
{
  double px, py, pa;
  truth_object_t *pytruth;

  pytruth = (truth_object_t*) self;
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  if (playerc_truth_get_pose(pytruth->truth, &px, &py, &pa) != 0)
    return NULL;

  return Py_BuildValue("(ddd)", &px, &py, &pa);
}

/* Set the robot pose
*/
static PyObject *truth_set_pose(PyObject *self, PyObject *args)
{
  double px, py, pa;
  truth_object_t *pytruth;

  pytruth = (truth_object_t*) self;
  if (!PyArg_ParseTuple(args, "ddd", &px, &py, &pa))
    return NULL;

  return PyInt_FromLong(playerc_truth_set_pose(pytruth->truth, px, py, pa));
}


/* Assemble python truth type
 */
PyTypeObject truth_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "truth",
  sizeof(truth_object_t),
  0,
  truth_del, /*tp_dealloc*/
  0,          /*tp_print*/
  truth_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  truth_str, /*tp_string*/
};


static PyMethodDef truth_methods[] =
{
  {"subscribe", truth_subscribe, METH_VARARGS},
  {"unsubscribe", truth_unsubscribe, METH_VARARGS},
  {"get_pose", truth_get_pose, METH_VARARGS},  
  {"set_pose", truth_set_pose, METH_VARARGS},
  {NULL, NULL}
};

