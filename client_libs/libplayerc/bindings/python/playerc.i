
%module libplayerc
%{
#include "playerc.h"
%}


// Include Player header so we can pick up some constants
#define __PACKED__
%import "player.h"

// Provide array access
%typemap(out) double [ANY] {
  int i;
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++) 
  {
    PyObject *o = PyFloat_FromDouble((double) $1[i]);
    PyList_SetItem($result,i,o);
  }
}


// Provide array access doubly-dimensioned arrays
%typemap(out) double [ANY][ANY] {
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

%include "playerc.h"