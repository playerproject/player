/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/*
 * internal prototypes and such
 *
 * $Id$
 */

#ifndef _PUBSUB_UTIL_H
#define _PUBSUB_UTIL_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERTCP_EXPORT
  #elif defined (playertcp_EXPORTS)
    #define PLAYERTCP_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERTCP_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERTCP_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* the player transport protocol types */
#define PLAYER_TRANSPORT_TCP 1
#define PLAYER_TRANSPORT_UDP 2


/*
 * this function creates a socket of the indicated type and binds it to 
 * the indicated port.
 *
 * NOTE: we pick the IP address (and thus network interface) for binding 
 *       by calling gethostname() and then stripping it down to the first
 *       component (i.e. no domain name).  if this process won't
 *       result in the IP address that you want, tough luck.
 *
 * ARGS:
 *  serverp: this is a value-result param that will contain the socket's info
 *  blocking: whether or not it should be blocking
 *  portnum: port to bind() to
 *  socktype: should be SOCK_DGRAM (for UDP) or SOCK_STREAM (for TCP)
 *  backlog: number of waiting connections to be allowed (TCP only)
 *
 * RETURN: 
 *  On success, the fd of the new socket is returned.  Otherwise, -1 
 *  is returned and an explanatory note is dumped to stderr.
 */
PLAYERTCP_EXPORT int create_and_bind_socket(char blocking, unsigned int host, 
                                    int* portnum, int socktype, int backlog);
#ifdef __cplusplus
}
#endif

#endif
