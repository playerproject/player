/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Andrew Howard
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

%module playerc

%{
#include "playerc_wrap.h"

#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif
%}

%include "typemaps.i"

// Provide array access to the RFID tags
// We will return a list of tuples. Each tuple contains the RFID type, and the RFID ID.
//  The RFID ID is a Python string containing the unprocessed bytes of the RFID tag.
//  In Python, use   rfid.tags[i][1].encode('hex')   to see the regular Hex-ASCII representation.
%typemap(out) playerc_rfidtag_t [ANY]
{
  int i;
  $result = PyList_New($1_dim0);  //we will return a Python List

  //At this level, we don't get the ammount of tags, so we just produce a long list, of size PLAYER_RFID_MAX_TAGS
  //(this is the size of the array of playerc_rfidtag_t contained in the RFID data packet)
  for (i = 0; i != $1_dim0; ++i)
  {

    PyObject *tuple = PyTuple_New(2); // we will make a tuple with 2 fields: type(int) and RFID ID(string)

    const unsigned int blength=PLAYERC_RFID_MAX_GUID;
    char buffer[blength];
    memset(buffer, 0, blength );

    //result is of type playerc_rfidtag_t
    unsigned int j;    
    unsigned int guid_count=$1[i].guid_count;

    //copy the bytes into the buffer
    for (j=0 ; j != guid_count ; ++j) {
	buffer[j]=$1[i].guid[j];
    }

    //generate a Python string from the buffer
    PyObject *ostring = PyString_FromStringAndSize(buffer,guid_count);

    //generate an Int from the tag type
    PyObject *otype = PyInt_FromLong($1[i].type);

    //set the previous objects into the tuple
    PyTuple_SetItem(tuple,0,otype);
    PyTuple_SetItem(tuple,1,ostring);

    //set the tupple into the corresponding place of the list
    PyList_SetItem($result,i,tuple);
  }

  //$result is the Python List that gets returned automatically at the end of this function
}


// For playerc_simulation_get_pose2d()
%apply double *OUTPUT { double *x, double *y, double *a };

// Provide array (write) access
%typemap(in) double [ANY] (double temp[$1_dim0])
{
  int i;
  if (!PySequence_Check($input))
  {
    PyErr_SetString(PyExc_ValueError,"Expected a sequence");
    return NULL;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected $1_dim0 elements");
    return NULL;
  }
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyNumber_Check(o))
    {
      temp[i] = (float) PyFloat_AsDouble(o);    }
    else
    {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
      return NULL;
    }
  }
  $1 = temp;
}

%typemap(in) float [ANY] (float temp[$1_dim0])
{
  int i;
  if (!PySequence_Check($input))
  {
    PyErr_SetString(PyExc_ValueError,"Expected a sequence");
    return NULL;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected $1_dim0 elements");
    return NULL;
  }
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyNumber_Check(o))
    {
      temp[i] = (float) PyFloat_AsDouble(o);    }
    else
    {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
      return NULL;
    }
  }
  $1 = temp;
}

%typemap(in) uint8_t data[]
{
	int temp = 0;
	// Check if is a list
			if (PyList_Check ($input))
	{
		int size = PyList_Size ($input);
		int ii = 0;
		$1 = (uint8_t*) malloc (size * sizeof (uint8_t));
		for (ii = 0; ii < size; ii++)
		{
			PyObject *o = PyList_GetItem ($input, ii);
			temp = PyInt_AsLong (o);
			if (temp == -1 && PyErr_Occurred ())
			{
				free ($1);
				return NULL;
			}
			$1[ii] = (uint8_t) temp;
		}
	}
	else
	{
		PyErr_SetString (PyExc_TypeError, "not a list");
		return NULL;
	}
}

/*// typemap to free the array created in the previous typemap
%typemap(freearg) uint8_t data[]
{
	if ($input) free ((uint8_t*) $input);
}*/

