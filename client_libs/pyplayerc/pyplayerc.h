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

#ifndef PYPLAYERC_H
#define PYPLAYERC_H


/* Acquire lock and set python state for current thread. */
extern void thread_acquire(void);

/* Release lock and set python state to NULL. */
extern void thread_release(void);

// Error handling
extern PyObject *errorob;

// Python wrapper for client type.
typedef struct
{
  PyObject_HEAD

  // The libplayerc client pointer.
  playerc_client_t *client;

  // The available device list.
  PyObject *idlist;
  
} client_object_t;



#endif

