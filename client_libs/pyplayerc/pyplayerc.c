/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for Player client
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


/***************************************************************************
 * Error handling
 **************************************************************************/

PyObject *errorob;


/***************************************************************************
 * Thread utilities
 **************************************************************************/


/* Index of our info in the TLS area of the thread
   Needed for correct storing and restoring of the Python interpreter state
   in multi-threaded programs.
 */
static pthread_key_t thread_key;


/* Acquire lock and set python state for current thread
 */
void thread_acquire(void)
{
  PyThreadState *state;
    
  state = (PyThreadState*) pthread_getspecific(thread_key);
  PyEval_AcquireLock();
  PyThreadState_Swap(state);
}


/* Release lock and set python state to NULL
 */
void thread_release(void)
{
  PyThreadState *state;
    
  state = PyThreadState_Swap(NULL);
  if (thread_key == 0)
    pthread_key_create(&thread_key, NULL);
  pthread_setspecific(thread_key, state);
  PyEval_ReleaseLock();
}


/***************************************************************************
 * Multi client
 **************************************************************************/


/* Python wrapper for client type */
typedef struct
{
    PyObject_HEAD
    playerc_mclient_t *mclient;
} mpyclient_t;


staticforward PyTypeObject mclient_type;
staticforward PyMethodDef mclient_methods[];


static PyObject *mclient_new(PyObject *self, PyObject *args)
{
  mpyclient_t *mpyclient;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  mpyclient = PyObject_New(mpyclient_t, &mclient_type);
  mpyclient->mclient = playerc_mclient_create();

  return (PyObject*) mpyclient;
}


static void mclient_del(PyObject *self)
{
  mpyclient_t *mpyclient;

  mpyclient = (mpyclient_t*) self;
  playerc_mclient_destroy(mpyclient->mclient);
  PyObject_Del(self);
}


static PyObject *mclient_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  mpyclient_t *mpyclient;

  mpyclient = (mpyclient_t*) self;
  result = Py_FindMethod(mclient_methods, self, attrname);

  return result;
}


// Peek our clients
static PyObject *mclient_peek(PyObject *self, PyObject *args)
{
  int result;
  double timeout;
  mpyclient_t *mpyclient;
  mpyclient = (mpyclient_t*) self;

  if (!PyArg_ParseTuple(args, "d", &timeout))
    return NULL;

  thread_release();
  result = playerc_mclient_peek(mpyclient->mclient, (int) (timeout * 1000));
  thread_acquire();

  if (result < 0)
  {
    PyErr_SetString(errorob, "");
    return NULL;
  }
    
  return PyInt_FromLong(result);
}


// Read and process a message
static PyObject *mclient_read(PyObject *self, PyObject *args)
{
  int result;
  double timeout;
  mpyclient_t *mpyclient;
  mpyclient = (mpyclient_t*) self;

  if (!PyArg_ParseTuple(args, "d", &timeout))
    return NULL;

  thread_release();
  result = playerc_mclient_read(mpyclient->mclient, (int) (timeout * 1000));
  thread_acquire();

  if (result < 0)
  {
    PyErr_SetString(errorob, "");
    return NULL;
  }
    
  return PyInt_FromLong(result);
}



/* Assemble python mclient type
 */
static PyTypeObject mclient_type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "mclient",
    sizeof(mpyclient_t),
    0,
    mclient_del, /*tp_dealloc*/
    0,          /*tp_print*/
    mclient_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
};


static PyMethodDef mclient_methods[] =
{
  {"peek", mclient_peek, METH_VARARGS},
  {"read", mclient_read, METH_VARARGS},
  {NULL, NULL}
};



/***************************************************************************
 * Single client
 **************************************************************************/

/* Python wrapper for client type is declared in header.*/

staticforward PyTypeObject client_type;
staticforward PyMethodDef client_methods[];