// typemap for passing points into the graphics2d interface
%typemap(in) player_point_2d_t pts[]
{
	// Check if is a list
	if (PyList_Check ($input))
	{
		int size = PyList_Size ($input);
		int ii = 0;
		$1 = (player_point_2d_t*) malloc (size * sizeof (player_point_2d_t));
		for (ii = 0; ii < size; ii++)
		{
			PyObject *o = PyList_GetItem ($input, ii);
			if (PyTuple_Check (o))
			{
				if (PyTuple_GET_SIZE (o) != 2)
				{
					PyErr_SetString (PyExc_ValueError, "tuples must have 2 items");
					free ($1);
					return NULL;
				}
				$1[ii].px = PyFloat_AsDouble (PyTuple_GET_ITEM (o, 0));
				$1[ii].py = PyFloat_AsDouble (PyTuple_GET_ITEM (o, 1));
			}
			else
			{
				PyErr_SetString (PyExc_TypeError, "list must contain tuples");
				free ($1);
				return NULL;
			}
		}
	}
	else
	{
		PyErr_SetString (PyExc_TypeError, "not a list");
		return NULL;
	}
}

// typemap for passing 3d points into the graphics3d interface
%typemap(in) player_point_3d_t pts[]
{
	// Check if is a list
	if (PyList_Check ($input))
	{
		int size = PyList_Size ($input);
		int ii = 0;
		$1 = (player_point_3d_t*) malloc (size * sizeof (player_point_3d_t));
		for (ii = 0; ii < size; ii++)
		{
			PyObject *o = PyList_GetItem ($input, ii);
			if (PyTuple_Check (o))
			{
				if (PyTuple_GET_SIZE (o) != 3)
				{
					PyErr_SetString (PyExc_ValueError, "tuples must have 3 items");
					free ($1);
					return NULL;
				}
				$1[ii].px = PyFloat_AsDouble (PyTuple_GET_ITEM (o, 0));
				$1[ii].py = PyFloat_AsDouble (PyTuple_GET_ITEM (o, 1));
				$1[ii].pz = PyFloat_AsDouble (PyTuple_GET_ITEM (o, 2));
			}
			else
			{
				PyErr_SetString (PyExc_TypeError, "list must contain tuples");
				free ($1);
				return NULL;
			}
		}
	}
	else
	{
		PyErr_SetString (PyExc_TypeError, "not a list");
		return NULL;
	}
}

// typemap to free the array created in the previous typemap
%typemap(freearg) player_point2d_t pts[]
{
	if ($input) free ((player_point2d_t*) $input);
}

// typemap for tuples to colours
%typemap(in) player_color_t (player_color_t temp)
{
	// Check it is a tuple
	if (PyTuple_Check ($input))
	{
		// Check the tuple has four elements
		if (PyTuple_GET_SIZE ($input) != 4)
		{
			PyErr_SetString (PyExc_ValueError, "tuple must have 4 items");
			return NULL;
		}
		temp.alpha = PyInt_AsLong (PyTuple_GET_ITEM ($input, 0));
		temp.red = PyInt_AsLong (PyTuple_GET_ITEM ($input, 1));
		temp.green = PyInt_AsLong (PyTuple_GET_ITEM ($input, 2));
		temp.blue = PyInt_AsLong (PyTuple_GET_ITEM ($input, 3));
	}
	else
	{
		PyErr_SetString (PyExc_TypeError, "not a tuple");
		return NULL;
	}
}

// Provide array (write) access
%typemap(in) double [ANY][ANY] (double temp[$1_dim0][$1_dim1])
{
  int i,j;
  if (!PySequence_Check($input))
  {
    PyErr_SetString(PyExc_ValueError,"Expected a sequence");
    return NULL;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected $1_dim0 elements");
    return NULL;
  }
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *olist = PySequence_GetItem($input,i);
    if (!PySequence_Check(olist))
    {
      PyErr_SetString(PyExc_ValueError,"Expected a sequence");
      return NULL;
    }
    if (PySequence_Length(olist) != $1_dim1) {
      PyErr_SetString(PyExc_ValueError,"Size mismatch. Expected $1_dim1 elements");
      return NULL;
    }

    for (j = 0; j < $1_dim1; j++)
    {
      PyObject *o = PySequence_GetItem(olist,j);

      if (PyNumber_Check(o))
      {
        temp[i][j] = (float) PyFloat_AsDouble(o);
      }
      else
      {
        PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
        return NULL;
      }
    }
  }
  $1 = temp;
}

%typemap(in) uint8_t
{
  $1 = (uint8_t) PyLong_AsLong($input);
}

%typemap(in) uint32_t
{
  $1 = (uint32_t) PyLong_AsLong ($input);
}

