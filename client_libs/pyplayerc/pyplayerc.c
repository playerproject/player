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
} mclient_object_t;


staticforward PyTypeObject mclient_type;
staticforward PyMethodDef mclient_methods[];


static PyObject *mclient_new(PyObject *self, PyObject *args)
{
    mclient_object_t *mclientob;

    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    mclientob = PyObject_New(mclient_object_t, &mclient_type);
    mclientob->mclient = playerc_mclient_create();

    return (PyObject*) mclientob;
}


static void mclient_del(PyObject *self)
{
    mclient_object_t *mclientob;

    mclientob = (mclient_object_t*) self;
    playerc_mclient_destroy(mclientob->mclient);
    PyObject_Del(self);
}


static PyObject *mclient_getattr(PyObject *self, char *attrname)
{
    PyObject *result;
    mclient_object_t *mclientob;

    mclientob = (mclient_object_t*) self;
    result = Py_FindMethod(mclient_methods, self, attrname);

    return result;
}


/* Connect to server
 */
static PyObject *mclient_connect(PyObject *self, PyObject *args)
{
    int result;
    mclient_object_t *mclientob;
    mclientob = (mclient_object_t*) self;

    result = playerc_mclient_connect(mclientob->mclient);
    return PyInt_FromLong(result);
}


/* Disconnect from server
 */
static PyObject *mclient_disconnect(PyObject *self, PyObject *args)
{
    int result;
    mclient_object_t *mclientob;
    mclientob = (mclient_object_t*) self;

    result = playerc_mclient_disconnect(mclientob->mclient);
    return PyInt_FromLong(result);
}


/* Read and process a message
*/
static PyObject *mclient_read(PyObject *self, PyObject *args)
{
    int result;
    double timeout;
    mclient_object_t *mclientob;
    mclientob = (mclient_object_t*) self;

    if (!PyArg_ParseTuple(args, "d", &timeout))
        return NULL;

    thread_release();
    result = playerc_mclient_read(mclientob->mclient, (int) (timeout * 1000));
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
    sizeof(mclient_object_t),
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
    {"connect", mclient_connect, METH_VARARGS},
    {"disconnect", mclient_disconnect, METH_VARARGS},
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
  mclient_object_t *mclientob;
  client_object_t *clientob;

  if (!PyArg_ParseTuple(args, "Osi", &mclientob, &hostname, &port))
    return NULL;

  clientob = PyObject_New(client_object_t, &client_type);
  if ((PyObject*) mclientob == Py_None)
    clientob->client = playerc_client_create(NULL, hostname, port);
  else
    clientob->client = playerc_client_create(mclientob->mclient, hostname, port);

  clientob->idlist = PyTuple_New(0);

  return (PyObject*) clientob;
}


static void client_del(PyObject *self)
{
  client_object_t *clientob;

  clientob = (client_object_t*) self;
  playerc_client_destroy(clientob->client);
  PyObject_Del(self);
}


static PyObject *client_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  client_object_t *clientob;

  clientob = (client_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "host") == 0)
    result = PyString_FromString(clientob->client->host);
  else if (strcmp(attrname, "devlist") == 0)
  {
    Py_INCREF(clientob->idlist);
    result = clientob->idlist;
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
  client_object_t *clientob;
  clientob = (client_object_t*) self;

  thread_release();
  result = playerc_client_connect(clientob->client);
  thread_acquire();
  if (result < 0)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_errorstr);
    return NULL;
  }
  return PyInt_FromLong(result);
}

/* Disconnect from server
 */
static PyObject *client_disconnect(PyObject *self, PyObject *args)
{
  int result;
  client_object_t *clientob;
  clientob = (client_object_t*) self;

  result = playerc_client_disconnect(clientob->client);
  return PyInt_FromLong(result);
}