static PyObject *client_new(PyObject *self, PyObject *args)
{
  char *hostname;
  int port;
  mpyclient_t *mpyclient;
  pyclient_t *pyclient;

  if (!PyArg_ParseTuple(args, "Osi", &mpyclient, &hostname, &port))
    return NULL;

  pyclient = PyObject_New(pyclient_t, &client_type);
  if ((PyObject*) mpyclient == Py_None)
    pyclient->client = playerc_client_create(NULL, hostname, port);
  else
    pyclient->client = playerc_client_create(mpyclient->mclient, hostname, port);

  pyclient->idlist = PyTuple_New(0);

  return (PyObject*) pyclient;
}


static void client_del(PyObject *self)
{
  pyclient_t *pyclient;

  pyclient = (pyclient_t*) self;
  playerc_client_destroy(pyclient->client);
  PyObject_Del(self);
}


static PyObject *client_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  pyclient_t *pyclient;

  pyclient = (pyclient_t*) self;

  result = NULL;
  if (strcmp(attrname, "host") == 0)
  {
    result = PyString_FromString(pyclient->client->host);
  }
  else if (strcmp(attrname, "devlist") == 0)
  {
    Py_INCREF(pyclient->idlist);
    result = pyclient->idlist;
  }
  else
    result = Py_FindMethod(client_methods, self, attrname);

  return result;
}


/* Connect to server
 */
static PyObject *client_connect(PyObject *self, PyObject *args)
{
  int result;
  pyclient_t *pyclient;
  pyclient = (pyclient_t*) self;

  thread_release();
  result = playerc_client_connect(pyclient->client);
  thread_acquire();
  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }
  return PyInt_FromLong(result);
}


/* Disconnect from server
 */
static PyObject *client_disconnect(PyObject *self, PyObject *args)
{
  int result;
  pyclient_t *pyclient;
  pyclient = (pyclient_t*) self;

  result = playerc_client_disconnect(pyclient->client);
  return PyInt_FromLong(result);
}


// See if there are any pending messages.
static PyObject *client_peek(PyObject *self, PyObject *args)
{
  int result;
  double timeout;
  pyclient_t *pyclient;
  pyclient = (pyclient_t*) self;

  if (!PyArg_ParseTuple(args, "d", &timeout))
    return NULL;

  thread_release();
  result = playerc_client_peek(pyclient->client, (int) (timeout * 1000));
  thread_acquire();
    
  return PyInt_FromLong(result);
}


/* Read and process a message
*/
static PyObject *client_read(PyObject *self, PyObject *args)
{
  int i;
  void *result;
  PyObject *resultob;
  pyclient_t *pyclient;
  pyclient = (pyclient_t*) self;

  thread_release();
  result = playerc_client_read(pyclient->client);
  thread_acquire();

  if (result == NULL)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }
  else
  {
    // See if it was a synch packet
    if (result == pyclient->client)
    {
      resultob = (PyObject*) pyclient;
      Py_INCREF(resultob);
      return resultob;
    }
    
    // Go through the list of devices and work our which one we got.
    for (i = 0; i < pyclient->client->device_count; i++)
    {
      if (pyclient->client->device[i] == result)
      {
        resultob = (PyObject*) pyclient->client->device[i]->user_data;
        Py_INCREF(resultob);
        return resultob;
      }
    }
    
    // Should never get here!
    PyErr_SetString(errorob, "internal error: device not found");
    return NULL;
  }
}


// Query the device list.
static PyObject *client_get_devlist(PyObject *self, PyObject *args)
{
  int i;
  int result;
  pyclient_t *pyclient;
  player_device_id_t *id;
  PyObject *idob;

  pyclient = (pyclient_t*) self;

  thread_release();
  result = playerc_client_get_devlist(pyclient->client);
  thread_acquire();

  if (result)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_error_str());
    return NULL;
  }

  // Build the available device list.
  Py_DECREF(pyclient->idlist);
  pyclient->idlist = PyTuple_New(pyclient->client->id_count);
  for (i = 0; i < pyclient->client->id_count; i++)
  {
    id = pyclient->client->ids + i;
    idob = PyTuple_New(3);
    PyTuple_SetItem(idob, 0, PyInt_FromLong(id->code));
    PyTuple_SetItem(idob, 1, PyInt_FromLong(id->index));
    PyTuple_SetItem(idob, 2, PyString_FromString(playerc_lookup_name(id->code)));
    PyTuple_SetItem(pyclient->idlist, i, idob);
  }

  return PyInt_FromLong(result);
}


