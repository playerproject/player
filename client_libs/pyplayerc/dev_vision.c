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
  playerc_vision_t *vision;
  PyObject *width, *height;
  PyObject *blobs;
} vision_object_t;

/* Local declarations */
static void vision_onread(vision_object_t *visionob);

PyTypeObject vision_type;
staticforward PyMethodDef vision_methods[];


PyObject *vision_new(PyObject *self, PyObject *args)
{
  client_object_t *clientob;
  vision_object_t *visionob;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &clientob, &index))
    return NULL;

  visionob = PyObject_New(vision_object_t, &vision_type);
  visionob->client = clientob->client;
  visionob->vision = playerc_vision_create(clientob->client, index);
  visionob->vision->info.user_data = visionob;
  visionob->width = PyInt_FromLong(0);
  visionob->height = PyInt_FromLong(0);
  visionob->blobs = PyList_New(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(clientob->client, (playerc_device_t*) visionob->vision,
                             (playerc_callback_fn_t) vision_onread,
                             (void*) visionob);
    
  return (PyObject*) visionob;
}


static void vision_del(PyObject *self)
{
  vision_object_t *visionob;
  visionob = (vision_object_t*) self;

  playerc_client_delcallback(visionob->client, (playerc_device_t*) visionob->vision,
                             (playerc_callback_fn_t) vision_onread,
                             (void*) visionob);    

  Py_DECREF(visionob->blobs);
  Py_DECREF(visionob->width);
  Py_DECREF(visionob->height);
  
  playerc_vision_destroy(visionob->vision);
  PyObject_Del(self);
}


static PyObject *vision_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  vision_object_t *visionob;

  visionob = (vision_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(visionob->vision->info.datatime);
  }
  else if (strcmp(attrname, "width") == 0)
  {
    Py_INCREF(visionob->width);
    result = visionob->width;
  }
  else if (strcmp(attrname, "height") == 0)
  {
    Py_INCREF(visionob->height);
    result = visionob->height;
  }
  else if (strcmp(attrname, "blobs") == 0)
  {
    Py_INCREF(visionob->blobs);
    result = visionob->blobs;
  }
  else
    result = Py_FindMethod(vision_methods, self, attrname);

  return result;
}



/* Get string representation (type function) */
static PyObject *vision_str(PyObject *self)
{
  int i;
  char s[64];
  char str[8192];
  playerc_vision_blob_t *blob;
  vision_object_t *visionob;
  
  visionob = (vision_object_t*) self;

  snprintf(str, sizeof(str),
           "vision %02d %013.3f ",
           visionob->vision->info.index,
           visionob->vision->info.datatime);

  for (i = 0; i < visionob->vision->blob_count; i++)
  {
    blob = visionob->vision->blobs + i;
    snprintf(s, sizeof(s), "%2d %3d %3d %3d [%3d %3d %3d %3d] ",
             blob->channel, blob->x, blob->y, blob->area,
             blob->left, blob->top, blob->right, blob->bottom);
             assert(strlen(str) + strlen(s) < sizeof(str));
    strcat(str, s);
  }
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void vision_onread(vision_object_t *visionob)
{
  int i;
  PyObject *tuple;
  playerc_vision_blob_t *blob;

  thread_acquire();

  Py_DECREF(visionob->width);
  Py_DECREF(visionob->height);
  visionob->width = PyInt_FromLong(visionob->vision->width);
  visionob->height = PyInt_FromLong(visionob->vision->height);
  
  Py_DECREF(visionob->blobs);
  visionob->blobs = PyList_New(visionob->vision->blob_count);
  for (i = 0; i < visionob->vision->blob_count; i++)
  {
    blob = visionob->vision->blobs + i;
    tuple = PyTuple_New(8);
    PyTuple_SetItem(tuple, 0, PyInt_FromLong(blob->channel));
    PyTuple_SetItem(tuple, 1, PyInt_FromLong(blob->x));
    PyTuple_SetItem(tuple, 2, PyInt_FromLong(blob->y));
    PyTuple_SetItem(tuple, 3, PyInt_FromLong(blob->area));
    PyTuple_SetItem(tuple, 4, PyInt_FromLong(blob->left));
    PyTuple_SetItem(tuple, 5, PyInt_FromLong(blob->top));
    PyTuple_SetItem(tuple, 6, PyInt_FromLong(blob->right));
    PyTuple_SetItem(tuple, 7, PyInt_FromLong(blob->bottom));
    PyList_SetItem(visionob->blobs, i, tuple);
  }

  thread_release();
}


/* Subscribe to the device. */
static PyObject *vision_subscribe(PyObject *self, PyObject *args)
{
  char access;
  vision_object_t *visionob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  visionob = (vision_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_vision_subscribe(visionob->vision, access));
  thread_acquire();

  return result;
}


/* Unsubscribe from the device. */
static PyObject *vision_unsubscribe(PyObject *self, PyObject *args)
{
  vision_object_t *visionob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  visionob = (vision_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_vision_unsubscribe(visionob->vision));
  thread_acquire();

  return result;
}


/* Assemble python vision type
 */
PyTypeObject vision_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "vision",
  sizeof(vision_object_t),
  0,
  vision_del, /*tp_dealloc*/
  0,          /*tp_print*/
  vision_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
  0,          /*tp_call*/
  vision_str,  /*tp_string*/
};


static PyMethodDef vision_methods[] =
{
  {"subscribe", vision_subscribe, METH_VARARGS},
  {"unsubscribe", vision_unsubscribe, METH_VARARGS},  
  {NULL, NULL}
};
