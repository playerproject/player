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
  PyObject *vx, *vy, *va;
} truth_object_t;


PyTypeObject truth_type;
staticforward PyMethodDef truth_methods[];


/* Initialise (type function) */
PyObject *truth_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  truth_object_t *pytruth;
  int robot, index;

  if (!PyArg_ParseTuple(args, "Oii", &pyclient, &robot, &index))
    return NULL;

  pytruth = PyObject_New(truth_object_t, &truth_type);
  pytruth->client = pyclient->client;
  pytruth->truth = playerc_truth_create(pyclient->client, robot, index);
    
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

