/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2002-2003
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
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
  
} pyclient_t;


#endif

