/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for vision device
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


/* Python wrapper for vision type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_blobfinder_t *blobfinder;
  PyObject *width, *height;
  PyObject *blobs;
} blobfinder_object_t;

/* Local declarations */
static void blobfinder_onread(blobfinder_object_t *blobfinderob);

PyTypeObject blobfinder_type;
staticforward PyMethodDef blobfinder_methods[];


PyObject *blobfinder_new(PyObject *self, PyObject *args)
{
  pyclient_t *pyclient;
  blobfinder_object_t *blobfinderob;
  int robot, index;

  if (!PyArg_ParseTuple(args, "Oii", &pyclient, &robot, &index))
    return NULL;

  blobfinderob = PyObject_New(blobfinder_object_t, &blobfinder_type);
  blobfinderob->client = pyclient->client;
  blobfinderob->blobfinder = playerc_blobfinder_create(pyclient->client, robot, index);
  blobfinderob->blobfinder->info.user_data = blobfinderob;
  blobfinderob->width = PyInt_FromLong(0);
  blobfinderob->height = PyInt_FromLong(0);
  blobfinderob->blobs = PyList_New(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(pyclient->client, (playerc_device_t*) blobfinderob->blobfinder,
                             (playerc_callback_fn_t) blobfinder_onread,
                             (void*) blobfinderob);
    
  return (PyObject*) blobfinderob;
}


static void blobfinder_del(PyObject *self)
{
  blobfinder_object_t *blobfinderob;
  blobfinderob = (blobfinder_object_t*) self;

  playerc_client_delcallback(blobfinderob->client, (playerc_device_t*) blobfinderob->blobfinder,
                             (playerc_callback_fn_t) blobfinder_onread,
                             (void*) blobfinderob);    

  Py_DECREF(blobfinderob->blobs);
  Py_DECREF(blobfinderob->width);
  Py_DECREF(blobfinderob->height);
  
  playerc_blobfinder_destroy(blobfinderob->blobfinder);
  PyObject_Del(self);
}


static PyObject *blobfinder_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  blobfinder_object_t *blobfinderob;

  blobfinderob = (blobfinder_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(blobfinderob->blobfinder->info.datatime);
  }
  else if (strcmp(attrname, "width") == 0)
  {
    Py_INCREF(blobfinderob->width);
    result = blobfinderob->width;
  }
  else if (strcmp(attrname, "height") == 0)
  {
    Py_INCREF(blobfinderob->height);
    result = blobfinderob->height;
  }
  else if (strcmp(attrname, "blobs") == 0)
  {
    Py_INCREF(blobfinderob->blobs);
    result = blobfinderob->blobs;
  }
  else
    result = Py_FindMethod(blobfinder_methods, self, attrname);

  return result;
}



/* Get string representation (type function) */
static PyObject *blobfinder_str(PyObject *self)
{
  int i;
  char s[64];
  char str[8192];
  playerc_blobfinder_blob_t *blob;
  blobfinder_object_t *blobfinderob;
  
  blobfinderob = (blobfinder_object_t*) self;

  snprintf(str, sizeof(str),
           "blobfinder %02d %013.3f ",
           blobfinderob->blobfinder->info.index,
           blobfinderob->blobfinder->info.datatime);

  for (i = 0; i < blobfinderob->blobfinder->blob_count; i++)
  {
    blob = blobfinderob->blobfinder->blobs + i;
    snprintf(s, sizeof(s), "%2d %3d %3d %3d [%3d %3d %3d %3d] ",
             blob->channel, blob->x, blob->y, blob->area,
             blob->left, blob->top, blob->right, blob->bottom);
             assert(strlen(str) + strlen(s) < sizeof(str));
    strcat(str, s);
  }
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void blobfinder_onread(blobfinder_object_t *blobfinderob)
{
  int i;
  PyObject *tuple;
  playerc_blobfinder_blob_t *blob;

  thread_acquire();

  Py_DECREF(blobfinderob->width);
  Py_DECREF(blobfinderob->height);
  blobfinderob->width = PyInt_FromLong(blobfinderob->blobfinder->width);
  blobfinderob->height = PyInt_FromLong(blobfinderob->blobfinder->height);
  
  Py_DECREF(blobfinderob->blobs);
  blobfinderob->blobs = PyList_New(blobfinderob->blobfinder->blob_count);
  for (i = 0; i < blobfinderob->blobfinder->blob_count; i++)
  {
    blob = blobfinderob->blobfinder->blobs + i;
    tuple = PyTuple_New(8);
    PyTuple_SetItem(tuple, 0, PyInt_FromLong(blob->channel));
    PyTuple_SetItem(tuple, 1, PyInt_FromLong(blob->x));
    PyTuple_SetItem(tuple, 2, PyInt_FromLong(blob->y));
    PyTuple_SetItem(tuple, 3, PyInt_FromLong(blob->area));
    PyTuple_SetItem(tuple, 4, PyInt_FromLong(blob->left));
    PyTuple_SetItem(tuple, 5, PyInt_FromLong(blob->top));
    PyTuple_SetItem(tuple, 6, PyInt_FromLong(blob->right));
    PyTuple_SetItem(tuple, 7, PyInt_FromLong(blob->bottom));
    PyList_SetItem(blobfinderob->blobs, i, tuple);
  }

  thread_release();
}


/* Subscribe to the device. */
static PyObject *blobfinder_subscribe(PyObject *self, PyObject *args)
{
  char access;
  blobfinder_object_t *blobfinderob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  blobfinderob = (blobfinder_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_blobfinder_subscribe(blobfinderob->blobfinder, access));
  thread_acquire();

  return result;
}


/* Unsubscribe from the device. */
static PyObject *blobfinder_unsubscribe(PyObject *self, PyObject *args)
{
  blobfinder_object_t *blobfinderob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  blobfinderob = (blobfinder_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_blobfinder_unsubscribe(blobfinderob->blobfinder));
  thread_acquire();

  return result;
}


/* Assemble python blobfinder type
 */
PyTypeObject blobfinder_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "blobfinder",
  sizeof(blobfinder_object_t),
  0,
  blobfinder_del, /*tp_dealloc*/
  0,          /*tp_print*/
  blobfinder_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
  0,          /*tp_call*/
  blobfinder_str,  /*tp_string*/
};


static PyMethodDef blobfinder_methods[] =
{
  {"subscribe", blobfinder_subscribe, METH_VARARGS},
  {"unsubscribe", blobfinder_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};