// Integer types
%typemap(out) uint8_t
{
  $result = PyInt_FromLong((long) (unsigned char) $1);
}

%typemap(out) uint16_t
{
  $result = PyInt_FromLong((long) (unsigned long) $1);
}

%typemap(out) uint32_t
{
  $result = PyInt_FromLong((long) (unsigned long long) $1);
}

// Provide array access
%typemap(out) double [ANY]
{
  int i;
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *o = PyFloat_FromDouble((double) $1[i]);
    PyList_SetItem($result,i,o);
  }
}

%typemap(out) float [ANY]
{
  int i;
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *o = PyFloat_FromDouble((double) $1[i]);
    PyList_SetItem($result,i,o);
  }
}

%typemap(out) uint8_t [ANY]
{
  $result = PyBuffer_FromMemory((void*)$1,sizeof(uint8_t)*$1_dim0);
}

// Provide array access doubly-dimensioned arrays
%typemap(out) double [ANY][ANY]
{
  int i, j;
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *l = PyList_New($1_dim1);
    for (j = 0; j < $1_dim1; j++)
    {
      PyObject *o = PyFloat_FromDouble((double) $1[i][j]);
      PyList_SetItem(l,j,o);
    }
    PyList_SetItem($result,i,l);
  }
}

// Catch-all rule to converts arrays of structures to tuples of proxies for
// the underlying structs
%typemap(out) SWIGTYPE [ANY]
{
 int i;
  $result = PyTuple_New($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    PyObject *o = SWIG_NewPointerObj($1 + i, $1_descriptor, 0);
    PyTuple_SetItem($result,i,o);
  }
}

// Provide thread-support on some functions
%exception read
{
  Py_BEGIN_ALLOW_THREADS
  $action
  Py_END_ALLOW_THREADS
}


// Include Player header so we can pick up some constants and generate
// wrapper code for structs
%include "libplayerinterface/player.h"
%include "libplayerinterface/player_interfaces.h"


// Use this for regular c-bindings;
// e.g. playerc_client_connect(client, ...)
//%include "../../playerc.h"


// Use this for object-oriented bindings;
// e.g., client.connect(...)
// This file is created by running ../parse_header.py
%include "playerc_wrap.i"

%extend playerc_blackboard
{

#define DICT_GROUPS_INDEX 0
#define DICT_SUBSCRIPTION_DATA_INDEX 1
#define LIST_EVENTS_INDEX 2
#define BOOL_QUEUE_EVENTS_INDEX 3

// Helper function to convert a c blackboard_entry_t into a python dictionary object.
// Returns a new reference to a dictionary object.
PyObject *__convert_blackboard_entry__(player_blackboard_entry_t *entry)
{
  PyObject *entry_dict, *data, *temp_pystring;
  char* str;
  int i;
  double d;
  int ok;

  entry_dict = PyDict_New(); // New reference
  assert (entry_dict);

  assert(entry);
  assert(entry->key);
  assert(entry->key_count > 0);
  assert(entry->group);
  assert(entry->group_count > 0);


temp_pystring = PyString_FromString(entry->key);
  ok = PyDict_SetItemString(entry_dict, "key", temp_pystring); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'key'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(temp_pystring);
  	return NULL;
  }
Py_XDECREF(temp_pystring);


temp_pystring = PyString_FromString(entry->group);
  ok = PyDict_SetItemString(entry_dict, "group", temp_pystring); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'group'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(temp_pystring);
  	return NULL;
  }
Py_XDECREF(temp_pystring);


temp_pystring = PyLong_FromLong(entry->type);
  ok = PyDict_SetItemString(entry_dict, "type", temp_pystring); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'type'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(temp_pystring);
  	return NULL;
  }
Py_XDECREF(temp_pystring);


temp_pystring = PyLong_FromLong(entry->subtype);
  ok = PyDict_SetItemString(entry_dict, "subtype", temp_pystring); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'subtype'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(temp_pystring);
  	return NULL;
  }
Py_XDECREF(temp_pystring);


temp_pystring = PyLong_FromLong(entry->timestamp_sec);
  ok = PyDict_SetItemString(entry_dict, "timestamp_sec", temp_pystring); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'timestamp_sec'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(temp_pystring);
  	return NULL;
  }
Py_XDECREF(temp_pystring);


