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
extern void thread_acquire();

/* Release lock and set python state to NULL. */
extern void thread_release();


/* Python wrapper for client type */
typedef struct
{
  PyObject_HEAD
  playerc_client_t *client;
} client_object_t;



#endif

