
%module playerc
%{
#include "playerc.h"
%}

// Integer types
%typemap(out) uint16_t
{
  $result = PyInt_FromLong((long) (unsigned long) $1);
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

// Arrays of playerc_device_info_t
%typemap(out) playerc_device_info [ANY]
{
  int i;
  $result = PyTuple_New(arg1->devinfo_count);
  for (i = 0; i < arg1->devinfo_count; i++) 
  {
    PyObject *o = SWIG_NewPointerObj($1 + i, SWIGTYPE_p_playerc_device_info, 0);
    PyTuple_SetItem($result,i,o);
  }
}

// Arrays of playerc_blobfinder_blob_t
%typemap(out) playerc_blobfinder_blob [ANY]
{
  int i;
  $result = PyTuple_New(arg1->devinfo_count);
  for (i = 0; i < arg1->devinfo_count; i++) 
  {
    PyObject *o = SWIG_NewPointerObj($1 + i, SWIGTYPE_p_playerc_blobfinder_blob, 0);
    PyTuple_SetItem($result,i,o);
  }
}

// Arrays of playerc_fiducial_item_t
%typemap(out) playerc_fiducial_item [ANY]
{
  int i;
  $result = PyTuple_New(arg1->devinfo_count);
  for (i = 0; i < arg1->devinfo_count; i++) 
  {
    PyObject *o = SWIG_NewPointerObj($1 + i, SWIGTYPE_p_playerc_fiducial_item, 0);
    PyTuple_SetItem($result,i,o);
  }
}

// Arrays of playerc_localize_hypoth_t
%typemap(out) playerc_localize_hypoth [ANY]
{
  int i;
  $result = PyTuple_New(arg1->devinfo_count);
  for (i = 0; i < arg1->devinfo_count; i++) 
  {
    PyObject *o = SWIG_NewPointerObj($1 + i, SWIGTYPE_p_playerc_localize_hypoth, 0);
    PyTuple_SetItem($result,i,o);
  }
}

// Arrays of playerc_wifi_link_t
%typemap(out) playerc_wifi_link [ANY]
{
  int i;
  $result = PyTuple_New(arg1->devinfo_count);
  for (i = 0; i < arg1->devinfo_count; i++) 
  {
    PyObject *o = SWIG_NewPointerObj($1 + i, SWIGTYPE_p_playerc_wifi_link, 0);
    PyTuple_SetItem($result,i,o);
  }
}






// Include Player header so we can pick up some constants
#define __PACKED__
%import "../../../../server/player.h"


// Use this for regular c-bindings;
// e.g. playerc_client_connect(client, ...)
//%include "../../playerc.h"

// Use this for object-oriented bindings;
// e.g., client.connect(...)
// This file is created by running ../parse_header.py
%include "playerc_oo.i"