temp_pystring = PyLong_FromLong(entry->timestamp_usec);
  ok = PyDict_SetItemString(entry_dict, "timestamp_usec", temp_pystring); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'timestamp_usec'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(temp_pystring);
  	return NULL;
  }
Py_XDECREF(temp_pystring);


  switch(entry->subtype)
  {
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_NONE:
      data = Py_None;
      Py_INCREF(data);
    break;
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_STRING:
      assert(entry->type == PLAYERC_BLACKBOARD_DATA_TYPE_COMPLEX);
      str = malloc(entry->data_count);
      if (!str)
      {
  	    Py_XDECREF(entry_dict);
  	    return PyErr_NoMemory();
      }
      memcpy(str, entry->data, entry->data_count);
      data = PyString_FromString(str);
      free(str);
    break;
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_INT:
      assert(entry->type == PLAYERC_BLACKBOARD_DATA_TYPE_SIMPLE);
      i = 0;
      memcpy(&i, entry->data, entry->data_count);
      data = PyLong_FromLong(i);
    break;
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_DOUBLE:
      assert(entry->type == PLAYERC_BLACKBOARD_DATA_TYPE_SIMPLE);
      d = 0.0;
      memcpy(&d, entry->data, entry->data_count);
      data = PyFloat_FromDouble(d);
    break;
    default:
      data = Py_None;
      Py_INCREF(data);
    break;
  }
  
  ok = PyDict_SetItemString(entry_dict, "data", data); // Steals reference
  if (ok != 0)
  {
  	PyErr_SetString(PyExc_RuntimeError, "Could not set dictionary value for 'data'");
  	Py_XDECREF(entry_dict);
  	Py_XDECREF(data);
  	return NULL;
  }
  Py_XDECREF(data);
  
  return entry_dict;
}

// Helper function which sets a value in a nested dictionary. Equivalent of dict['group']['key'] = value.
// Steals the entry reference.
PyObject *__set_nested_dictionary_entry__(PyObject *dict, const char* key, const char* group, PyObject *entry)
{
  PyObject *group_dict;
  int ok;
  int create_dict = 0;

  // Check we got valid arguments.
  if (!PyDict_Check(dict))
  {
    PyErr_SetString(PyExc_TypeError, "Expected type 'dict' for first argument (dict)");
    return NULL;
  }

  if (!entry)
  {
    PyErr_SetString(PyExc_TypeError, "Expected type non NULL for fourth argument (entry)");
    return NULL;
  }

  if (key == NULL || strlen(key) == 0)
  {
    PyErr_SetString(PyExc_TypeError, "Expected type 'string' for second argument (key)");
    return NULL;
  }

  if (group == NULL || strlen(group) == 0)
  {
    PyErr_SetString(PyExc_TypeError, "Expected type 'string' for second argument (group)");
    return NULL;
  }

  // Get the dictionary for the entry's group. If it doesn't exist, create it.
  group_dict = (PyObject*)PyDict_GetItemString(dict, group); // Borrowed reference
  if (!group_dict)
  {
    create_dict = 1;
    group_dict = PyDict_New(); // New reference    
  }

  ok = PyDict_SetItemString(group_dict, key, entry); // Steals reference
  if (ok != 0)
  {
    PyErr_SetString(PyExc_RuntimeError, "Failed to set dictionary entry");
    if (create_dict)
    {
    	Py_XDECREF(group_dict);
    }
    return NULL;
  }

  // If we created the dictionary, we need to put it in the parent dictionary.
  if (create_dict == 1)
  {
    ok = PyDict_SetItemString(dict, group, group_dict); // Steals reference
    if (ok != 0)
    {
    	PyErr_SetString(PyExc_RuntimeError, "Failed to set dictionary entry");
    	Py_XDECREF(group_dict);
    	return NULL;
    }
  }

  Py_RETURN_NONE;
}