/* Assemble python client type
 */
static PyTypeObject client_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "client",
  sizeof(pyclient_t),
  0,
  client_del, /*tp_dealloc*/
  0,          /*tp_print*/
  client_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};


static PyMethodDef client_methods[] =
{
  {"connect", client_connect, METH_VARARGS},
  {"disconnect", client_disconnect, METH_VARARGS},
  {"peek", client_peek, METH_VARARGS},
  {"read", client_read, METH_VARARGS},
  {"get_devlist", client_get_devlist, METH_VARARGS},
  {NULL, NULL}
};



/***************************************************************************
 * module level stuff
 **************************************************************************/

extern PyTypeObject blobfinder_type;
extern PyObject *blobfinder_new(PyObject *self, PyObject *args);

extern PyTypeObject comms_type;
extern PyObject *comms_new(PyObject *self, PyObject *args);

extern PyTypeObject fiducial_type;
extern PyObject *fiducial_new(PyObject *self, PyObject *args);

extern PyTypeObject gps_type;
extern PyObject *gps_new(PyObject *self, PyObject *args);

extern PyTypeObject laser_type;
extern PyObject *laser_new(PyObject *self, PyObject *args);

extern PyTypeObject position_type;
extern PyObject *position_new(PyObject *self, PyObject *args);

extern PyTypeObject power_type;
extern PyObject *power_new(PyObject *self, PyObject *args);

extern PyTypeObject ptz_type;
extern PyObject *ptz_new(PyObject *self, PyObject *args);

extern PyTypeObject truth_type;
extern PyObject *truth_new(PyObject *self, PyObject *args);

extern PyTypeObject wifi_type;
extern PyObject *wifi_new(PyObject *self, PyObject *args);



static PyMethodDef module_methods[] =
{
  {"mclient", mclient_new, METH_VARARGS},
  {"client", client_new, METH_VARARGS},
  {"laser", laser_new, METH_VARARGS},
  {"position", position_new, METH_VARARGS},
  {"power", power_new, METH_VARARGS},
  {"ptz", ptz_new, METH_VARARGS},
  {"blobfinder", blobfinder_new, METH_VARARGS},
  {"fiducial", fiducial_new, METH_VARARGS},
  {"gps", gps_new, METH_VARARGS},
  {"comms", comms_new, METH_VARARGS},
  {"truth", truth_new, METH_VARARGS},
  {"wifi", wifi_new, METH_VARARGS},
  {NULL, NULL}
};


void initplayerc(void)
{
  PyObject *moduleob;
    
  /* Finish initialization of type objects here */
  mclient_type.ob_type = &PyType_Type;
  client_type.ob_type = &PyType_Type;
  laser_type.ob_type = &PyType_Type;
  position_type.ob_type = &PyType_Type;
  power_type.ob_type = &PyType_Type;
  ptz_type.ob_type = &PyType_Type;
  blobfinder_type.ob_type = &PyType_Type;
  fiducial_type.ob_type = &PyType_Type;
  gps_type.ob_type = &PyType_Type;
  wifi_type.ob_type = &PyType_Type;
    
  moduleob = Py_InitModule("playerc", module_methods);

  /* Add out exception */
  errorob = PyErr_NewException("playerc.error", NULL, NULL);
  PyDict_SetItemString(PyModule_GetDict(moduleob), "error", errorob);

  /* Make sure an interpreter lock object is created, since
     we will may try to release it at some point.
  */
  PyEval_InitThreads();
}