/* Read and process a message
*/
static PyObject *client_read(PyObject *self, PyObject *args)
{
  int i;
  void *result;
  PyObject *resultob;
  client_object_t *clientob;
  clientob = (client_object_t*) self;

  thread_release();
  result = playerc_client_read(clientob->client);
  thread_acquire();

  if (result == NULL)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }
  else
  {
    // See if it was a synch packet
    if (result == clientob->client)
    {
      resultob = (PyObject*) clientob;
      Py_INCREF(resultob);
      return resultob;
    }
    
    // Go through the list of devices and work our which one we got.
    for (i = 0; i < clientob->client->device_count; i++)
    {
      if (clientob->client->device[i] == result)
      {
        resultob = (PyObject*) clientob->client->device[i]->user_data;
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
  client_object_t *clientob;
  player_device_id_t *id;
  PyObject *idob;

  clientob = (client_object_t*) self;

  thread_release();
  result = playerc_client_get_devlist(clientob->client);
  thread_acquire();

  if (result)
  {
    PyErr_Format(errorob, "libplayerc: %s", playerc_errorstr);
    return NULL;
  }

  // Build the available device list.
  Py_DECREF(clientob->idlist);
  clientob->idlist = PyTuple_New(clientob->client->id_count);
  for (i = 0; i < clientob->client->id_count; i++)
  {
    id = clientob->client->ids + i;
    idob = PyTuple_New(3);
    PyTuple_SetItem(idob, 0, PyInt_FromLong(id->code));
    PyTuple_SetItem(idob, 1, PyInt_FromLong(id->index));
    PyTuple_SetItem(idob, 2, PyString_FromString(playerc_lookup_name(id->code)));
    PyTuple_SetItem(clientob->idlist, i, idob);
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
  sizeof(client_object_t),
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
    {"read", client_read, METH_VARARGS},
    {"get_devlist", client_get_devlist, METH_VARARGS},
    {NULL, NULL}
};



/***************************************************************************
 * gps device
 **************************************************************************/


/* Python wrapper for gps type */
typedef struct
{
    PyObject_HEAD
    playerc_client_t *client;
    playerc_gps_t *gps;
    PyObject *px, *py, *pa;
} gps_object_t;

/* Local declarations */
static void gps_onread(gps_object_t *gpsob);

staticforward PyTypeObject gps_type;
staticforward PyMethodDef gps_methods[];


static PyObject *gps_new(PyObject *self, PyObject *args)
{
  client_object_t *clientob;
  gps_object_t *gpsob;
  int index;
  char access;

  if (!PyArg_ParseTuple(args, "Oic", &clientob, &index, &access))
    return NULL;

  gpsob = PyObject_New(gps_object_t, &gps_type);
  gpsob->client = clientob->client;
  gpsob->gps = playerc_gps_create(clientob->client, index);
  gpsob->px = PyFloat_FromDouble(0);
  gpsob->py = PyFloat_FromDouble(0);
  gpsob->pa = PyFloat_FromDouble(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(clientob->client, (playerc_device_t*) gpsob->gps,
                             (playerc_callback_fn_t) gps_onread,
                             (void*) gpsob);
    
  return (PyObject*) gpsob;
}


static void gps_del(PyObject *self)
{
  gps_object_t *gpsob;
  gpsob = (gps_object_t*) self;

  playerc_client_delcallback(gpsob->client, (playerc_device_t*) gpsob->gps,
                             (playerc_callback_fn_t) gps_onread,
                             (void*) gpsob);    
  Py_DECREF(gpsob->px);
  Py_DECREF(gpsob->py);
  Py_DECREF(gpsob->pa);
  playerc_gps_destroy(gpsob->gps);
  PyObject_Del(self);
}


static PyObject *gps_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  gps_object_t *gpsob;

  gpsob = (gps_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(gpsob->gps->info.datatime);
  }
  else if (strcmp(attrname, "px") == 0)
  {
    Py_INCREF(gpsob->px);
    result = gpsob->px;
  }
  else if (strcmp(attrname, "py") == 0)
  {
    Py_INCREF(gpsob->py);
    result = gpsob->py;
  }
  else if (strcmp(attrname, "pa") == 0)
  {
    Py_INCREF(gpsob->pa);
    result = gpsob->pa;
  }
  else
    result = Py_FindMethod(gps_methods, self, attrname);

  return result;
}


/* Callback for post-processing incoming data */
static void gps_onread(gps_object_t *gpsob)
{
  thread_acquire();
    
  Py_DECREF(gpsob->px);
  Py_DECREF(gpsob->py);
  Py_DECREF(gpsob->pa);

  gpsob->px = PyFloat_FromDouble(gpsob->gps->px);
  gpsob->py = PyFloat_FromDouble(gpsob->gps->py);
  gpsob->pa = PyFloat_FromDouble(gpsob->gps->pa);    

  thread_release();
}


/* Assemble python gps type
 */
static PyTypeObject gps_type = 
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
  0,          /*tp_hash */
};


static PyMethodDef gps_methods[] =
{
  {NULL, NULL}
};


/***************************************************************************
 * bps device
 **************************************************************************/
/*

// Python wrapper for bps type
typedef struct
{
    PyObject_HEAD
    playerc_client_t *client;
    playerc_bps_t *bps;
    PyObject *px, *py, *pa;
    PyObject *err;
} bps_object_t;

// Local declarations
static void bps_onread(bps_object_t *bpsob);

staticforward PyTypeObject bps_type;
staticforward PyMethodDef bps_methods[];


static PyObject *bps_new(PyObject *self, PyObject *args)
{
    client_object_t *clientob;
    bps_object_t *bpsob;
    int index;
    char access;

    if (!PyArg_ParseTuple(args, "Oic", &clientob, &index, &access))
        return NULL;

    bpsob = PyObject_New(bps_object_t, &bps_type);
    bpsob->client = clientob->client;
    bpsob->bps = playerc_bps_create(clientob->client, index);
    bpsob->px = PyFloat_FromDouble(0);
    bpsob->py = PyFloat_FromDouble(0);
    bpsob->pa = PyFloat_FromDouble(0);
    bpsob->err = PyFloat_FromDouble(0);

    // Add callback for post-processing incoming data.
    playerc_client_addcallback(clientob->client, (playerc_device_t*) bpsob->bps,
                               (playerc_callback_fn_t) bps_onread,
                               (void*) bpsob);
    
    return (PyObject*) bpsob;
}


static void bps_del(PyObject *self)
{
    bps_object_t *bpsob;
    bpsob = (bps_object_t*) self;

    playerc_client_delcallback(bpsob->client, (playerc_device_t*) bpsob->bps,
                               (playerc_callback_fn_t) bps_onread,
                               (void*) bpsob);    
    Py_DECREF(bpsob->px);
    Py_DECREF(bpsob->py);
    Py_DECREF(bpsob->pa);
    Py_DECREF(bpsob->err);
    playerc_bps_destroy(bpsob->bps);
    PyObject_Del(self);
}


static PyObject *bps_getattr(PyObject *self, char *attrname)
{
    PyObject *result;
    bps_object_t *bpsob;

    bpsob = (bps_object_t*) self;

    result = NULL;
    if (strcmp(attrname, "px") == 0)
    {
        Py_INCREF(bpsob->px);
        result = bpsob->px;
    }
    else if (strcmp(attrname, "py") == 0)
    {
        Py_INCREF(bpsob->py);
        result = bpsob->py;
    }
    else if (strcmp(attrname, "pa") == 0)
    {
        Py_INCREF(bpsob->pa);
        result = bpsob->pa;
    }
    else if (strcmp(attrname, "err") == 0)
    {
        Py_INCREF(bpsob->err);
        result = bpsob->err;
    }
    else
        result = Py_FindMethod(bps_methods, self, attrname);

    return result;
}


// Callback for post-processing incoming data.
static void bps_onread(bps_object_t *bpsob)
{
    thread_acquire();
    
    Py_DECREF(bpsob->px);
    Py_DECREF(bpsob->py);
    Py_DECREF(bpsob->pa);
    Py_DECREF(bpsob->err);

    bpsob->px = PyFloat_FromDouble(bpsob->bps->px);
    bpsob->py = PyFloat_FromDouble(bpsob->bps->py);
    bpsob->pa = PyFloat_FromDouble(bpsob->bps->pa);    
    bpsob->err = PyFloat_FromDouble(bpsob->bps->err);
    
    thread_release();
}


// Set the beacon positions
//   (id, (px, py, pa), (ux, uy, ua))
static PyObject *bps_setbeacon(PyObject *self, PyObject *args)
{
    bps_object_t *bpsob;
    int id;
    double px, py, pa;
    double ux, uy, ua;

    bpsob = (bps_object_t*) self;
    if (!PyArg_ParseTuple(args, "i(ddd)(ddd)", &id, &px, &py, &pa, &ux, &uy, &ua))
        return NULL;

    return PyInt_FromLong(playerc_bps_set_beacon(bpsob->bps, id, px, py, pa, ux, uy, ua));
}


// Assemble python bps type
static PyTypeObject bps_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "bps",
  sizeof(bps_object_t),
  0,
  bps_del,    //tp_dealloc
  0,          //tp_print
  bps_getattr, //tp_getattr
  0,          //tp_setattr
  0,          //tp_compare
  0,          //tp_repr
  0,          //tp_as_number
  0,          //tp_as_sequence
  0,          //tp_as_mapping
  0,          //tp_hash
};


static PyMethodDef bps_methods[] =
{
    {"setbeacon", bps_setbeacon, METH_VARARGS},
    {NULL, NULL}
};
*/


/***************************************************************************
 * broadcast device
 **************************************************************************/


/* Python wrapper for broadcast type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_comms_t *comms;
  PyObject *px, *py, *pa;
} comms_object_t;

staticforward PyTypeObject comms_type;
staticforward PyMethodDef comms_methods[];


static PyObject *comms_new(PyObject *self, PyObject *args)
{
  client_object_t *clientob;
  comms_object_t *commsob;
  int index;
  char access;

  if (!PyArg_ParseTuple(args, "Oic", &clientob, &index, &access))
    return NULL;

  commsob = PyObject_New(comms_object_t, &comms_type);
  commsob->client = clientob->client;
  commsob->comms = playerc_comms_create(clientob->client, index);

  return (PyObject*) commsob;
}


static void comms_del(PyObject *self)
{
  comms_object_t *commsob;
  commsob = (comms_object_t*) self;

  playerc_comms_destroy(commsob->comms);
  PyObject_Del(self);
}


static PyObject *comms_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  comms_object_t *commsob;

  commsob = (comms_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
    result = PyFloat_FromDouble(commsob->comms->info.datatime);
  else
    result = Py_FindMethod(comms_methods, self, attrname);

  return result;
}


/* Write a message
   Args: string
   Returns: none
*/
static PyObject *comms_write(PyObject *self, PyObject *args)
{
  comms_object_t *commsob;
  char *msg;
    
  if (!PyArg_ParseTuple(args, "s", &msg))
    return NULL;
  commsob = (comms_object_t*) self;

  thread_release();
  playerc_comms_send(commsob->comms, msg, strlen(msg));
  thread_acquire();

  Py_INCREF(Py_None);
  return Py_None;
}


/* Read a message
   Args: none
   Returns: message as a string
*/
static PyObject *comms_read(PyObject *self, PyObject *args)
{
  comms_object_t *commsob;
  int len;
  char *msg;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  commsob = (comms_object_t*) self;

  thread_release();
  len = commsob->comms->msg_len;
  msg = commsob->comms->msg;
  thread_acquire();

  if (len <= 0)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return PyString_FromStringAndSize(msg, len);
}


/* Assemble python comms type
 */
static PyTypeObject comms_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "comms",
  sizeof(comms_object_t),
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
  0,          /*tp_hash */
};


static PyMethodDef comms_methods[] =
{
  {"read", comms_read, METH_VARARGS},
  {"write", comms_write, METH_VARARGS},
  {NULL, NULL}
};



/***************************************************************************
 * module level stuff
 **************************************************************************/

extern PyTypeObject blobfinder_type;
extern PyObject *blobfinder_new(PyObject *self, PyObject *args);

extern PyTypeObject fiducial_type;
extern PyObject *fiducial_new(PyObject *self, PyObject *args);

extern PyTypeObject laser_type;
extern PyObject *laser_new(PyObject *self, PyObject *args);

extern PyTypeObject position_type;
extern PyObject *position_new(PyObject *self, PyObject *args);

extern PyTypeObject ptz_type;
extern PyObject *ptz_new(PyObject *self, PyObject *args);



static PyMethodDef module_methods[] =
{
  {"mclient", mclient_new, METH_VARARGS},
  {"client", client_new, METH_VARARGS},
  {"laser", laser_new, METH_VARARGS},
  {"position", position_new, METH_VARARGS},
  {"ptz", ptz_new, METH_VARARGS},
  {"blobfinder", blobfinder_new, METH_VARARGS},
  {"fiducial", fiducial_new, METH_VARARGS},
  {"gps", gps_new, METH_VARARGS},
//  {"bps", bps_new, METH_VARARGS},
  {"comms", comms_new, METH_VARARGS},
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
  ptz_type.ob_type = &PyType_Type;
  blobfinder_type.ob_type = &PyType_Type;
  fiducial_type.ob_type = &PyType_Type;
  gps_type.ob_type = &PyType_Type;
//  bps_type.ob_type = &PyType_Type;
    
  moduleob = Py_InitModule("playerc", module_methods);

  /* Add out exception */
  errorob = PyErr_NewException("playerc.error", NULL, NULL);
  PyDict_SetItemString(PyModule_GetDict(moduleob), "error", errorob);

  /* Make sure an interpreter lock object is created, since
     we will may try to release it at some point.
  */
  PyEval_InitThreads();
}