// Helper function used to keep track to the number of subscriptions a group/key combination has
int __increment_reference_count__(PyObject *dict, const char* key, const char* group, int inc)
{
  PyObject *group_dict, *entry;
  int old_value, new_value;

  // Get the dictionary for the entry's group. If it doesn't exist.
  group_dict = (PyObject*)PyDict_GetItemString(dict, group); // Borrowed reference
  if (!group_dict)
  {
    playerc_blackboard___set_nested_dictionary_entry__(self, dict, key, group, (PyObject*)PyInt_FromLong(inc)); // Steals reference to entry
    return inc;
  }

  entry = PyDict_GetItemString(group_dict, key); // Borrowed reference
  if (entry)
  {
  	old_value = PyLong_AsLong(entry);
  }
  else
  {
  	old_value = 0;
  }
  
  new_value = old_value + inc;
  if (new_value < 0)
  {
  	new_value = 0;
  }

  playerc_blackboard___set_nested_dictionary_entry__(self, dict, key, group, PyInt_FromLong(new_value)); // Steals reference to entry

  return new_value;
}

// Replacement for the existing on_blackboard_event method. The python implementation uses this instead.
static void __python_on_blackboard_event__(playerc_blackboard_t *device, player_blackboard_entry_t entry)
{
  PyObject *entry_dict, *groups_dict, *list, *queue_events;

  // Get the main groups dictionary
  assert(device->py_private);
  groups_dict = (PyObject*)PyTuple_GetItem(device->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(groups_dict);

  // Get a dictionary of the entry
  entry_dict = playerc_blackboard___convert_blackboard_entry__(device, &entry); // New reference
  assert(entry_dict);
  
  // If we are queueing events, add them to the list
  queue_events = PyTuple_GetItem(device->py_private, BOOL_QUEUE_EVENTS_INDEX); // Borrowed reference
  assert(queue_events);
  if (PyLong_AsLong(queue_events) != 0)
  {
    list = (PyObject*)PyTuple_GetItem(device->py_private, LIST_EVENTS_INDEX); // Borrowed reference
    assert(list);
    PyList_Append(list, entry_dict); // Increments refcount for us
  }
  
  // Set the entry in the groups dictionary
  playerc_blackboard___set_nested_dictionary_entry__(device, groups_dict, entry.key, entry.group, entry_dict); // Steals reference to entry_dict
}

// Returns a list of dictionary objects containing the entries.
// Requires that set_queue_events is set to True for any events to be queued.
// All events will be retrieved.
// Will always return a list object.
%feature("autodoc", "1");
PyObject *GetEvents()
{
  PyObject *list, *copy, *item;
  int i, j, ok;

  assert(self->py_private);
  list = (PyObject*)PyTuple_GetItem(self->py_private, LIST_EVENTS_INDEX); // Borrowed reference
  assert(list);

  j = PyList_Size(list);
  copy = PyList_New(j); // New reference
  
  for (i = 0; i < j; i++)
  {
  	item = PyList_GetItem(list, 0); // Borrowed reference
  	Py_INCREF(item);
  	assert(item);
    ok = PyList_SetItem(copy, i, item); // Steals reference
    if (ok != 0)
    {
    	PyErr_SetString(PyExc_RuntimeError, "Failed to set list entry");
    	Py_XDECREF(copy);
    	Py_XDECREF(item);
    	return NULL;
    }
    PySequence_DelItem(list,0);
  }

  return copy;
}

// Returns a dictionary of the entry dictionary objects indexed by group.
// Will only contain the latest values.
// Will always return a dictionary object.
PyObject *GetDict()
{
  PyObject *dict, *copy;

  assert(self->py_private);
  dict = (PyObject*)PyTuple_GetItem(self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(dict);

  copy = PyDict_Copy(dict); // New reference
  return copy;
}

// Set whether events should be put into the list, to be retrieved by get_events_list().
PyObject *SetQueueEvents(PyObject *boolean)
{
  int ok;

  if(!PyBool_Check(boolean))
  {
    PyErr_SetString(PyExc_TypeError, "Expected type 'bool'");
    return NULL;
  }
  if (boolean == Py_False)
  {
    ok = PyTuple_SetItem((PyObject*)self->py_private, BOOL_QUEUE_EVENTS_INDEX, PyLong_FromLong(0)); // Steals reference
    if (ok != 0)
    {
      PyErr_SetString(PyExc_RuntimeError, "Failed to set tuple entry");
      return NULL;
    }
  }
  else
  {
    ok = PyTuple_SetItem((PyObject*)self->py_private, BOOL_QUEUE_EVENTS_INDEX, PyLong_FromLong(1)); // Steals reference
    if (ok != 0)
    {
      PyErr_SetString(PyExc_RuntimeError, "Failed to set tuple entry");
      return NULL;
    }
  }

  Py_RETURN_NONE;
}

PyObject *Subscribe(int access)
{
	int result;
	result = playerc_blackboard_subscribe(self, access);
	return PyInt_FromLong(result);
}

PyObject *Unsubscribe()
{
	int result;
	result = playerc_blackboard_unsubscribe(self);
	return PyInt_FromLong(result);
}

PyObject *SetString(const char* key, const char* group, const char* value)
{
  int result;
  result = playerc_blackboard_set_string(self, key, group, value);
  return PyInt_FromLong(result);
}

PyObject *SetInt(const char* key, const char* group, const int value)
{
  int result;
  result = playerc_blackboard_set_int(self, key, group, value);
  return PyInt_FromLong(result);
}

PyObject *SetDouble(const char* key, const char* group, const double value)
{
  int result;
  result = playerc_blackboard_set_double(self, key, group, value);
  return PyInt_FromLong(result);
}

int UnsubscribeFromKey(const char *key, const char *group)
{
  PyObject *groups_dict, *group_dict, *subscription_data;
  int result, ref_count;

  subscription_data = PyTuple_GetItem((PyObject*)self->py_private, DICT_SUBSCRIPTION_DATA_INDEX); // Borrowed reference
  assert(subscription_data);
  ref_count = playerc_blackboard___increment_reference_count__(self, subscription_data, key, group, -1);
  if (ref_count <= 0)
  {
    groups_dict = PyTuple_GetItem((PyObject*)self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
    assert(groups_dict);
    group_dict = PyDict_GetItemString(groups_dict, group); // Borrowed reference
    assert(group_dict);
    PyDict_DelItemString(group_dict, key);
  }

  result = playerc_blackboard_unsubscribe_from_key(self, key, group);
  return result;
}

PyObject *SubscribeToKey(const char *key, const char *group)
{
  int result;
  PyObject *subscription_data, *groups_dict, *entry_dict;
  player_blackboard_entry_t *entry;

  result = playerc_blackboard_subscribe_to_key(self, key, group, &entry);
  if (result != 0)
  {
    PyErr_SetString(PyExc_RuntimeError, "Failed to subscribe to key");
    return NULL;
  }

  entry_dict = playerc_blackboard___convert_blackboard_entry__(self, entry); // New reference
  assert(entry_dict);
  groups_dict = (PyObject*)PyTuple_GetItem((PyObject*)self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(groups_dict);

  Py_INCREF(entry_dict);
  playerc_blackboard___set_nested_dictionary_entry__(self, groups_dict, key, group, entry_dict); // Steals reference to entry_dict

  subscription_data = PyTuple_GetItem((PyObject*)self->py_private, DICT_SUBSCRIPTION_DATA_INDEX); // Borrowed reference
  assert(subscription_data);

  playerc_blackboard___increment_reference_count__(self, subscription_data, key, group, 1);

  free(entry->key);
  free(entry->group);
  free(entry->data);
  free(entry);

  return entry_dict;
}

int UnsubscribeFromGroup(const char *group)
{
  int i, j, ref_count;
  PyObject *groups_dict, *group_dict, *list, *tuple, *subscription_data, *items, *item;
  const char* key;
  int result;

  result = playerc_blackboard_unsubscribe_from_group(self, group);

  assert(self->py_private);
  groups_dict = PyTuple_GetItem((PyObject*)self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(groups_dict);

  group_dict = PyDict_GetItemString(groups_dict, group); // Borrowed reference
  if (group_dict)
  {
    list = PyList_New(0); // New reference
    subscription_data = PyTuple_GetItem((PyObject*)self->py_private, DICT_SUBSCRIPTION_DATA_INDEX); // Borrowed reference
    assert(subscription_data);

    items = PyDict_Items(group_dict);
    j = PyList_Size(items);
    for (i = 0; i < j; i++)
    {
      tuple = PyList_GetItem(items, 0); // Borrowed reference
      assert(tuple);
      item = PyTuple_GetItem(tuple, 0); // Borrowed reference
      assert(item);
      key = PyString_AsString(item);
      ref_count = playerc_blackboard___increment_reference_count__(self, subscription_data, key, group, 0);
      if (ref_count <= 0)
      {  
        PyList_Append(list, item); // Increments refcount for us
      }
    }

    j = PyList_Size(list);
    for (i =0; i < j; i++)
    {
      item = PyList_GetItem(list, i); // Borrowed reference
      assert(item);
      PyDict_DelItem(group_dict, item);
    }

    Py_DECREF(list);
  }

  return result;
}

int SubscribeToGroup(const char *group)
{
  int result;
  result = playerc_blackboard_subscribe_to_group(self, group);
  return result;
}

PyObject *GetEntry(const char *key, const char *group)
{
  int result;
  PyObject *entry_dict, *groups_dict;
  player_blackboard_entry_t *entry;
  
  result = playerc_blackboard_get_entry(self, key, group, &entry);
  if (result != 0)
  {
    PyErr_SetString(PyExc_RuntimeError, "Failed to get entry");
    return NULL;
  }
  
  entry_dict = playerc_blackboard___convert_blackboard_entry__(self, entry); // New reference
  
  groups_dict = PyTuple_GetItem((PyObject*)self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(groups_dict);
  assert(entry_dict);
  //Py_INCREF(entry_dict);
  playerc_blackboard___set_nested_dictionary_entry__(self, groups_dict, key, group, entry_dict); // Steals reference to entry_dict
  
  player_blackboard_entry_t_free(entry);

  return entry_dict;
}

PyObject *SetEntryRaw(player_blackboard_entry_t *entry)
{
  int result;
  PyObject *entry_dict, *groups_dict;
  
  entry_dict = playerc_blackboard___convert_blackboard_entry__(self, entry); // New reference
  assert(entry_dict);
  groups_dict = PyTuple_GetItem((PyObject*)self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(groups_dict);

  playerc_blackboard___set_nested_dictionary_entry__(self, groups_dict, entry->key, entry->group, entry_dict); // Steals reference to entry_dict
  result = playerc_blackboard_set_entry(self, entry);
  return PyInt_FromLong(result);
}

PyObject *SetEntry(PyObject *dict)
{
  PyObject *key, *group, *type, *subtype, *timestamp_sec, *timestamp_usec, *data, *groups_dict;
  char *str;
  int i, result, length;
  double d;

  if (!PyDict_Check(dict))
  {
    PyErr_SetString(PyExc_RuntimeError, "Expected a dictionary object.");
    return NULL;
  }

  player_blackboard_entry_t entry;
  memset(&entry, 0, sizeof(entry));

  key = PyDict_GetItem(dict, PyString_FromString("key")); // Borrowed reference
  group = PyDict_GetItem(dict, PyString_FromString("group")); // Borrowed reference
  type = PyDict_GetItem(dict, PyString_FromString("type")); // Borrowed reference
  subtype = PyDict_GetItem(dict, PyString_FromString("subtype")); // Borrowed reference
  timestamp_sec = PyDict_GetItem(dict, PyString_FromString("timestamp_sec")); // Borrowed reference
  timestamp_usec = PyDict_GetItem(dict, PyString_FromString("timestamp_usec")); // Borrowed reference
  data = PyDict_GetItem(dict, PyString_FromString("data")); // Borrowed reference

  if (key == NULL || group == NULL || type == NULL || subtype == NULL || timestamp_sec == NULL || timestamp_usec == NULL || data == NULL)
  {
    PyErr_SetString(PyExc_RuntimeError, "Dictionary object missing keys. One or more of the following keys were not found: 'key', 'group', 'type', 'subtype', 'timestamp_sec', 'timestamp_usec', 'data'.");
    return NULL;
  }

  if (!PyString_Check(key))
  {
    PyErr_SetString(PyExc_TypeError, "'key' should be a 'string' type.");
    return NULL;
  }

  if (!PyString_Check(group))
  {
    PyErr_SetString(PyExc_TypeError, "'group' should be a 'string' type.");
    return NULL;
  }

  if (!PyLong_Check(type))
  {
    PyErr_SetString(PyExc_TypeError, "'type' should be a 'long' type.");
    return NULL;
  }

  if (!PyLong_Check(subtype))
  {
    PyErr_SetString(PyExc_TypeError, "'subtype' should be a 'long' type.");
    return PyLong_FromLong(-1);
  }

  if (!PyLong_Check(timestamp_sec))
  {
    PyErr_SetString(PyExc_TypeError, "'timestamp_sec' should be a 'long' type.");
    return NULL;
  }

  if (!PyLong_Check(timestamp_usec))
  {
    PyErr_SetString(PyExc_TypeError, "'timestamp_usec' should be a 'long' type");
    return NULL;
  }

  entry.key = PyString_AsString(key);
  entry.key_count = strlen(entry.key) + 1;
  entry.group = PyString_AsString(key);
  entry.group_count = strlen(entry.group) + 1;
  entry.type = PyInt_AsLong(type);
  entry.subtype = PyInt_AsLong(subtype);
  entry.timestamp_sec = PyInt_AsLong(timestamp_sec);
  entry.timestamp_usec = PyInt_AsLong(timestamp_usec);

  switch (entry.subtype)
  {
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_NONE:
      entry.data = NULL;
      entry.data_count = 0;
      break;
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_STRING:
      if (!PyString_Check(data))
      {
        PyErr_SetString(PyExc_TypeError, "'data' should be a 'string' type.");
        return NULL;
      }
      str = NULL;
      str = PyString_AsString(data);
      length = strlen(str) + 1;
      entry.data = malloc(length);
      assert(entry.data);
      memcpy(entry.data, str, length);
      entry.data_count = length;
      break;
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_INT:
      if (!PyLong_Check(data))
      {
        PyErr_SetString(PyExc_TypeError, "'data' should be a 'long' type.");
        return NULL;
      }
      i = 0;
      i = PyInt_AsLong(data);
      length = sizeof(i);
      entry.data = malloc(length);
      assert(entry.data);
      memcpy(&entry.data, &i, length);
      entry.data_count = length;
      break;
    case PLAYERC_BLACKBOARD_DATA_SUBTYPE_DOUBLE:
      if (!PyLong_Check(data))
      {
        PyErr_SetString(PyExc_TypeError, "'data' should be a 'long' type.");
        return NULL;
      }
      d = 0.0;
      d = PyLong_AsDouble(data);
      length = sizeof(d);
      entry.data = malloc(length);
      assert(entry.data);
      memcpy(&entry.data, &d, length);
      entry.data_count = length;
      break;
    default:
      entry.data = NULL;
      entry.data_count = 0;
      break;
  }

  result = playerc_blackboard_set_entry(self, &entry);
  groups_dict = PyTuple_GetItem((PyObject*)self->py_private, DICT_GROUPS_INDEX); // Borrowed reference
  assert(groups_dict);
  Py_INCREF(dict);
  playerc_blackboard___set_nested_dictionary_entry__(self, groups_dict, entry.key, entry.group, dict); // Steals reference to entry
  free(entry.data);

  return PyInt_FromLong(result);
}

}

%{
// This code overrides the default swig wrapper. We use a different callback function for the python interface.
// It performs the same functionality, but points the callback function to the new python one. 
playerc_blackboard_t *python_playerc_blackboard_create(playerc_client_t *client, int index)
{
  playerc_blackboard_t *device= playerc_blackboard_create(client, index);
  if (device == NULL)
  {
    PyErr_SetString(PyExc_RuntimeError, "Failed to create blackboard");
    return NULL;
  }

  // Tuple item 0: Dictionary of group/keys/values. As in dict['group']['key']=value. Updated on client.read() callback.
  // Tuple item 1: Dictionary of keys that have been subscribed to.
  // Tuple item 2: List of all events that have occurred. Need to call set_queue_events(True). Updated on client.read() callback.
  // Tuple item 3: Whether to queue all events or not. Default not. (0 for false, non-zero for true)
  device->py_private = (void*)Py_BuildValue("({},{},[],i)",0); // tuple of a dict and a list to store our keys and events in
  
  device->on_blackboard_event = playerc_blackboard___python_on_blackboard_event__;
  return device;

}
#undef new_playerc_blackboard
#define new_playerc_blackboard python_playerc_blackboard_create

// We have to clear up the list we created in the callback function.
void python_playerc_blackboard_destroy(playerc_blackboard_t* device)
{
  playerc_device_term(&device->info);
  if (device->py_private != NULL)
  {
  	Py_XDECREF((PyObject*)device->py_private);
  }
  free(device);
}
#undef del_playerc_blackboard
#define del_playerc_blackboard python_playerc_blackboard_destroy
%}
