/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 * $Id$
 *
 * a pure C client
 */

/** @addtogroup clientlibs Client Libraries */
/** @{ */
/** @defgroup player_clientlib_c C client library

Included with Player is a simple, no-frills client interface library
written in ANSI C (@p client_libs/c).  This client is intentionally
primitive and most users will find it inconvenient for writing anything
more that the simplest control program.  Rather than direct use, the C
client should be considered the reference implementation of a Player
client library and should be consulted for networking details when
writing new clients in other languages (the C client can also be used
directly as a low-level substrate for other clients; the C++ client is
implemented in this way).  In the file @p playercclient.c are defined
the 5 device-neutral functions necessary in any client.

*/

/** @} */

#ifndef PLAYERCCLIENT_H
#define PLAYERCCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif


#include <player.h>
#include <netinet/in.h> /* for struct in_addr */

/** @addtogroup player_clientlib_c C client library */
/** @{ */
/** @defgroup player_clientlib_c_core Core functionality */
/** @{ */

/** @brief Connection structure.

A pointer to this structure is passed into all functions; it keeps track
of connection state. */
typedef struct player_connection
{
  /* the socket we will talk on */
  int sock;
  /* transport that we'll use (either PLAYER_TRANSPORT_TCP or
   * PLAYER_TRANSPORT_UDP) */
  int protocol;
  /* our server-supplied unique id; it gets inserted into the first two
   * bytes of the reserved field on all messages sent in UDP mode */
  uint16_t id;
  /* the server's address */
  struct sockaddr_in server_addr;
  /* the banner given back by the server after connection */
  char banner[PLAYER_IDENT_STRLEN];
} player_connection_t;

#define PLAYER_CCLIENT_DEBUG_LEVEL_DEFAULT 5

/** @brief Adjust debug ouput.

Higher numbers are more output, 0 is none.  Incidentally, it returns the
current level, and if you give -1 for &lt;level&gt;, then the current
level is unchanged.  */
int player_debug_level(int level);

/** @brief Connect to server listening at host:port. 

conn is filled in with relevant information, and is used in
subsequent player function calls.  

Returns:
  - 0 if everything is OK (connection opened)
  - -1 if something went wrong (connection NOT opened)
*/
int player_connect(player_connection_t* conn, 
		   const char* host, 
		   const int port);

int player_connect_host(player_connection_t* conn, 
                        const char* host, 
                        const int port);

/** @brief Connect to server listening at addr:port.

conn is filled in with relevant information, and is used in
subsequent player function calls. (alternative to player_connect()
using binary address)

Returns:
  - 0 if everything is OK (connection opened)
  - -1 if something went wrong (connection NOT opened)
*/
int player_connect_ip(player_connection_t* conn, 
		      const struct in_addr* addr, 
		      const int port);
/** @brief Connect to server listening at the address specified in 
sockaddr.

conn is filled in with relevant information, and is used in
subsequent player function calls. player_connect() and player_connect_ip
use this function.

Returns:
  - 0 if everything is OK (connection opened)
  - -1 if something went wrong (connection NOT opened)*/
int player_connect_sockaddr(player_connection_t* conn, 
			    const struct sockaddr_in* server);
/** @brief Close a connection. 

conn should be a value that was previously returned
by a call to player_connect()

Returns:
   - 0 if everything is OK (connection closed)
   - -1 if something went wrong (connection not closed)
*/
int player_disconnect(player_connection_t* conn);

/** @brief Issue some request to the server. 

requestlen is the length of the request.  reply,
if non-NULL, will be used to hold the reply; replylen is the size
of the buffer (player_request() will not overrun your buffer)

Returns:
 - 0 if everything went OK
 - -1 if something went wrong (you should probably close the connection!)
*/
int player_request(player_connection_t* conn, 
                   uint16_t device, uint16_t device_index, 
                   const char* payload, size_t payloadlen, 
                   player_msghdr_t* replyhdr, char* reply, size_t replylen);

/** @brief Issue a single device request (special case of player_request())
 
If grant_access is non-NULL, then the actual granted access will be
written there.

Returns:
  - 0 if everything went OK
  - -1 if something went wrong (you should probably close the connection!)
*/
int player_request_device_access(player_connection_t* conn,
                                 uint16_t device,
                                 uint16_t device_index,
                                 uint8_t req_access,
                                 uint8_t* grant_access,
                                 char* driver_name,
                                 int driver_name_len);

/** @brief Read from the indicated connection.

 Put the data in buffer, up to bufferlen.

Returns:
  - 0 if everything went OK
  - -1 if something went wrong (you should probably close the connection!)
*/
int player_read(player_connection_t* conn, player_msghdr_t* hdr,
                char* payload, size_t payloadlen);

int player_read_tcp(player_connection_t* conn, player_msghdr_t* hdr,
                    char* payload, size_t payloadlen);

int player_read_udp(player_connection_t* conn, player_msghdr_t* hdr,
                    char* payload, size_t payloadlen);

/** brief Write commands to the indicated connection. 

Writes the data contained in command, up to commandlen.

Returns:
  - 0 if everything goes OK
  - -1 if something went wrong (you should probably close the connection!)
*/
int player_write(player_connection_t* conn, 
                 uint16_t device, uint16_t device_index,
                 const char* command, size_t commandlen);
/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif

