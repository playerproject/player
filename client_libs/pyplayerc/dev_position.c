/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: Python bindings for position device
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


/* Python wrapper for position type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
  playerc_position_t *position;
  PyObject *px, *py, *pa;
  PyObject *vx, *vy, *va;
} position_object_t;


PyTypeObject position_type;
staticforward PyMethodDef position_methods[];

/* Local declarations */
static void position_onread(position_object_t *positionob);


/* Initialise (type function) */
PyObject *position_new(PyObject *self, PyObject *args)
{
  client_object_t *clientob;
  position_object_t *positionob;
  int index;

  if (!PyArg_ParseTuple(args, "Oi", &clientob, &index))
    return NULL;

  positionob = PyObject_New(position_object_t, &position_type);
  positionob->client = clientob->client;
  positionob->position = playerc_position_create(clientob->client, index);
  positionob->position->info.user_data = positionob;
  positionob->px = PyFloat_FromDouble(0);
  positionob->py = PyFloat_FromDouble(0);
  positionob->pa = PyFloat_FromDouble(0);

  /* Add callback for post-processing incoming data */
  playerc_client_addcallback(clientob->client, (playerc_device_t*) positionob->position,
                             (playerc_callback_fn_t) position_onread,
                             (void*) positionob);
    
  return (PyObject*) positionob;
}


/* Finailize (type function) */
static void position_del(PyObject *self)
{
  position_object_t *positionob;
  positionob = (position_object_t*) self;

  playerc_client_delcallback(positionob->client, (playerc_device_t*) positionob->position,
                             (playerc_callback_fn_t) position_onread,
                             (void*) positionob);    

  Py_DECREF(positionob->px);
  Py_DECREF(positionob->py);
  Py_DECREF(positionob->pa);
  playerc_position_destroy(positionob->position);
  PyObject_Del(self);
}


/* Get attributes (type function) */
static PyObject *position_getattr(PyObject *self, char *attrname)
{
    PyObject *result;
    position_object_t *positionob;

    positionob = (position_object_t*) self;

    result = NULL;
    if (strcmp(attrname, "datatime") == 0)
    {
        result = PyFloat_FromDouble(positionob->position->info.datatime);
    }
    else if (strcmp(attrname, "px") == 0)
    {
        Py_INCREF(positionob->px);
        result = positionob->px;
    }
    else if (strcmp(attrname, "py") == 0)
    {
        Py_INCREF(positionob->py);
        result = positionob->py;
    }
    else if (strcmp(attrname, "pa") == 0)
    {
        Py_INCREF(positionob->pa);
        result = positionob->pa;
    }
    else if (strcmp(attrname, "vx") == 0)
    {
        Py_INCREF(positionob->vx);
        result = positionob->vx;
    }
    else if (strcmp(attrname, "vy") == 0)
    {
        Py_INCREF(positionob->vy);
        result = positionob->vy;
    }
    else if (strcmp(attrname, "va") == 0)
    {
        Py_INCREF(positionob->va);
        result = positionob->va;
    }
    else if (strcmp(attrname, "stall") == 0)
    {
        result = PyInt_FromLong(positionob->position->stall);
    }
    else
        result = Py_FindMethod(position_methods, self, attrname);

    return result;
}


/* Get string representation (type function) */
static PyObject *position_str(PyObject *self)
{
  char str[128];
  position_object_t *positionob;
  positionob = (position_object_t*) self;


  snprintf(str, sizeof(str),
           "position %02d %013.3f"
           " %+07.3f %+07.3f %+04.3f"
           " %+04.3f %+04.3f %+04.3f",
           positionob->position->info.index,
           positionob->position->info.datatime,
           positionob->position->px,
           positionob->position->py,
           positionob->position->pa,
           positionob->position->vx,
           positionob->position->vy,
           positionob->position->va);
  return PyString_FromString(str);
}


/* Callback for post-processing incoming data */
static void position_onread(position_object_t *positionob)
{
    thread_acquire();
    
    Py_DECREF(positionob->px);
    Py_DECREF(positionob->py);
    Py_DECREF(positionob->pa);
    positionob->px = PyFloat_FromDouble(positionob->position->px);
    positionob->py = PyFloat_FromDouble(positionob->position->py);
    positionob->pa = PyFloat_FromDouble(positionob->position->pa);    
    positionob->vx = PyFloat_FromDouble(positionob->position->vx);
    positionob->vy = PyFloat_FromDouble(positionob->position->vy);
    positionob->va = PyFloat_FromDouble(positionob->position->va);    
    
    thread_release();
}


/* Subscribe to the device. */
static PyObject *position_subscribe(PyObject *self, PyObject *args)
{
  char access;
  position_object_t *positionob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "c", &access))
    return NULL;
  positionob = (position_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_position_subscribe(positionob->position, access));
  thread_acquire();

  return result;
}


/* Unsubscribe from the device. */
static PyObject *position_unsubscribe(PyObject *self, PyObject *args)
{
  position_object_t *positionob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  positionob = (position_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_position_unsubscribe(positionob->position));
  thread_acquire();

  return result;
}


/* Enable/disable motors
   Args: (enable)
   Returns: (0 on success)
*/
static PyObject *position_enable(PyObject *self, PyObject *args)
{
  int enable;
  position_object_t *positionob;
  PyObject *result;
    
  if (!PyArg_ParseTuple(args, "i", &enable))
    return NULL;
  positionob = (position_object_t*) self;

  thread_release();
  result = PyInt_FromLong(playerc_position_enable(positionob->position, enable));
  thread_acquire();
  
  return result;
}


/* Set the robot speed
   (vx, vy, va)
*/
static PyObject *position_set_speed(PyObject *self, PyObject *args)
{
    double vx, vy, va;
    position_object_t *positionob;
    
    if (!PyArg_ParseTuple(args, "ddd", &vx, &vy, &va))
        return NULL;
    positionob = (position_object_t*) self;

    return PyInt_FromLong(playerc_position_set_speed(positionob->position, vx, vy, va));
}


/* Assemble python position type
 */
PyTypeObject position_type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "position",
    sizeof(position_object_t),
    0,
    position_del, /*tp_dealloc*/
    0,          /*tp_print*/
    position_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,          /*tp_call*/
    position_str, /*tp_string*/
};


static PyMethodDef position_methods[] =
{
  {"subscribe", position_subscribe, METH_VARARGS},
  {"unsubscribe", position_unsubscribe, METH_VARARGS},  
  {"enable", position_enable, METH_VARARGS},
  {"set_speed", position_set_speed, METH_VARARGS},
  {NULL, NULL}
};

