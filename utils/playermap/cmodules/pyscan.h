
/**************************************************************************
 * Desc: Python bindings for the scan library
 * Author: Andrew Howard
 * Date: 1 Apr 2003
 * CVS: $Id$
 *************************************************************************/



// Python wrapper for scan class
typedef struct
{
  PyObject_HEAD
  scan_t *scan;
} pyscan_t;

extern PyTypeObject pyscan_type;
extern PyMethodDef pyscan_methods[];


// Python wrapper for scan match class
typedef struct
{
  PyObject_HEAD
  scan_match_t *scan_match;
} pyscan_match_t;

extern PyTypeObject pyscan_match_type;
extern PyMethodDef pyscan_match_methods[];

// Create a scan_match
extern PyObject *pyscan_match_alloc(pyscan_match_t *self, PyObject *args);


