
/**************************************************************************
 * Desc: Python bindings for the scan library
 * Author: Andrew Howard
 * Date: 1 Apr 2003
 * CVS: $Id$
 *************************************************************************/

#include "scan.h"

// Python wrapper for scan class
typedef struct
{
  PyObject_HEAD
  scan_t *scan;
} pyscan_t;

extern PyTypeObject pyscan_type;
extern PyMethodDef pyscan_methods[];


// Python wrapper for scan group class
typedef struct
{
  PyObject_HEAD
  scan_group_t *ob;
} pyscan_group_t;

extern PyTypeObject pyscan_group_type;
extern PyMethodDef pyscan_group_methods[];


// Create a scan
extern PyObject *pyscan_group_alloc(pyscan_group_t *self, PyObject *args);


// Python wrapper for scan match class
typedef struct
{
  PyObject_HEAD
  scan_match_t *scan_match;
  pyscan_group_t *pyscan_a;
  pyscan_group_t *pyscan_b;
} pyscan_match_t;

extern PyTypeObject pyscan_match_type;
extern PyMethodDef pyscan_match_methods[];

// Create a scan_match
extern PyObject *pyscan_match_alloc(pyscan_match_t *self, PyObject *args);


