/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for comms device
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


/* Python wrapper for comms type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_comms_t *comms;
} pycomms_t;


PyTypeObject comms_type;
staticforward PyMethodDef comms_methods[];

/* Local declarations */
static void comms_onread(pycomms_t *pycomms);


/* Initialise (type function) */
PyObject *comms_new(PyObject *self, PyObject *args)
{
  pyclient_t *clientob;
  pycomms_t *pycomms;
  int robot, index;

  if (!PyArg_ParseTuple(args, "Oii", &clientob, &robot, &index))
    return NULL;

  pycomms = PyObject_New(pycomms_t, &comms_type);
  pycomms->client = clientob->client;
  pycomms->comms = playerc_comms_create(clientob->client, robot, index);
  pycomms->comms->info.user_data = pycomms;

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(clientob->client, (playerc_device_t*) pycomms->comms,
                             (playerc_callback_fn_t) comms_onread,
                             (void*) pycomms);
    
  return (PyObject*) pycomms;
}


/* Finailize (type function) */
static void comms_del(PyObject *self)
{
  pycomms_t *pycomms;
  pycomms = (pycomms_t*) self;

  playerc_client_delcallback(pycomms->client, (playerc_device_t*) pycomms->comms,
                             (playerc_callback_fn_t) comms_onread,
                             (void*) pycomms);    

  playerc_comms_destroy(pycomms->comms);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *comms_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  pycomms_t *pycomms;

  pycomms = (pycomms_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(pycomms->comms->info.datatime);
  }
  else
    result = Py_FindMethod(comms_methods, self, attrname);

  return result;
}


/* Get string representation (type function) */
static PyObject *comms_str(PyObject *self)
{
  char str[128];
  pycomms_t *pycomms;
  pycomms = (pycomms_t*) self;

  snprintf(str, sizeof(str),
           "comms %02d %013.3f",
           pycomms->comms->info.index,
           pycomms->comms->info.datatime);
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void comms_onread(pycomms_t *pycomms)
{
  thread_acquire();
    
  // TODO
  
  thread_release();
}


/* Subscribe to the device. */
static PyObject *comms_subscribe(PyObject *self, PyObject *args)
{
  char access;
  pycomms_t *pycomms;
  int result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  pycomms = (pycomms_t*) self;

  thread_release();
  result = playerc_comms_subscribe(pycomms->comms, access);
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
static PyObject *comms_unsubscribe(PyObject *self, PyObject *args)
{
  pycomms_t *pycomms;
  int result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pycomms = (pycomms_t*) self;

  thread_release();
  result = playerc_comms_unsubscribe(pycomms->comms);
  thread_acquire();

  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


// Send a message
static PyObject *comms_send(PyObject *self, PyObject *args)
{
  pycomms_t *pycomms;
  char *msg;
  int msg_len;
    
  if (!PyArg_ParseTuple(args, "s#", &msg, &msg_len))
    return NULL;
  pycomms = (pycomms_t*) self;

  thread_release();
  playerc_comms_send(pycomms->comms, msg, msg_len);
  thread_acquire();

  Py_INCREF(Py_None);
  return Py_None;
}


// Read any queued message
static PyObject *comms_recv(PyObject *self, PyObject *args)
{
  pycomms_t *pycomms;
  int len;
  char *msg;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  pycomms = (pycomms_t*) self;

  len = pycomms->comms->msg_len;
  msg = pycomms->comms->msg;

  if (len <= 0)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return PyString_FromStringAndSize(msg, len);
}


/* Assemble python comms type
 */
PyTypeObject comms_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "comms",
  sizeof(pycomms_t),
  0,
  comms_del, /*tp_dealloc*/
  0,          /*tp_print*/
  comms_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash*/
  0,          /*tp_call*/
  comms_str, /*tp_string*/
};


static PyMethodDef comms_methods[] =
{
  {"subscribe", comms_subscribe, METH_VARARGS},
  {"unsubscribe", comms_unsubscribe, METH_VARARGS},
  {"send", comms_send, METH_VARARGS},
  {"recv", comms_recv, METH_VARARGS},
  {NULL, NULL}
};

