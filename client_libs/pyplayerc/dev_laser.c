/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for laser device
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


/* Python wrapper for laser type */
typedef struct
{
    PyObject_HEAD
    playerc_client_t *client;
    playerc_laser_t *laser;
    int ignore;
    PyObject *scan;
} laser_object_t;

/* Local declarations */
static void laser_onread(laser_object_t *laserob);

PyTypeObject laser_type;
staticforward PyMethodDef laser_methods[];


PyObject *laser_new(PyObject *self, PyObject *args)
{
  client_object_t *clientob;
  laser_object_t *laserob;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &clientob, &index))
    return NULL;

  laserob = PyObject_New(laser_object_t, &laser_type);
  laserob->client = clientob->client;
  laserob->laser = playerc_laser_create(clientob->client, index);
  laserob->laser->info.user_data = laserob;
  laserob->ignore = 0;
  laserob->scan =  PyList_New(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(clientob->client, (playerc_device_t*) laserob->laser,
                             (playerc_callback_fn_t) laser_onread,
                             (void*) laserob);
    
  return (PyObject*) laserob;
}


static void laser_del(PyObject *self)
{
  laser_object_t *laserob;
  laserob = (laser_object_t*) self;

  playerc_client_delcallback(laserob->client, (playerc_device_t*) laserob->laser,
                             (playerc_callback_fn_t) laser_onread,
                             (void*) laserob);    
  Py_DECREF(laserob->scan);
  playerc_laser_destroy(laserob->laser);
  PyObject_Del(self);
}


static PyObject *laser_getattr(PyObject *self, char *attrname)
{
  PyObject *result;
  laser_object_t *laserob;

  laserob = (laser_object_t*) self;

  result = NULL;
  if (strcmp(attrname, "datatime") == 0)
  {
    result = PyFloat_FromDouble(laserob->laser->info.datatime);
  }
  else if (strcmp(attrname, "scan") == 0)
  {
    Py_INCREF(laserob->scan);
    result = laserob->scan;
  }
  else
    result = Py_FindMethod(laser_methods, self, attrname);

  return result;
}



/* Get string representation (type function) */
static PyObject *laser_str(PyObject *self)
{
  int i;
  char s[64];
  char str[8192];
  laser_object_t *laserob;
  laserob = (laser_object_t*) self;

  snprintf(str, sizeof(str),
           "laser %02d %013.3f ",
           laserob->laser->info.index,
           laserob->laser->info.datatime);

  for (i = 0; i < laserob->laser->scan_count; i++)
  {
    snprintf(s, sizeof(s), "%05.3f %+05.3f %d ",
             laserob->laser->scan[i][0],
             laserob->laser->scan[i][1],
             laserob->laser->intensity[i]);
    assert(strlen(str) + strlen(s) < sizeof(str));
    strcat(str, s);
  }
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void laser_onread(laser_object_t *laserob)
{
  int i;
  PyObject *tuple;

  if (laserob->ignore)
    return;

  thread_acquire();
    
  Py_DECREF(laserob->scan);
  laserob->scan = PyList_New(laserob->laser->scan_count);
    
  for (i = 0; i < laserob->laser->scan_count; i++)
  {
    tuple = PyTuple_New(5);
    PyTuple_SetItem(tuple, 0, PyFloat_FromDouble(laserob->laser->scan[i][0]));
    PyTuple_SetItem(tuple, 1, PyFloat_FromDouble(laserob->laser->scan[i][1]));
    PyTuple_SetItem(tuple, 2, PyFloat_FromDouble(laserob->laser->point[i][0]));
    PyTuple_SetItem(tuple, 3, PyFloat_FromDouble(laserob->laser->point[i][1]));
    PyTuple_SetItem(tuple, 4, PyInt_FromLong(laserob->laser->intensity[i]));
    PyList_SetItem(laserob->scan, i, tuple);
  }

  thread_release();
}


/* Subscribe to the device. */
static PyObject *laser_subscribe(PyObject *self, PyObject *args)
{
  char access;
  laser_object_t *laserob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  laserob = (laser_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_laser_subscribe(laserob->laser, access));
  thread_acquire();

  return result;
}


/* Unsubscribe from the device. */
static PyObject *laser_unsubscribe(PyObject *self, PyObject *args)
{
  laser_object_t *laserob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  laserob = (laser_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_laser_unsubscribe(laserob->laser));
  thread_acquire();

  return result;
}


/* Ignore laser data (just to save some CPU cycles)
   (ignore = [0, 1])
*/
static PyObject *laser_ignore(PyObject *self, PyObject *args)
{
  int ignore;
  laser_object_t *laserob;
    
  if (!PyArg_ParseTuple(args, "i", &ignore))
    return NULL;
  laserob = (laser_object_t*) self;

  laserob->ignore = ignore;

  Py_INCREF(Py_None);
  return Py_None;
}


/* Configure laser
   (min_angle, max_angle, resolution, intensity)
*/
static PyObject *laser_set_config(PyObject *self, PyObject *args)
{
  double min_angle, max_angle, resolution;
  int intensity;
  laser_object_t *laserob;
  int result;
    
  if (!PyArg_ParseTuple(args, "dddi", &min_angle, &max_angle, &resolution, &intensity))
    return NULL;
  laserob = (laser_object_t*) self;

  thread_release();
  result = playerc_laser_set_config(laserob->laser,
                                    min_angle, max_angle,
                                    resolution, intensity);
  thread_acquire();
  return PyInt_FromLong(result);
}


/* Assemble python laser type
 */
PyTypeObject laser_type = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "laser",
  sizeof(laser_object_t),
  0,
  laser_del, /*tp_dealloc*/
  0,          /*tp_print*/
  laser_getattr, /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  0,          /*tp_as_number*/
  0,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
  0,          /*tp_call*/
  laser_str,  /*tp_string*/
};


static PyMethodDef laser_methods[] =
{
  {"subscribe", laser_subscribe, METH_VARARGS},
  {"unsubscribe", laser_unsubscribe, METH_VARARGS},  
  {"ignore", laser_ignore, METH_VARARGS},
  {"set_config", laser_set_config, METH_VARARGS},
  {NULL, NULL}
};
